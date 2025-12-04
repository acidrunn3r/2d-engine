cmake_minimum_required(VERSION 3.16)
project(2d-engine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Добавляем SFML
add_subdirectory(SFML-2.6.2)

# Добавляем engine
add_subdirectory(engine)

# Создаем главное приложение
add_executable(2d-engine main.cpp)

# Линкуем с engine и SFML
target_link_libraries(2d-engine
    engine
    sfml-graphics
    sfml-window
    sfml-system
)

# Добавляем include директории
target_include_directories(2d-engine PRIVATE
    ${CMAKE_SOURCE_DIR}/engine/include
    ${CMAKE_SOURCE_DIR}/SFML-2.6.2/include
)

# Для macOS/Linux добавляем pthread
if(APPLE OR UNIX)
    target_link_libraries(2d-engine pthread)
endif()

# Копируем ассеты
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})