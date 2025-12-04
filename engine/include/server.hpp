#ifndef SERVER_HPP
#define SERVER_HPP

#include "network.hpp"
#include "engine.hpp"
#include <thread>
#include <memory>
#include <map>
#include <vector>

class GameServer : public NetworkBase {
private:
    int tcp_port;
    int udp_port;
    int tcp_socket;
    int udp_socket;
    
    std::vector<Planet> planets;
    std::mutex planets_mutex;
    
    // Клиенты (IP:PORT -> последнее время обновления)
    std::map<std::string, time_t> clients;
    std::mutex clients_mutex;
    
    std::thread tcp_thread;
    std::thread udp_thread;
    std::thread simulation_thread;
    
    // JSON конфигурация планет
    std::string planets_json;
    
    // Физические константы
    const float G = 100.0f;
    const float timeStep = 0.016f;
    const float FRICTION_COEFFICIENT = 0.08f;
    
public:
    GameServer(int port = DEFAULT_PORT);
    ~GameServer();
    
    bool start();
    void stop();
    bool isRunning() const { return running; }
    
    void setPlanets(const std::vector<Planet>& planets);
    void updatePlanets(const std::vector<Planet>& planets);
    
    int getTcpPort() const { return tcp_port; }
    int getUdpPort() const { return udp_port; }
    
    std::vector<Planet> getPlanets() const { return planets; }
    
private:
    void runTcpServer();
    void runUdpServer();
    void runSimulation();
    
    void handleTcpClient(int client_socket, const std::string& client_ip);
    void sendPlanetsJson(int client_socket);
    
    void broadcastUdpUpdate();
    std::string serializePlanetsForUdp();
    
    void loadPlanetsFromJson();
    void createDefaultPlanets();
    
    // Физика (скопировано из вашего кода)
    void applyGravity();
    void applyCollision();
};

#endif // SERVER_HPP