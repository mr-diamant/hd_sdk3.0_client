#include <iostream>
#include <string>
#include <vector>
#include <winsock2.h>
#include <windows.h>
#include <iomanip>
#include <fstream>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

typedef unsigned short huint16;
typedef unsigned int huint32;
typedef unsigned char huint8;

// Определение кодов ошибок согласно официальной документации
enum HErrorCode {
    kSuccess = 0,            ///< Успех (нормальное состояние)
    kWriteFinish,            ///< Запись файла завершена
    kProcessError,           ///< Ошибка процесса (логики)
    kVersionTooLow,          ///< Версия протокола слишком низкая
    kDeviceOccupa,           ///< Устройство занято другим процессом
    kFileOccupa,             ///< Файл занят
    kReadFileExcessive,      ///< Слишком много пользователей читают файл одновременно
    kInvalidPacketLen,       ///< Ошибка длины пакета данных
    kInvalidParam,           ///< Неверный параметр в запросе
    kNotSpaceToSave,         ///< Недостаточно места в памяти устройства
    kCreateFileFailed,       ///< Не удалось создать файл
    kWriteFileFailed,        ///< Ошибка записи в файл
    kReadFileFailed,         ///< Ошибка чтения файла
    kInvalidFileData,        ///< Неверные (битые) данные файла
    kFileContentError,       ///< Ошибка в содержимом файла
    kOpenFileFailed,         ///< Не удалось открыть файл
    kSeekFileFailed,         ///< Ошибка позиционирования в файле
    kRenameFailed,           ///< Ошибка переименования файла
    kFileNotFound,           ///< Файл не найден
    kFileNotFinish,          ///< Файл получен не полностью
    kXmlCmdTooLong,          ///< XML-команда превышает допустимую длину
    kInvalidXmlIndex,        ///< Недопустимое значение индекса XML команды
    kParseXmlFailed,         ///< Ошибка парсинга XML (синтаксис)
    kInvalidMethod,          ///< Неверное имя метода (отсутствует в системе)
    kMemoryFailed,           ///< Ошибка выделения памяти
    kSystemError,            ///< Внутренняя системная ошибка контроллера
    kUnsupportVideo,         ///< Видеокодек не поддерживается
    kNotMediaFile,           ///< Файл не является медиа-контентом
    kParseVideoFailed,       ///< Ошибка анализа видеофайла
    kUnsupportFrameRate,     ///< Неподдерживаемая частота кадров
    kUnsupportResolution,    ///< Неподдерживаемое разрешение видео
    kUnsupportFormat,        ///< Неподдерживаемый формат файла
    kUnsupportDuration,      ///< Неподдерживаемая длительность
    kDownloadFileFailed,     ///< Ошибка скачивания файла
    kScreenNodeIsNull,       ///< Узел экрана отсутствует (null)
    kNodeExist,              ///< Узел уже существует
    kNodeNotExist,           ///< Узел не существует
    kPluginNotExist,         ///< Плагин не найден
    kCheckLicenseFailed,     ///< Ошибка проверки лицензии (License)
    kNotFoundWifiModule,     ///< Модуль Wi-Fi не обнаружен
    kTestWifiUnsuccessful,   ///< Тест Wi-Fi модуля завершился неудачей
    kRunningError,           ///< Ошибка выполнения (Runtime Error)
    kUnsupportMethod,        ///< Метод не поддерживается данной версией железа/ПО
    kInvalidGUID,            ///< Неверный или истекший GUID сессии
    kFirmwareFormatError,    ///< Ошибка формата файла прошивки
    kTagNotFound,            ///< XML-тег не найден
    kAttrNotFound,           ///< XML-атрибут не найден
    kCreateTagFailed,        ///< Ошибка создания XML-тега
    kUnsupportDeviceType,    ///< Модель устройства не поддерживается данным SDK
};

