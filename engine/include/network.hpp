#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>
#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Color.hpp>

// Unix/macOS заголовки
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>

// Порт по умолчанию
const int DEFAULT_PORT = 5555;
const int BUFFER_SIZE = 4096;

// Структура для передачи данных о планете
struct PlanetData {
    int id;
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Color color;
    float radius;
    float mass;
};

// Базовый сетевой класс
class NetworkBase {
protected:
    std::atomic<bool> running{false};
    
public:
    NetworkBase() = default;
    virtual ~NetworkBase() = default;
    
    static std::string getLocalIP();
    static bool isValidIP(const std::string& ip);
    static bool isValidPort(int port);
    
    // Вспомогательные функции для работы с сокетами
    static bool setSocketNonBlocking(int socket);
    static bool setSocketTimeout(int socket, int seconds);
    
    // Флаги для sendto/recvfrom
    static int getSendFlags();
};

#endif // NETWORK_HPP