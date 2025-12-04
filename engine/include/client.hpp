#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "network.hpp"
#include <functional>
#include <queue>
#include <thread>
#include <SFML/Graphics/Vertex.hpp>

// Типы колбэков для обновлений
using PlanetsUpdateCallback = std::function<void(const std::vector<PlanetData>&)>;
using ConnectionCallback = std::function<void(bool)>;

class GameClient : public NetworkBase {
private:
    std::string server_ip;
    int tcp_port;
    int udp_port;
    
    int tcp_socket;
    int udp_socket;
    
    // Полученные данные
    std::vector<PlanetData> planets;
    std::mutex planets_mutex;
    
    // Траектории
    std::vector<std::vector<sf::Vertex>> trajectories;
    int max_trajectory_points;
    
    // Колбэки
    PlanetsUpdateCallback planets_callback;
    ConnectionCallback connection_callback;
    
    // Флаги
    bool is_connected;
    bool is_host; // Является ли хостом
    
    std::thread receive_thread;
    std::thread udp_thread;
    
public:
    GameClient();
    ~GameClient();
    
    // Подключение
    bool connect(const std::string& ip, int port, bool as_host = false);
    void disconnect();
    bool isConnected() const { return is_connected; }
    bool isHost() const { return is_host; }
    
    // Получение данных
    std::vector<PlanetData> getPlanets();
    std::vector<std::vector<sf::Vertex>> getTrajectories() const;
    
    // Колбэки
    void setPlanetsUpdateCallback(PlanetsUpdateCallback callback);
    void setConnectionCallback(ConnectionCallback callback);
    
    // Для отрисовки
    void updateTrajectories();
    void clearTrajectories();
    
private:
    bool setupTcpConnection();
    bool setupUdpConnection();
    void receiveTcpData();
    void receiveUdpData();
    
    void parsePlanetsJson(const std::string& json);
    void parseUdpUpdate(const std::string& data);
};

#endif // CLIENT_HPP