// Типы команд протокола согласно официальной спецификации
enum HcmdType {
    kTcpHeartbeatAsk        = 0x005f,   ///< Запрос TCP Heartbeat (пульс)
    kTcpHeartbeatAnswer     = 0x0060,   ///< Ответ на TCP Heartbeat
    kSearchDeviceAsk        = 0x1001,   ///< Запрос поиска устройств
    kSearchDeviceAnswer     = 0x1002,   ///< Ответ на поиск устройств
    kErrorAnswer            = 0x2000,   ///< Сообщение об ошибке (бинарное)
    kSDKServiceAsk          = 0x2001,   ///< Запрос согласования версии SDK
    kSDKServiceAnswer       = 0x2002,   ///< Ответ на согласование версии SDK
    kSDKCmdAsk              = 0x2003,   ///< Запрос XML SDK команды
    kSDKCmdAnswer           = 0x2004,   ///< Ответ на XML SDK команду
    kGPSInfoAnswer          = 0x3007,   ///< Ответ с информацией GPS
    kFileStartAsk           = 0x8001,   ///< Запрос начала передачи файла
    kFileStartAnswer        = 0x8002,   ///< Ответ на начало передачи файла
    kFileContentAsk         = 0x8003,   ///< Пакет с содержимым файла
    kFileContentAnswer      = 0x8004,   ///< Ответ на запись содержимого файла
    kFileEndAsk             = 0x8005,   ///< Запрос завершения передачи файла
    kFileEndAnswer          = 0x8006,   ///< Ответ на завершение передачи файла
    kReadFileAsk            = 0x8007,   ///< Запрос чтения файла с устройства
    kReadFileAnswer         = 0x8008,   ///< Ответ на чтение файла
};

const huint32 LOCAL_TCP_VERSION = 0x1000007;

#pragma pack(push, 1)
// Официальная структура базового заголовка
typedef struct HTcpHeader {
    huint16 len;    ///< Длина пакета команд
    huint16 cmd;    ///< Значение команды
} HTcpHeader;

// Официальная структура расширенного заголовка
typedef struct HTcpExt {
    huint16 len;    ///< Длина пакета команд
    huint16 cmd;    ///< Значение команды
    huint8   ext[0]; ///< Стартовый адрес расширенных данных
} HTcpExtAsk, HTcpExtAnswer;

// Структура SDK-запроса, вытекающая из документации:
// Len(2) + Cmd(2) + Total(4) + Index(4)
typedef struct SDKCmdAsk {
    huint16 len;
    huint16 cmd;
    huint32 totalSize;
    huint32 index;
} SDKCmdAsk;

// Структура пакета с ошибкой
typedef struct ErrorPacket {
    huint16 len;
    huint16 cmd;
    huint16 code;
} ErrorPacket;

// Пакет согласования версии
struct VersionPacket {
    HTcpHeader header;
    huint32 version;
};
#pragma pack(pop)

// Глобальные переменные
string g_guid = "##GUID";
bool g_handshakeDone = false;

// Функция получения текстового описания ошибки по коду
const char* GetErrorDescription(int code) {
    switch (code) {
        case kSuccess: return "Успех";
        case kWriteFinish: return "Запись файла завершена";
        case kProcessError: return "Ошибка процесса";
        case kVersionTooLow: return "Версия слишком низкая";
        case kDeviceOccupa: return "Устройство занято";
        case kFileOccupa: return "Файл занят";
        case kReadFileExcessive: return "Превышен лимит параллельного чтения";
        case kInvalidPacketLen: return "Ошибка длины пакета";
        case kInvalidParam: return "Неверный параметр";
        case kNotSpaceToSave: return "Нет места для сохранения";
        case kCreateFileFailed: return "Ошибка создания файла";
        case kWriteFileFailed: return "Ошибка записи файла";
        case kReadFileFailed: return "Ошибка чтения файла";
        case kInvalidFileData: return "Неверные данные файла";
        case kFileContentError: return "Ошибка содержимого файла";
        case kOpenFileFailed: return "Ошибка открытия файла";
        case kSeekFileFailed: return "Ошибка позиционирования";
        case kRenameFailed: return "Ошибка переименования";
        case kFileNotFound: return "Файл не найден";
        case kFileNotFinish: return "Файл не докачан";
        case kXmlCmdTooLong: return "XML команда слишком длинная";
        case kInvalidXmlIndex: return "Неверный индекс XML";
        case kParseXmlFailed: return "Ошибка парсинга XML";
        case kInvalidMethod: return "Неверное имя метода";
        case kMemoryFailed: return "Ошибка памяти";
        case kSystemError: return "Системная ошибка";
        case kUnsupportMethod: return "Метод не поддерживается (блокировка прошивки или модели)";
        case kInvalidGUID: return "Неверный GUID";
        case kUnsupportDeviceType: return "Неподдерживаемая модель устройства";
        default: return "Неизвестная ошибка";
    }
}

