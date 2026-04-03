cmake_minimum_required(VERSION 3.10)
project(HDServerConfig)

set(CMAKE_CXX_STANDARD 11)

# Компилируем под нативную (64-бит) архитектуру, так как 32-битная DLL больше не используется
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")

# Пути к заголовкам SDK
set(SDK_ROOT "${CMAKE_SOURCE_DIR}/..")
include_directories(
    "${SDK_ROOT}/HDSDK/Protocol"
    "${SDK_ROOT}/HDSDK/SDK"
    "${SDK_ROOT}/HDSDK/SDK/Data"
)

# Добавляем исполняемый файл
add_executable(HDServerConfig main.cpp)

# Ссылка на библиотеку винсокетов
target_link_libraries(HDServerConfig PRIVATE 
    ws2_32
)
