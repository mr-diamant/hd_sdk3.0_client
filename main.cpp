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

// Официальные типы данных Huidu SDK
typedef unsigned short huint16;
typedef unsigned int   huint32;
typedef signed char    hint8;

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
// Официальная структура заголовка (HTcpHeader)
typedef struct HTcpHeader {
    huint16 len;    ///< Длина командного пакета
    huint16 cmd;    ///< Значение команды
} HTcpHeader;

// Официальная структура расширенного заголовка (HTcpExt)
typedef struct HTcpExt {
    huint16 len;    ///< Длина командного пакета
    huint16 cmd;    ///< Значение команды
    // Для SDK команд за ним следуют TotalSize (4) и Index (4)
    huint32 totalSize;
    huint32 index;
} HTcpExtAsk, HTcpExtAnswer;

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

// Ручная отправка XML через сокет с использованием HTcpExt заголовока
bool SendRawXml(SOCKET s, const string& xml) {
    HTcpExtAsk hdr;
    hdr.len = (huint16)(sizeof(HTcpExtAsk) + xml.size());
    hdr.cmd = kSDKCmdAsk;
    hdr.totalSize = (huint32)xml.size();
    hdr.index = 0;

    printHex("Send Header (0x2003)", (char*)&hdr, sizeof(HTcpExtAsk));

    if (send(s, (char*)&hdr, sizeof(HTcpExtAsk), 0) == SOCKET_ERROR) return false;
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
            return ToUpper(xml.substr(pos, endPos - pos));
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

    cout << "Connecting to " << controllerIp << ":10001 (Official Protocol)..." << endl;
    if (connect(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        cerr << "Connect FAILED!" << endl;
        return 1;
    }
    cout << "Connected!" << endl;

    // --- ШАГ 1: Binary Handshake (HTcpHeader) ---
    cout << "Step 1: Binary Handshake (v0x1000007)..." << endl;
    VersionPacket vp = {{8, kSDKServiceAsk}, LOCAL_TCP_VERSION};
    send(s, (char*)&vp, sizeof(vp), 0);

    char buf[16384];
    int b = recv(s, buf, 8, 0); 
    if (b >= 8) {
        printHex("Recv Binary Answer", buf, b);
        huint16 resCmd = *(huint16*)(buf + 2);
        if (resCmd == kSDKServiceAnswer) {
            huint32 resVer = *(huint32*)(buf + 4);
            cout << "DEBUG: Binary Handshake OK. Controller Version: 0x" << hex << resVer << dec << endl;
        }
    } else {
        cerr << "Step 1 FAILED!" << endl;
        return 1;
    }

    // --- ШАГ 2: XML GUID Handshake (HTcpExt) ---
    cout << "Step 2: XML GUID Handshake (GetIFVersion)..." << endl;
    string handshakeXml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><sdk guid=\"##GUID\"><in method=\"GetIFVersion\"><version value=\"1000000\"/></in></sdk>";
    if (!SendRawXml(s, handshakeXml)) {
        cerr << "Step 2 FAILED!" << endl;
        return 1;
    }

    b = recv(s, buf, sizeof(buf), 0);
    if (b > 12) {
        string xmlResponse(buf + 12, b - 12);
        cout << "DEBUG: XML Response: " << xmlResponse << endl;
        string guid = ExtractGuid(xmlResponse);
        if (!guid.empty() && guid != "##GUID") {
            g_guid = guid;
            g_handshakeDone = true;
            cout << "DEBUG: SUCCESS! Session GUID: " << g_guid << endl;
        }
    } else {
        cerr << "Step 2 FAILED!" << endl;
        return 1;
    }

    // --- ШАГ 3: Выполнение команд ---
    if (g_handshakeDone) {
        // [ЭКСПЕРИМЕНТАЛЬНО] Пробуем открыть порт 9528 для "авторизации" сессии
        SOCKET s9528 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        sockaddr_in addr9528 = addr;
        addr9528.sin_port = htons(9528);
        cout << "DEBUG: Attempting to open Port 9528 (Management Port)..." << endl;
        connect(s9528, (sockaddr*)&addr9528, sizeof(addr9528)); 

        cout << "Step 3: Sending command (" << mode << ")..." << endl;
        vector<string> commands;
        const string XML_DECL = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";

        if (mode == "reboot") {
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"Reboot\" delay=\"0\"></in></sdk>");
        } else if (mode == "set") {
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"SetSDKTcpServer\"><host value=\"" + serverIp + "\"/><port value=\"" + to_string(serverPort) + "\"/></in></sdk>");
        } else if (mode == "verify") {
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"GetSDKTcpServer\"></in></sdk>");
            commands.push_back(XML_DECL + "<sdk guid=\"" + g_guid + "\"><in method=\"GetDeviceInfo\"></in></sdk>");
        }

        for (const string& cmdXml : commands) {
            if (SendRawXml(s, cmdXml)) {
                b = recv(s, buf, sizeof(buf), 0);
                if (b > 12) {
                    string finalRes(buf + 12, b - 12);
                    cout << "DEBUG: Response: " << finalRes << endl;
                    
                    string status = FindErrorInXml(finalRes);
                    if (status == "Успех") {
                        cout << ">>> SUCCESS! <<<" << endl;
                    } else {
                        cout << ">>> ОШИБКА: " << status << " <<<" << endl;
                    }
                }
            }
        }
        closesocket(s9528);
    }

    cout << "Finished. Cleaning up..." << endl;
    closesocket(s);
    WSACleanup();
    return 0;
}