// Вспомогательная функция декодирования Base64
vector<unsigned char> Base64Decode(const string& input) {
    static const string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[base64_chars[i]] = i;

    vector<unsigned char> out;
    int val = 0, valb = -8;
    for (unsigned char c : input) {
        if (T[c] == -1) continue;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

// Вспомогательная функция для поиска описания ошибки в XML
string FindErrorInXml(const string& xml) {
    if (xml.find("result=\"kSuccess\"") != string::npos || xml.find("result='kSuccess'") != string::npos) 
        return "Успех";
    
    if (xml.find("kUnsupportMethod") != string::npos) return GetErrorDescription(kUnsupportMethod);
    if (xml.find("kInvalidGUID") != string::npos) return GetErrorDescription(kInvalidGUID);
    if (xml.find("kParseXmlFailed") != string::npos) return GetErrorDescription(kParseXmlFailed);
    if (xml.find("kInvalidMethod") != string::npos) return GetErrorDescription(kInvalidMethod);
    
    return "Неизвестная ошибка или метод";
}

// Вспомогательная функция для hex-вывода
void printHex(const char* label, const char* data, int len) {
    cout << "DEBUG: " << label << " [" << len << " bytes]: ";
    for (int i = 0; i < len; i++) {
        cout << hex << setw(2) << setfill('0') << (int)(unsigned char)data[i] << " ";
    }
    cout << dec << endl;
}

// Приведение строки к верхнему регистру
string ToUpper(string s) {
    for (auto &c : s) c = toupper((unsigned char)c);
    return s;
}

// Ручная отправка XML через сокет с использованием SDKCmdAsk заголовка (Len + Cmd + Total + Index)
bool SendRawXml(SOCKET s, const string& xml) {
    SDKCmdAsk hdr;
    hdr.len = (huint16)(sizeof(SDKCmdAsk) + xml.size());
    hdr.cmd = kSDKCmdAsk;
    hdr.totalSize = (huint32)xml.size();
    hdr.index = 0;

    // printHex("Send Header (0x2003)", (char*)&hdr, sizeof(SDKCmdAsk)); // ОТКЛЮЧАЕМ ВЫВОД

    if (send(s, (char*)&hdr, sizeof(SDKCmdAsk), 0) == SOCKET_ERROR) return false;
    if (send(s, xml.c_str(), (int)xml.size(), 0) == SOCKET_ERROR) return false;
    return true;
}

// Извлечение GUID и приведение к верхнему регистру
string ExtractGuid(const string& xml) {
    size_t pos = xml.find("guid=\"");
    if (pos == string::npos) pos = xml.find("guid='");
    
    if (pos != string::npos) {
        pos += 6;
        char quote = xml[pos - 1];
        size_t endPos = xml.find(quote, pos);
        if (endPos != string::npos) {
            return xml.substr(pos, endPos - pos);
        }
    }
    return "";
}

string ExtractXMLAttr(const string& xml, const string& tag, const string& attr) {
    size_t tagPos = xml.find("<" + tag);
    if (tagPos == string::npos) return "";
    size_t attrPos = xml.find(attr + "=\"", tagPos);
    if (attrPos == string::npos) return "";
    attrPos += attr.length() + 2;
    size_t endPos = xml.find("\"", attrPos);
    return xml.substr(attrPos, endPos - attrPos);
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    if (argc >= 2 && (string(argv[1]) == "--help" || string(argv[1]) == "-h" || string(argv[1]) == "help")) {
        cout << "HDServerConfig - LED Controller Management Utility (HD-A3L / SDK 3.0)" << endl;
        cout << "Usage: HDServerConfig.exe <command> <controller_ip> [params...]" << endl << endl;
        cout << "Commands:" << endl;
        cout << "  verify <IP>\t\t\tGet full device info (Model, RAM, Flash, Version, Server)" << endl;
        cout << "  screenshot <IP> [file]\tCapture screen and save as JPG (GetScreenshot2)" << endl;
        cout << "  list_files <IP> [type]\tList raw media files (optional type: 0-image, 1-video, 6-proj)" << endl;
        cout << "  scan_files <IP>\t\tScan all file categories (0,1,6,8,128,129) and log results" << endl;
        cout << "  raw_cmd <IP> <Method>\t\tSend a custom XML method (e.g., GetTaskInfo)" << endl;
        cout << "  list_playlist <IP>\t\tList current playlist" << endl;
        cout << "  set <IP> <ServerIP> <Port>\tSet the Cloud Server IP and Port" << endl;
        cout << "  reboot <IP>\t\t\tReboot the controller" << endl;
        cout << "  on/off <IP>\t\t\tTurn screen ON or OFF" << endl;
        cout << "  bright <-percent|-get> <IP>\tSet (-50) or Get (-get) brightness level" << endl;
        cout << "  exec_template <IP> <File>\tExecute any custom XML template from the xml_templates folder" << endl;
        cout << "  json_verify <IP>\t\t[Deprecated] Run the internal Port 10001 JSON verification" << endl;
        return 0;
    }

    if (argc < 3) {
        cout << "Usage: HDServerConfig.exe <command> <controller_ip> [params...]\nRun 'HDServerConfig.exe --help' for a full list of commands." << endl;
        return 1;
    }

    string mode = argv[1];
    string controllerIp;
    string serverIp = "";
    int serverPort = 0;

    if (mode == "bright") {
        if (argc < 4) {
            cout << "Usage: HDServerConfig.exe bright <-percent|-get> <controller_ip>" << endl;
            return 1;
        }
        controllerIp = argv[3];
    } else if (mode == "on" || mode == "off" || mode == "list_files" || mode == "list_programs" || mode == "list_playlist" || mode == "scan_files" || mode == "screenshot") {
        if (argc < 3) {
            cout << "Usage: HDServerConfig.exe " << mode << " <controller_ip> [params...]" << endl;
            return 1;
        }
        controllerIp = argv[2];
    } else if (mode == "raw_cmd") {
        if (argc < 4) {
             cout << "Usage: HDServerConfig.exe raw_cmd <controller_ip> <MethodName>" << endl;
             return 1;
        }
        controllerIp = argv[2];
    } else {
        if (argc < 3) {
            cout << "Usage: HDServerConfig.exe <command> <controller_ip> [params...]\nRun 'HDServerConfig.exe --help' for a full list of commands." << endl;
            return 1;
        }
        controllerIp = argv[2];
        serverIp = (argc > 3) ? argv[3] : "";
        serverPort = (argc > 4) ? stoi(argv[4]) : 0;
    }

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        cerr << "Socket creation failed" << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(20001); // ИЗМЕНЕН ПОРТ ДЛЯ A3L СЕРИИ НА 20001
    addr.sin_addr.s_addr = inet_addr(controllerIp.c_str());

    if (connect(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        cerr << "Connect FAILED!" << endl;
        return 1;
    }

    if (mode != "json_verify") {
        // --- ШАГ 1: Binary Handshake (HTcpHeader) - Обязателен для старого XML протокола ---
        VersionPacket vp = {{8, kSDKServiceAsk}, LOCAL_TCP_VERSION};
        send(s, (char*)&vp, sizeof(vp), 0);

        char buf[16384];
        int b = recv(s, buf, 8, 0); 
        if (b >= 8) {
            // Handshake OK
        } else {
            cerr << "Step 1 FAILED!" << endl;
            return 1;
        }

        // --- ШАГ 2: XML GUID Handshake (HTcpExt) ---
        string handshakeXml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><sdk guid=\"##GUID\"><in method=\"GetIFVersion\"><version value=\"1000000\"/></in></sdk>";
        if (!SendRawXml(s, handshakeXml)) {
            cerr << "Step 2 FAILED!" << endl;
            return 1;
        }

        b = recv(s, buf, sizeof(buf), 0);
        if (b > 12) {
            string xmlResponse(buf + 12, b - 12);
            string guid = ExtractGuid(xmlResponse);
            if (!guid.empty() && guid != "##GUID") {
                // ВАЖНО: BoxPlayer (20001) использует GUID в нижнем регистре без тире.
                // Не меняем регистр! Оставляем в точности как прислал контроллер.
                g_guid = guid;
                g_handshakeDone = true;
            }
        } else {
            cerr << "Step 2 FAILED!" << endl;
            return 1;
        }
    } else {
        // Для JSON API пропускаем старый хендшейк
        g_handshakeDone = true;
    }

    // --- ШАГ 3: Выполнение команд ---
    if (g_handshakeDone) {
        vector<string> commands;
        const string XML_DECL = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";

        char buf[16384];
        int b;
        SOCKET s9528 = INVALID_SOCKET;

        /* [ОТКЛЮЧЕНО] - На порту 20001 это может мешать
        if (mode != "json_verify") {
            s9528 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            sockaddr_in addr9528 = addr;
            addr9528.sin_port = htons(9528);
            connect(s9528, (sockaddr*)&addr9528, sizeof(addr9528)); 
        }
        */

        if (mode == "reboot") {
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"Reboot\" delay=\"0\"></in></sdk>");
        } else if (mode == "on") {
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"OpenScreen\"></in></sdk>");
        } else if (mode == "off") {
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"CloseScreen\"></in></sdk>");
        } else if (mode == "list_files") {
            string type = (argc > 3) ? argv[3] : "";
            if (type.empty()) {
                commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"GetFiles\"></in></sdk>");
            } else {
                commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"GetFiles\" type=\"" + type + "\"></in></sdk>");
            }
        } else if (mode == "list_playlist") {
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"GetPlayList\"></in></sdk>");
        } else if (mode == "list_programs") {
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"GetPrograms\"></in></sdk>");
        } else if (mode == "screenshot") {
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"GetScreenshot2\"><image width=\"0\" height=\"0\"/></in></sdk>");
        } else if (mode == "scan_files") {
            vector<string> types = {"0", "1", "6", "8", "128", "129"};
            for (auto t : types) {
                commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"GetFiles\" type=\"" + t + "\"></in></sdk>");
            }
        } else if (mode == "raw_cmd") {
            string method = (argc > 3) ? argv[3] : "";
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"" + method + "\"></in></sdk>");
        } else if (mode == "bright") {
            string val = argv[2];
            if (val == "-get") {
                commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"GetLuminancePloy\"></in></sdk>");
            } else {
                // Если передано -50, убираем минус
                if (val[0] == '-') val = val.substr(1);
                commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"SetLuminancePloy\"><mode value=\"default\"/><default value=\"" + val + "\"/><ploy><item enable=\"false\" percent=\"" + val + "\" start=\"00:00:00\"/></ploy><sensor max=\"100\" min=\"1\" time=\"5\"/></in></sdk>");
            }
        } else if (mode == "set") {
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"SetSDKTcpServer\"><server port=\"" + to_string(serverPort) + "\" host=\"" + serverIp + "\"/></in></sdk>");
        } else if (mode == "verify") {
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"GetSDKTcpServer\"></in></sdk>");
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"GetDeviceInfo\"></in></sdk>");
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"GetDeviceName\"></in></sdk>");
        } else if (mode == "exec_template") {
            string filename = (argc > 3) ? argv[3] : "";
            if (filename.empty()) {
                cerr << "Error: No XML file specified for send_xml mode!" << endl;
            } else {
                ifstream file(filename);
                if (!file.is_open()) {
                    cerr << "Error: Could not open file " << filename << endl;
                } else {
                    stringstream buffer;
                    buffer << file.rdbuf();
                    string payload = buffer.str();
                    
                    // Заменяем универсальный маркер на реальный токен сессии
                    string search = "##SESSION_GUID##";
                    size_t pos = 0;
                    while ((pos = payload.find(search, pos)) != string::npos) {
                        payload.replace(pos, search.length(), g_guid);
                        pos += g_guid.length();
                    }
                    commands.push_back(payload);
                }
            }
        } else if (mode == "json_verify") {
            // ЭТАП 1: АВТОРИЗАЦИЯ (Подделка под HDPlayer)
            string loginJson = "{\r\n"
                 "    \"ask\": {\r\n"
                 "        \"Advanced\": {\r\n"
                 "            \"deviceName\": \"BoxPlayer2\",\r\n"
                 "            \"disableUSB\": false,\r\n"
                 "            \"otgMode\": 1,\r\n"
                 "            \"performanceMode\": \"\",\r\n"
                 "            \"storageLocation\": \"local\",\r\n"
                 "            \"voiceControl\": false\r\n"
                 "        }\r\n"
                 "    },\r\n"
                 "    \"log\": \"Windows,HDPlayer,tau1k,volodya,,,_,2026-04-03_01:31:54,wireless_32769-169.254.196.44-1A:B5:CD:9B:76:C7,wireless_32772-169.254.4.142-16:B5:CD:9B:76:C7,wireless_32768-192.168.8.16-14:B5:CD:9B:76:C7,ethernet_32770-169.254.43.194-14:B5:CD:9B:76:C8,\",\r\n"
                 "    \"module\": \"Set\",\r\n"
                 "    \"uuid\": \"{2208a526-d57d-411a-aba8-d1f5c747ea27}\",\r\n"
                 "    \"version\": 5\r\n"
                 "}";
            SDKCmdAsk hdrLogin;
            hdrLogin.len = (huint16)(sizeof(SDKCmdAsk) + loginJson.size());
            hdrLogin.cmd = 0x2100;
            hdrLogin.totalSize = (huint32)loginJson.size();
            hdrLogin.index = 0;
            send(s, (char*)&hdrLogin, sizeof(SDKCmdAsk), 0);
            send(s, loginJson.c_str(), (int)loginJson.size(), 0);
            cout << "DEBUG: Sent JSON Login (Log Registry)" << endl;

            // ЭТАП 2: ЗАПРОС ИНФОРМАЦИИ
            string jsonAsk = "{\r\n"
                             "    \"ask\": {\r\n"
                             "        \"list\": [\r\n"
                             "            \"NetworkInfo\",\r\n"
                             "            \"CloudServer\",\r\n"
                             "            \"DeviceInfo\"\r\n"
                             "        ]\r\n"
                             "    },\r\n"
                             "    \"module\": \"Get\",\r\n"
                             "    \"tag\": \"CloudServer;;NetworkInfo;;DeviceInfo\",\r\n"
                             "    \"uuid\": \"{e53d9ba6-cb25-4fbc-9d4c-5dda594e9b09}\",\r\n"
                             "    \"version\": 5\r\n"
                             "}";
            
            SDKCmdAsk hdr;
            hdr.len = (huint16)(sizeof(SDKCmdAsk) + jsonAsk.size());
            hdr.cmd = 0x2100; // ПРАВИЛЬНАЯ секретная команда (0x2100)
            hdr.totalSize = (huint32)jsonAsk.size();
            hdr.index = 0;

            send(s, (char*)&hdr, sizeof(SDKCmdAsk), 0);
            send(s, jsonAsk.c_str(), (int)jsonAsk.size(), 0);
            cout << "DEBUG: Sent JSON Request for CloudServer & DeviceInfo" << endl;
            
            // Запускаем цикл обработки пакетов
            int empty_reads = 0;
            bool gotResponse = false;

            while (empty_reads < 20 && !gotResponse) {
                b = recv(s, buf, sizeof(buf), 0);
                if (b > 0) {
                    empty_reads = 0;
                    int offset = 0;
                    
                    while (offset + 4 <= b) {
                        huint16 pLen = *(huint16*)(buf + offset);
                        huint16 pCmd = *(huint16*)(buf + offset + 2);

                        // Ждем пока докачается весь пакет (простейшая обработка фрагментации TCP)
                        while (offset + pLen > b) {
                            int b2 = recv(s, buf + b, sizeof(buf) - b, 0);
                            if (b2 > 0) b += b2;
                            else break;
                        }

                        if (pCmd == kTcpHeartbeatAsk) {
                            cout << "DEBUG: Recv Heartbeat Ask (0x005f). Sending Answer (0x0060)..." << endl;
                            HTcpHeader ans;
                            ans.len = 4;
                            ans.cmd = kTcpHeartbeatAnswer;
                            send(s, (char*)&ans, 4, 0);
                        } else {
                            cout << "DEBUG: Recv Packet: len=" << pLen << " cmd=0x" << hex << pCmd << dec << endl;
                            if (pLen > 12) {
                                string finalRes(buf + offset + 12, pLen - 12);
                                cout << ">>> JSON RESPONSE <<<" << endl << finalRes << endl;
                                gotResponse = true;
                            } else {
                                printHex("Unknown Packet", buf + offset, pLen);
                            }
                        }
                        
                        offset += pLen;
                        if (pLen == 0) break; // Защита от бесконечного цикла
                    }
                } else {
                    empty_reads++;
                    Sleep(100);
                }
            }
            
            if (!gotResponse) {
                cout << ">>> ОШИБКА: Превышено время ожидания ответа <<<" << endl;
            }
        }

        // Блок отправки XML-команд (если это не json_verify)
        if (mode != "json_verify") {
            for (const string& cmdXml : commands) {
                if (SendRawXml(s, cmdXml)) {
                    b = recv(s, buf, sizeof(buf), 0);
                    if (b == 6) {
                        huint16 resCmd = *(huint16*)(buf + 2);
                        if (resCmd == kErrorAnswer) {
                            huint16 errCode = *(huint16*)(buf + 4);
                            cout << ">>> ERROR: " << GetErrorDescription(errCode) << " (Code: " << errCode << ") <<<" << endl;
                        }
                    } else if (b > 12) {
                        string finalRes(buf + 12, b - 12);
                        string status = FindErrorInXml(finalRes);
                        if (status == "Успех") {
                            // Форматированный вывод
                            if (cmdXml.find("GetSDKTcpServer") != string::npos) {
                                string outHost = ExtractXMLAttr(finalRes, "server", "host");
                                string outPort = ExtractXMLAttr(finalRes, "server", "port");
                                if (outHost.empty()) cout << "[Server Config] No Cloud Server connected." << endl;
                                else cout << "[Server Config] IP: " << outHost << " Port: " << outPort << endl;
                            } else if (cmdXml.find("GetDeviceInfo") != string::npos) {
                                string devId = ExtractXMLAttr(finalRes, "device", "id");
                                string devCpu = ExtractXMLAttr(finalRes, "device", "cpu");
                                string outWidth = ExtractXMLAttr(finalRes, "screen", "width");
                                string outHeight = ExtractXMLAttr(finalRes, "screen", "height");
                                
                                string ram = ExtractXMLAttr(finalRes, "memory", "size");
                                string fFree = ExtractXMLAttr(finalRes, "flash", "free");
                                string fTotal = ExtractXMLAttr(finalRes, "flash", "total");
                                string softVer = ExtractXMLAttr(finalRes, "version", "value");
                                if (softVer.empty()) softVer = ExtractXMLAttr(finalRes, "version", "software");

                                cout << "[Device Info]" << endl;
                                cout << "  Model/CPU: " << devId << " (CPU: " << devCpu << ")" << endl;
                                cout << "  Screen:    " << outWidth << "x" << outHeight << endl;
                                if (!ram.empty())     cout << "  RAM Size:  " << ram << " MB" << endl;
                                if (!fFree.empty())   cout << "  Flash:     " << fFree << "/" << fTotal << " MB (Free/Total)" << endl;
                                if (!softVer.empty()) cout << "  SW Ver:    " << softVer << endl;
                            } else if (cmdXml.find("GetDeviceName") != string::npos) {
                                string devName = ExtractXMLAttr(finalRes, "name", "value");
                                cout << "[Device Name] " << devName << endl;
                            } else if (cmdXml.find("GetFiles") != string::npos) {
                                // Сохраняем сырой ответ в лог-файл
                                ofstream logFile("sdk_debug.log", ios::app);
                                if (logFile.is_open()) {
                                    logFile << "\n--- [GetFiles Raw Response] ---\n" << finalRes << "\n-----------------------------\n" << endl;
                                    logFile.close();
                                    cout << "[Debug] Raw response saved to sdk_debug.log" << endl;
                                }

                                cout << "[Files List]" << endl;
                                cout << left << setw(30) << "Name" << setw(12) << "Size(KB)" << setw(10) << "Type" << "MD5" << endl;
                                cout << string(80, '-') << endl;

                                size_t pos = 0;
                                int count = 0;
                                // Более гибкий поиск тега <file (регистронезависимость не в C++11 по умолчанию, но ищем просто <file)
                                string lowerRes = finalRes;
                                for(auto &c : lowerRes) c = tolower((unsigned char)c);

                                while ((pos = lowerRes.find("<file", pos)) != string::npos) {
                                    size_t tagEnd = lowerRes.find(">", pos);
                                    if (tagEnd == string::npos) break;
                                    
                                    string fileTag = finalRes.substr(pos, tagEnd - pos + 1);
                                    
                                    // Используем существующий ExtractXMLAttr, он ищет внутри строки
                                    string name = ExtractXMLAttr(fileTag, "file", "name");
                                    if (name.empty()) name = ExtractXMLAttr(fileTag, "file", "Name"); // на всякий случай

                                    string size = ExtractXMLAttr(fileTag, "file", "size");
                                    if (size.empty()) size = ExtractXMLAttr(fileTag, "file", "Size");

                                    string type = ExtractXMLAttr(fileTag, "file", "type");
                                    string md5 = ExtractXMLAttr(fileTag, "file", "md5");
                                    if (md5.empty()) md5 = ExtractXMLAttr(fileTag, "file", "MD5");

                                    // Расшифровка типов
                                    string typeStr = "Other";
                                    if (type == "0") typeStr = "Image";
                                    else if (type == "1") typeStr = "Video";
                                    else if (type == "2") typeStr = "Font";
                                    else if (type == "3") typeStr = "Firmware";
                                    else if (type == "4") typeStr = "FPGA";
                                    else if (type == "5") typeStr = "Config";
                                    else if (type == "6") typeStr = "Resources";

                                    long sizeKb = (size.empty() ? 0 : stol(size)) / 1024;
                                    if (!name.empty()) {
                                        cout << left << setw(30) << name << setw(12) << sizeKb << setw(10) << typeStr << md5 << endl;
                                        count++;
                                    }
                                    
                                    pos = tagEnd + 1;
                                }
                                if (count == 0) cout << "(No files found)" << endl;
                                else cout << "Total files: " << count << endl;
                            } else if (cmdXml.find("GetPlayList") != string::npos) {
                                ofstream logFile("sdk_debug.log", ios::app);
                                if (logFile.is_open()) {
                                    logFile << "\n--- [GetPlayList Raw Response] ---\n" << finalRes << "\n-----------------------------\n" << endl;
                                    logFile.close();
                                    cout << "[Debug] Raw response saved to sdk_debug.log" << endl;
                                }
                                cout << "[Playlist Info]" << endl << finalRes << endl;
                            } else if (cmdXml.find("GetPrograms") != string::npos) {
                                // Логируем ответ
                                ofstream logFile("sdk_debug.log", ios::app);
                                if (logFile.is_open()) {
                                    logFile << "\n--- [GetPrograms Raw Response] ---\n" << finalRes << "\n-----------------------------\n" << endl;
                                    logFile.close();
                                    cout << "[Debug] Raw response saved to sdk_debug.log" << endl;
                                }
                                cout << "[Programs Info]" << endl << finalRes << endl;
                            } else if (cmdXml.find("GetScreenshot2") != string::npos || cmdXml.find("ScreenShot") != string::npos) {
                                // Обработка скриншота (может быть в GetScreenshot2 или ScreenShot)
                                string imgData = ExtractXMLAttr(finalRes, "image", "data");
                                if (imgData.empty()) imgData = ExtractXMLAttr(finalRes, "screenshot", "image"); // для ScreenShot

                                if (!imgData.empty()) {
                                    string outName = "screenshot.jpg";
                                    if (argc > 3 && mode == "screenshot") outName = argv[3];

                                    vector<unsigned char> jpg = Base64Decode(imgData);
                                    if (!jpg.empty()) {
                                        ofstream outFile(outName, ios::binary);
                                        if (outFile.is_open()) {
                                            outFile.write((const char*)jpg.data(), jpg.size());
                                            outFile.close();
                                            cout << "[Success] Screenshot saved to: " << outName << " (" << jpg.size() << " bytes)" << endl;
                                        } else {
                                             cout << ">>> ERROR: Field to create file " << outName << " <<<" << endl;
                                        }
                                    } else {
                                        cout << ">>> ERROR: Base64 decode failed <<<" << endl;
                                    }
                                } else {
                                    // Fallback: если GetScreenshot2 не поддерживается, пробуем старый ScreenShot сразу
                                    if (cmdXml.find("GetScreenshot2") != string::npos && finalRes.find("kUnsupportMethod") != string::npos) {
                                        cout << "[Info] GetScreenshot2 not supported, trying legacy ScreenShot..." << endl;
                                        string legacyXml = XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"ScreenShot\" /></sdk>";
                                        if (SendRawXml(s, legacyXml)) {
                                            // Получаем еще один ответ
                                            b = recv(s, buf, sizeof(buf), 0);
                                            if (b > 12) {
                                                string legacyRes(buf + 12, b - 12);
                                                string legacyImg = ExtractXMLAttr(legacyRes, "screenshot", "image");
                                                if (!legacyImg.empty()) {
                                                     string outName = "screenshot.jpg";
                                                     if (argc > 3) outName = argv[3];
                                                     vector<unsigned char> jpg = Base64Decode(legacyImg);
                                                     ofstream outFile(outName, ios::binary);
                                                     outFile.write((const char*)jpg.data(), jpg.size());
                                                     outFile.close();
                                                     cout << "[Success] Screenshot (Legacy) saved to: " << outName << endl;
                                                } else {
                                                    cout << ">>> ERROR: Legacy screenshot failed too <<<" << endl;
                                                }
                                            }
                                        }
                                    } else {
                                        cout << ">>> ERROR: No image data in response <<<" << endl;
                                    }
                                }
                            } else if (mode == "scan_files" && cmdXml.find("GetFiles") != string::npos) {
                                string type = ExtractXMLAttr(cmdXml, "in", "type");
                                ofstream logFile("sdk_debug.log", ios::app);
                                if (logFile.is_open()) {
                                    logFile << "\n--- [Scan Category: " << type << "] ---\n" << finalRes << "\n-----------------------------\n" << endl;
                                    logFile.close();
                                }
                                if (finalRes.find("<file") != string::npos) {
                                    cout << "[Found] Files found in category " << type << " (check sdk_debug.log for details)" << endl;
                                } else {
                                    cout << "[Empty] Category " << type << " is empty." << endl;
                                }
                            } else if (mode == "raw_cmd") {
                                ofstream logFile("sdk_debug.log", ios::app);
                                if (logFile.is_open()) {
                                    logFile << "\n--- [Raw CMD: " << argv[3] << "] ---\n" << finalRes << "\n-----------------------------\n" << endl;
                                    logFile.close();
                                    cout << "[Debug] Raw result saved to sdk_debug.log" << endl;
                                }
                                cout << ">>> RESPONSE <<<" << endl << finalRes << endl;
                            } else if (cmdXml.find("GetLuminancePloy") != string::npos) {
                                string curBright = ExtractXMLAttr(finalRes, "default", "value");
                                cout << "[Brightness] Current level: " << curBright << "%" << endl;
                            } else {
                                cout << "[Success] Command completed." << endl;
                            }
                        } else {
                            cout << ">>> ERROR: " << status << " <<<" << endl;
                        }
                    }
                }
            }
        }
        if (s9528 != INVALID_SOCKET) {
            closesocket(s9528);
        }
    }

    closesocket(s);
    WSACleanup();
    return 0;
}
