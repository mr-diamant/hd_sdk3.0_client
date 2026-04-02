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

// Глобальные переменные для состояния
string g_guid = "##GUID";
bool g_handshakeDone = false;
string g_lastResponse;
bool g_responseReceived = false;

// Колбэк для вывода XML
void CALLBACK readXmlCallback(const char* xml, int len, int errorCode, void* userData) {
    string response(xml, len);
    g_lastResponse = response;
    g_responseReceived = true;
    
    // Пытаемся извлечь GUID из любого XML ответа
    size_t guidPos = response.find("guid=\"");
    if (guidPos != string::npos) {
        guidPos += 6;
        size_t endPos = response.find("\"", guidPos);
        if (endPos != string::npos) {
            string newGuid = response.substr(guidPos, endPos - guidPos);
            if (newGuid != "##GUID") {
                if (g_guid == "##GUID") {
                    g_guid = newGuid;
                    g_handshakeDone = true;
                    cout << "DEBUG: SDK Handshake Successful! Session GUID: " << g_guid << endl;
                }
            }
        }
    }

    if (errorCode != 0) {
        cout << "DEBUG: XML Response (Error " << errorCode << "): " << response << endl;
    } else {
        cout << "DEBUG: XML Response: " << response << endl;
    }
}

// Функция для отправки сырых данных (SDK -> Socket)
HBool CALLBACK sendDataCallback(const char* data, int len, void* userData) {
    SOCKET s = (SOCKET)userData;
    
    // Логируем что SDK пытается отправить
    if (len >= 4) {
        unsigned short cmd = *(unsigned short*)(data + 2);
        if (cmd == 0x2001) cout << "DEBUG: SDK sending Protocol Version Negotiation (0x2001)..." << endl;
        else if (cmd == 0x2003) cout << "DEBUG: SDK sending XML Command (0x2003)..." << endl;
        else cout << "DEBUG: SDK sending Binary Packet (0x" << hex << cmd << dec << ")..." << endl;
    }

    int sent = send(s, data, len, 0);
    return (sent == len) ? HTrue : HFalse;
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

    cout << "Connecting to " << controllerIp << ":10001..." << endl;
    if (connect(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        cerr << "Connect FAILED!" << endl;
        return 1;
    }
    cout << "Connected!" << endl;

    // Инициализация SDK
    IHDProtocol sdk = CreateProtocol();
    InitProtocol(sdk);
    SetProtocolFunc(sdk, kSetReadXml, (void*)readXmlCallback);
    SetProtocolFunc(sdk, kSetSendFunc, (void*)sendDataCallback);
    SetProtocolFunc(sdk, kSetSendFuncData, (void*)s);

    // Запуск протокола (SDK сам отправит 0x2001)
    cout << "Starting SDK protocol engine..." << endl;
    RunProtocol(sdk);

    char buf[16384];
    int timeout = 0;
    
    // Главный цикл обработки
    // 1. Ждем завершения рукопожатия
    cout << "Waiting for GUID handshake..." << endl;
    while (!g_handshakeDone && timeout < 100) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(s, &readSet);
        timeval tv = {0, 100000}; // 100ms
        if (select(0, &readSet, NULL, NULL, &tv) > 0) {
            int b = recv(s, buf, sizeof(buf), 0);
            if (b > 0) {
                UpdateReadData(sdk, buf, b);
            } else if (b == 0) {
                cerr << "Connection closed by controller." << endl;
                break;
            }
        }
        timeout++;
    }

    if (!g_handshakeDone) {
        cerr << "Handshake FAILED! Timed out waiting for GUID." << endl;
    } else {
        // 2. Выполнение команды
        cout << "Step 3: Sending command (" << mode << ")..." << endl;
        
        // Для 'set' и 'verify' используем высокоуровневый SDK Info если возможно,
        // но RebootDevice шлем через SendXml
        if (mode == "reboot") {
            string cmdXml = "<?xml version=\"1.0\" encoding=\"utf-8\"?><sdk guid=\"" + g_guid + "\"><in method=\"RebootDevice\"/></sdk>";
            SendXml(sdk, cmdXml.c_str(), (int)cmdXml.size());
        } else if (mode == "set") {
            // Создаем XML вручную для надежности
            string cmdXml = "<?xml version=\"1.0\" encoding=\"utf-8\"?><sdk guid=\"" + g_guid + 
                            "\"><in method=\"SetSDKTcpServer\"><host value=\"" + serverIp + 
                            "\"/><port value=\"" + to_string(serverPort) + "\"/></in></sdk>";
            SendXml(sdk, cmdXml.c_str(), (int)cmdXml.size());
        } else if (mode == "verify") {
            string cmdXml = "<?xml version=\"1.0\" encoding=\"utf-8\"?><sdk guid=\"" + g_guid + "\"><in method=\"GetSDKTcpServer\"/></sdk>";
            SendXml(sdk, cmdXml.c_str(), (int)cmdXml.size());
            
            // Также запросим DeviceInfo
            cmdXml = "<?xml version=\"1.0\" encoding=\"utf-8\"?><sdk guid=\"" + g_guid + "\"><in method=\"GetDeviceInfo\"/></sdk>";
            SendXml(sdk, cmdXml.c_str(), (int)cmdXml.size());
        }

        // Ждем ответов
        timeout = 0;
        int expectedResponses = (mode == "verify" ? 2 : 1);
        int receivedResponses = 0;
        
        while (receivedResponses < expectedResponses && timeout < 50) {
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(s, &readSet);
            timeval tv = {0, 100000};
            if (select(0, &readSet, NULL, NULL, &tv) > 0) {
                int b = recv(s, buf, sizeof(buf), 0);
                if (b > 0) {
                    UpdateReadData(sdk, buf, b);
                    if (g_responseReceived) {
                        g_responseReceived = false;
                        receivedResponses++;
                    }
                }
            }
            timeout++;
        }
    }

    cout << "Application finished." << endl;
    FreeProtocol(sdk);
    closesocket(s);
    WSACleanup();
    return 0;
}
