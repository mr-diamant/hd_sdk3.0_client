#include <iostream>
#include <string>
#include <vector>
#include <winsock2.h>
#include <windows.h>
#include <iomanip>

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
        cout << "  verify <IP>\t\t\tGet device info, model, and current server settings" << endl;
        cout << "  set <IP> <ServerIP> <Port>\tSet the Cloud Server IP and Port" << endl;
        cout << "  reboot <IP>\t\t\tReboot the controller" << endl;
        cout << "  add_program <IP>\t\tSend an example program (text) to the screen" << endl;
        cout << "  clear <IP>\t\t\tClear all programs from the screen" << endl;
        cout << "  screen_off <IP>\t\tTurn off the LED screen" << endl;
        cout << "  screen_on <IP>\t\tTurn on the LED screen" << endl;
        cout << "  json_verify <IP>\t\t[Deprecated] Run the internal Port 10001 JSON verification" << endl;
        return 0;
    }

    if (argc < 3) {
        cout << "Usage: HDServerConfig.exe <command> <controller_ip> [params...]\nRun 'HDServerConfig.exe --help' for a full list of commands." << endl;
        return 1;
    }

    string mode = argv[1];
    string controllerIp = argv[2];
    string serverIp = (argc > 3) ? argv[3] : "";
    int serverPort = (argc > 4) ? stoi(argv[4]) : 0;

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
        } else if (mode == "set") {
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"SetSDKTcpServer\"><server port=\"" + to_string(serverPort) + "\" host=\"" + serverIp + "\"/></in></sdk>");
        } else if (mode == "verify") {
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"GetSDKTcpServer\"></in></sdk>");
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"GetDeviceInfo\"></in></sdk>");
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"GetDeviceName\"></in></sdk>");
        } else if (mode == "add_program") {
            // Тестируем точную структуру из примера документации
            string addProgXml = XML_DECL + "<sdk guid=\"" + g_guid + "\">"
                                "<in method=\"AddProgram\">"
                                "<screen timeStamps=\"0\">"
                                "<program guid=\"prog-1234\" type=\"normal\">"
                                "<backgroundMusic/>"
                                "<playControl count=\"1\" disabled=\"false\"/>"
                                "<area alpha=\"255\" guid=\"area-1234\">"
                                "<rectangle x=\"0\" height=\"128\" width=\"128\" y=\"0\"/>"
                                "<resources>"
                                "<text guid=\"text-1234\" singleLine=\"false\">"
                                "<style valign=\"middle\" align=\"center\"/>"
                                "<string>Example</string>"
                                "<font name=\"SimSun\" italic=\"false\" bold=\"false\" underline=\"false\" size=\"28\" color=\"#ffffff\"/>"
                                "</text>"
                                "</resources>"
                                "</area>"
                                "</program>"
                                "</screen>"
                                "</in>"
                                "</sdk>";
            commands.push_back(addProgXml);
        } else if (mode == "clear") {
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"ClearProgram\"></in></sdk>");
        } else if (mode == "screen_on") {
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"SwitchScreen\"><screen on=\"true\"/></in></sdk>");
        } else if (mode == "screen_off") {
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"SwitchScreen\"><screen on=\"false\"/></in></sdk>");
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
                                cout << "[Device Info] Model: " << devId << " (CPU: " << devCpu << ")" << endl;
                                cout << "[Screen Size] " << outWidth << "x" << outHeight << endl;
                            } else if (cmdXml.find("GetDeviceName") != string::npos) {
                                string devName = ExtractXMLAttr(finalRes, "name", "value");
                                cout << "[Device Name] " << devName << endl;
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
