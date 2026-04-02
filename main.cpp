#include <iostream>
#include <string>
#include <vector>
#include <winsock2.h>
#include <windows.h>
#include <HDSDK.h>
#include <SDKInfo.h>
#include <iomanip>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// Константы протокола
const unsigned short kSDKServiceAsk = 0x2001;
const unsigned short kSDKServiceAnswer = 0x2002;
const unsigned short kSDKCmdAsk = 0x2003;
const unsigned short kSDKCmdAnswer = 0x2004;
const unsigned int LOCAL_TCP_VERSION = 0x1000007;

#pragma pack(push, 1)
struct BinaryHeader {
    unsigned short len;
    unsigned short cmd;
};

struct VersionPacket {
    BinaryHeader header;
    unsigned int version;
};

struct XmlHeader {
    BinaryHeader header;
    unsigned int totalSize;
    unsigned int index;
};
#pragma pack(pop)

// Глобальные переменные
string g_guid = "##GUID";
bool g_handshakeDone = false;

void printHex(const char* label, const char* data, int len) {
    cout << "DEBUG: " << label << " [" << len << " bytes]: ";
    for (int i = 0; i < len; i++) {
        cout << hex << setw(2) << setfill('0') << (int)(unsigned char)data[i] << " ";
    }
    cout << dec << endl;
}

// Ручная отправка XML через сокет с заголовком 0x2003
bool SendRawXml(SOCKET s, const string& xml) {
    XmlHeader hdr;
    hdr.header.len = (unsigned short)(sizeof(XmlHeader) + xml.size());
    hdr.header.cmd = kSDKCmdAsk;
    hdr.totalSize = (unsigned int)xml.size();
    hdr.index = 0;

    printHex("Send SDK Header (0x2003)", (char*)&hdr, sizeof(XmlHeader));
    // cout << "DEBUG: Send XML Content: " << xml << endl;

    if (send(s, (char*)&hdr, sizeof(XmlHeader), 0) == SOCKET_ERROR) return false;
    if (send(s, xml.c_str(), (int)xml.size(), 0) == SOCKET_ERROR) return false;
    return true;
}

// Извлечение GUID из XML ответа
string ExtractGuid(const string& xml) {
    size_t pos = xml.find("guid=\"");
    if (pos != string::npos) {
        pos += 6;
        size_t endPos = xml.find("\"", pos);
        if (endPos != string::npos) {
            return xml.substr(pos, endPos - pos);
        }
    }
    return "";
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    if (argc < 3) {
        cout << "Usage: HDServerConfig.exe <mode> <controller_ip> [params...]" << endl;
        return 1;
    }

    string mode = argv[1];
    string controllerIp = argv[2];
    string serverIp = (argc > 3) ? argv[3] : "";
    int serverPort = (argc > 4) ? stoi(argv[4]) : 0;

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(10001);
    addr.sin_addr.s_addr = inet_addr(controllerIp.c_str());

    cout << "Connecting to " << controllerIp << ":10001 (Manual Mode)..." << endl;
    if (connect(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        cerr << "Connect FAILED!" << endl;
        return 1;
    }
    cout << "Connected!" << endl;

    // --- ШАГ 1: Binary Handshake ---
    cout << "Step 1: Binary Handshake (v0x1000007)..." << endl;
    VersionPacket vp = {{8, kSDKServiceAsk}, LOCAL_TCP_VERSION};
    send(s, (char*)&vp, sizeof(vp), 0);

    char buf[16384];
    int b = recv(s, buf, 8, 0); // Ждем ответ на версию
    if (b >= 8) {
        printHex("Recv Binary Answer", buf, b);
        unsigned short resCmd = *(unsigned short*)(buf + 2);
        if (resCmd == kSDKServiceAnswer) {
            unsigned int resVer = *(unsigned int*)(buf + 4);
            cout << "DEBUG: Binary Handshake OK. Controller Version: 0x" << hex << resVer << dec << endl;
        }
    } else {
        cerr << "Step 1 FAILED! No response from controller." << endl;
        return 1;
    }

    // --- ШАГ 2: XML Handshake (GUID) ---
    cout << "Step 2: XML GUID Handshake (GetIFVersion)..." << endl;
    string handshakeXml = "<?xml version=\"1.0\" encoding=\"utf-8\"?><sdk guid=\"##GUID\"><in method=\"GetIFVersion\"><version value=\"1000000\"/></in></sdk>";
    if (!SendRawXml(s, handshakeXml)) {
        cerr << "Step 2 FAILED! Send error." << endl;
        return 1;
    }

    b = recv(s, buf, sizeof(buf), 0);
    if (b > 12) {
        printHex("Recv XML Header", buf, 12);
        string xmlResponse(buf + 12, b - 12);
        cout << "DEBUG: XML Response: " << xmlResponse << endl;
        string guid = ExtractGuid(xmlResponse);
        if (!guid.empty() && guid != "##GUID") {
            g_guid = guid;
            g_handshakeDone = true;
            cout << "DEBUG: SUCCESS! Session GUID: " << g_guid << endl;
        }
    } else {
        cerr << "Step 2 FAILED! No XML response." << endl;
        return 1;
    }

    // --- ШАГ 3: Выполнение команды ---
    if (g_handshakeDone) {
        cout << "Step 3: Sending command (" << mode << ")..." << endl;
        string cmdXml;
        if (mode == "reboot") {
            cmdXml = "<?xml version=\"1.0\" encoding=\"utf-8\"?><sdk guid=\"" + g_guid + "\"><in method=\"RebootDevice\"/></sdk>";
        } else if (mode == "set") {
            cmdXml = "<?xml version=\"1.0\" encoding=\"utf-8\"?><sdk guid=\"" + g_guid + "\"><in method=\"SetSDKTcpServer\"><host value=\"" + serverIp + "\"/><port value=\"" + to_string(serverPort) + "\"/></in></sdk>";
        } else if (mode == "verify") {
            cmdXml = "<?xml version=\"1.0\" encoding=\"utf-8\"?><sdk guid=\"" + g_guid + "\"><in method=\"GetSDKTcpServer\"/></sdk>";
        }

        if (!cmdXml.empty() && SendRawXml(s, cmdXml)) {
            b = recv(s, buf, sizeof(buf), 0);
            if (b > 12) {
                string finalRes(buf + 12, b - 12);
                cout << "DEBUG: Final Response: " << finalRes << endl;
                if (finalRes.find("result=\"kSuccess\"") != string::npos) {
                    cout << ">>> COMMAND COMPLETED SUCCESSFULLY! <<<" << endl;
                } else if (finalRes.find("result=\"") != string::npos) {
                     cout << ">>> COMMAND RETURNED ERROR! <<<" << endl;
                }
            }
        }
    }

    cout << "Finished. Cleaning up..." << endl;
    closesocket(s);
    WSACleanup();
    return 0;
}
