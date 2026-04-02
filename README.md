# HDServerConfig

Утилита командной строки для настройки LED-контроллеров Huidu (серия HD-A3L) через SDK 3.0.

Проект предназначен для автоматизации настройки сетевых параметров контроллеров, в частности — указания адреса внешнего TCP-сервера для удаленного управления.

## Основные возможности

*   **set**: Установка IP-адреса и порта внешнего сервера (SDK TCP Server).
*   **verify**: Чтение текущих настроек контроллера и информации об устройстве (модель, версия прошивки).
*   **reboot**: Дистанционная перезагрузка контроллера для применения настроек.

## Требования для сборки

Проект настроен для компиляции в 32-битном режиме (необходимо для совместимости с `HDSDK.dll`).

*   **CMake** (3.10+)
*   **Ninja** (опционально, как генератор)
*   **LLVM-MinGW** (i686-w64-mingw32-g++) или MSVC (x86)
*   Библиотека **HDSDK.dll** (должна быть в папке с .exe или в системе)

## Сборка (MinGW-w64 x86)

```powershell
mkdir build
cd build
cmake -G Ninja -DCMAKE_CXX_COMPILER=i686-w64-mingw32-g++ ..
ninja
```

## Использование

### 1. Установка адреса сервера
Настройка контроллера на подключение к вашему серверу (например, 77.42.66.37):
```powershell
.\HDServerConfig.exe set 192.168.8.2 77.42.66.37 10001
```

### 2. Проверка настроек
Просмотр текущего состояния и подтверждение версии "D" (инженерная разработка):
```powershell
.\HDServerConfig.exe verify 192.168.8.2
```

### 3. Удаленная перезагрузка
```powershell
.\HDServerConfig.exe reboot 192.168.8.2
```

## Технические детали

Программа реализует полный цикл авторизации в протоколе Huidu SDK 3.0:
1.  **Binary Handshake**: Согласование версии протокола (используется `0x1000009`).
2.  **XML Authentication**: Получение сессионного GUID через метод `GetIFVersion`.
3.  **Command Execution**: Отправка команд в бинарной обертке `0x2003` (kSDKCmdAsk).

---

## English Short Description

**HDServerConfig** is a CLI tool to configure Huidu LED controllers (HD-A3L) using SDK 3.0. It allows setting custom TCP server addresses, verifying hardware info, and remote rebooting. Built for Win32 compatibility.
