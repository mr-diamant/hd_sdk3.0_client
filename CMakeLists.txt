cmake_minimum_required(VERSION 3.10)
project(HDServerConfig)

set(CMAKE_CXX_STANDARD 11)

# Указываем 32-битный таргет для совместимости с 32-битной DLL
if(MSVC)
    set(CMAKE_GENERATOR_PLATFORM Win32)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
endif()

# Пути к заголовкам SDK
set(SDK_ROOT "${CMAKE_SOURCE_DIR}/..")
include_directories(
    "${SDK_ROOT}/HDSDK/Protocol"
    "${SDK_ROOT}/HDSDK/SDK"
    "${SDK_ROOT}/HDSDK/SDK/Data"
)

# Добавляем исполняемый файл
add_executable(HDServerConfig main.cpp)

# Ссылка на библиотеку импорта (которую мы сгенерировали из DLL)
target_link_libraries(HDServerConfig PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/HDSDK.lib"
    ws2_32
)

# Копируем DLL в выходную директорию после сборки
add_custom_command(TARGET HDServerConfig POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${SDK_ROOT}/HDSDK/lib/HDSDK.dll"
    $<TARGET_FILE_DIR:HDServerConfig>
)
