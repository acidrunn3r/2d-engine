# Добавляем все исходные файлы engine
add_library(engine STATIC
    src/engine.cpp
    src/network.cpp
    src/server.cpp
    src/client.cpp
    src/ui.cpp
    # Добавляем jsoncpp напрямую
    ${CMAKE_SOURCE_DIR}/jsoncpp_amalgamated/jsoncpp.cpp
)

# Включаем директории
target_include_directories(engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/jsoncpp_amalgamated
    ${CMAKE_SOURCE_DIR}/SFML-2.6.2/include
)

# Линкуем с SFML
target_link_libraries(engine PUBLIC
    sfml-graphics
    sfml-window
    sfml-system
)

# Для macOS/Linux
if(APPLE OR UNIX)
    target_link_libraries(engine PUBLIC pthread)
endif()