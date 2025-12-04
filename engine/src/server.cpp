#include "../include/server.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <cstring>
#include <cmath>

GameServer::GameServer(int port) 
    : tcp_port(port), 
      udp_port(port + 1),
      tcp_socket(-1), 
      udp_socket(-1) {
    
    loadPlanetsFromJson();
}

GameServer::~GameServer() {
    stop();
}

void GameServer::loadPlanetsFromJson() {
    std::ifstream file("assets/planets.json");
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        planets_json = buffer.str();
        file.close();
        
        // Создаем планеты из JSON (простейший парсер)
        std::istringstream iss(planets_json);
        std::string line;
        planets.clear();
        
        while (std::getline(iss, line)) {
            if (line.find("\"x\"") != std::string::npos) {
                // Парсим планету
                Planet p(0, 0);
                float x = 0, y = 0, radius = 0, mass = 0, vx = 0, vy = 0;
                int r = 100, g = 150, b = 255; // Цвет по умолчанию
                
                while (std::getline(iss, line) && line.find("}") == std::string::npos) {
                    size_t colon = line.find(':');
                    if (colon != std::string::npos) {
                        std::string key = line.substr(0, colon);
                        std::string value = line.substr(colon + 1);
                        
                        // Убираем пробелы и запятые
                        key.erase(std::remove(key.begin(), key.end(), ' '), key.end());
                        key.erase(std::remove(key.begin(), key.end(), '\"'), key.end());
                        value.erase(std::remove(value.begin(), value.end(), ' '), value.end());
                        value.erase(std::remove(value.begin(), value.end(), ','), value.end());
                        value.erase(std::remove(value.begin(), value.end(), '['), value.end());
                        value.erase(std::remove(value.begin(), value.end(), ']'), value.end());
                        
                        if (key == "x") x = std::stof(value);
                        else if (key == "y") y = std::stof(value);
                        else if (key == "radius") radius = std::stof(value);
                        else if (key == "mass") mass = std::stof(value);
                        else if (key == "velocity") {
                            size_t comma = value.find(',');
                            if (comma != std::string::npos) {
                                vx = std::stof(value.substr(0, comma));
                                vy = std::stof(value.substr(comma + 1));
                            }
                        }
                        else if (key == "color") {
                            size_t comma1 = value.find(',');
                            size_t comma2 = value.find(',', comma1 + 1);
                            if (comma1 != std::string::npos && comma2 != std::string::npos) {
                                r = std::stoi(value.substr(0, comma1));
                                g = std::stoi(value.substr(comma1 + 1, comma2 - comma1 - 1));
                                b = std::stoi(value.substr(comma2 + 1));
                            }
                        }
                    }
                }
                
                if (radius > 0 && mass > 0) {
                    p = Planet(radius, mass);
                    p.setPosition(sf::Vector2f(x, y));
                    p.setVelocity(sf::Vector2f(vx, vy));
                    p.setColor(r, g, b);
                    planets.push_back(p);
                }
            }
        }
        
        std::cout << "Loaded " << planets.size() << " planets from assets/planets.json" << std::endl;
    } else {
        std::cerr << "Could not open assets/planets.json" << std::endl;
        createDefaultPlanets();
    }
}

void GameServer::createDefaultPlanets() {
    planets.clear();
    
    Planet p1(50.0f, 400000.0f);
    p1.setPosition(sf::Vector2f(1050, 540));
    p1.setVelocity(sf::Vector2f(0, 0));
    p1.setColor(28, 100, 255);
    
    Planet p2(20.0f, 200.0f);
    p2.setPosition(sf::Vector2f(600, 400));
    p2.setVelocity(sf::Vector2f(50, -50));
    p2.setColor(200, 200, 200);
    
    planets.push_back(p1);
    planets.push_back(p2);
    
    // Простой JSON для отправки
    std::stringstream json;
    json << "{\n";
    json << "  \"Planets\": [\n";
    json << "    {\n";
    json << "      \"x\": 1050,\n";
    json << "      \"y\": 540,\n";
    json << "      \"radius\": 50,\n";
    json << "      \"mass\": 400000,\n";
    json << "      \"velocity\": [0, 0],\n";
    json << "      \"color\": [28, 100, 255]\n";
    json << "    },\n";
    json << "    {\n";
    json << "      \"x\": 600,\n";
    json << "      \"y\": 400,\n";
    json << "      \"radius\": 20,\n";
    json << "      \"mass\": 200,\n";
    json << "      \"velocity\": [50, -50],\n";
    json << "      \"color\": [200, 200, 200]\n";
    json << "    }\n";
    json << "  ]\n";
    json << "}";
    
    planets_json = json.str();
    std::cout << "Created 2 default planets" << std::endl;
}

bool GameServer::start() {
    if (running) {
        return true;
    }
    
    // TCP сокет
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket < 0) {
        std::cerr << "Failed to create TCP socket" << std::endl;
        return false;
    }
    
    int opt = 1;
    if (setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        close(tcp_socket);
        return false;
    }
    
    struct sockaddr_in tcp_addr;
    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    tcp_addr.sin_port = htons(tcp_port);
    
    if (bind(tcp_socket, (struct sockaddr*)&tcp_addr, sizeof(tcp_addr)) < 0) {
        std::cerr << "TCP bind failed on port " << tcp_port << ": " 
                  << strerror(errno) << std::endl;
        close(tcp_socket);
        return false;
    }
    
    if (listen(tcp_socket, 5) < 0) {
        std::cerr << "TCP listen failed" << std::endl;
        close(tcp_socket);
        return false;
    }
    
    // UDP сокет
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        std::cerr << "Failed to create UDP socket" << std::endl;
        close(tcp_socket);
        return false;
    }
    
    struct sockaddr_in udp_addr;
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    udp_addr.sin_port = htons(udp_port);
    
    if (bind(udp_socket, (struct sockaddr*)&udp_addr, sizeof(udp_addr)) < 0) {
        std::cerr << "UDP bind failed on port " << udp_port << ": " 
                  << strerror(errno) << std::endl;
        close(tcp_socket);
        close(udp_socket);
        return false;
    }
    
    setSocketNonBlocking(udp_socket);
    
    running = true;
    
    tcp_thread = std::thread(&GameServer::runTcpServer, this);
    udp_thread = std::thread(&GameServer::runUdpServer, this);
    simulation_thread = std::thread(&GameServer::runSimulation, this);
    
    std::cout << "Server started:" << std::endl;
    std::cout << "  TCP: " << tcp_port << " (initial connection)" << std::endl;
    std::cout << "  UDP: " << udp_port << " (position updates)" << std::endl;
    std::cout << "  IP: " << getLocalIP() << std::endl;
    
    return true;
}

void GameServer::stop() {
    if (!running) return;
    
    running = false;
    
    if (tcp_socket >= 0) {
        close(tcp_socket);
        tcp_socket = -1;
    }
    if (udp_socket >= 0) {
        close(udp_socket);
        udp_socket = -1;
    }
    
    if (tcp_thread.joinable()) tcp_thread.join();
    if (udp_thread.joinable()) udp_thread.join();
    if (simulation_thread.joinable()) simulation_thread.join();
    
    std::cout << "Server stopped" << std::endl;
}

void GameServer::runTcpServer() {
    std::cout << "TCP server thread started" << std::endl;
    
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(tcp_socket, 
                                  (struct sockaddr*)&client_addr, 
                                  &client_len);
        
        if (client_socket < 0) {
            if (running) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }
        
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        
        std::cout << "New client: " << client_ip << ":" << ntohs(client_addr.sin_port) << std::endl;
        
        std::thread client_thread(&GameServer::handleTcpClient, this, 
                                 client_socket, std::string(client_ip));
        client_thread.detach();
    }
}

void GameServer::handleTcpClient(int client_socket, const std::string& client_ip) {
    sendPlanetsJson(client_socket);
    
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        std::string client_key = client_ip + ":" + std::to_string(udp_port);
        clients[client_key] = time(nullptr);
    }
    
    close(client_socket);
    
    std::cout << "Sent planets JSON to " << client_ip << std::endl;
}

void GameServer::sendPlanetsJson(int client_socket) {
    if (planets_json.empty()) {
        return;
    }
    
    // Отправляем размер JSON
    uint32_t json_size = planets_json.size();
    uint32_t network_size = htonl(json_size);
    send(client_socket, &network_size, sizeof(network_size), 0);
    
    // Отправляем JSON
    send(client_socket, planets_json.c_str(), planets_json.size(), 0);
}

void GameServer::runUdpServer() {
    std::cout << "UDP server thread started" << std::endl;
    
    char buffer[BUFFER_SIZE];
    
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int bytes_received = recvfrom(udp_socket, buffer, sizeof(buffer) - 1, 0,
                                     (struct sockaddr*)&client_addr, &client_len);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string message(buffer);
            
            if (message == "HEARTBEAT") {
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
                
                std::lock_guard<std::mutex> lock(clients_mutex);
                std::string client_key = std::string(client_ip) + ":" + 
                                        std::to_string(ntohs(client_addr.sin_port));
                clients[client_key] = time(nullptr);
            }
        }
        
        broadcastUdpUpdate();
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }
}

void GameServer::broadcastUdpUpdate() {
    std::string update_data = serializePlanetsForUdp();
    if (update_data.empty()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(clients_mutex);
    auto it = clients.begin();
    
    while (it != clients.end()) {
        if (time(nullptr) - it->second > 30) {
            std::cout << "Removing inactive client: " << it->first << std::endl;
            it = clients.erase(it);
            continue;
        }
        
        size_t colon_pos = it->first.find(':');
        if (colon_pos != std::string::npos) {
            std::string ip = it->first.substr(0, colon_pos);
            int port = std::stoi(it->first.substr(colon_pos + 1));
            
            struct sockaddr_in client_addr;
            memset(&client_addr, 0, sizeof(client_addr));
            client_addr.sin_family = AF_INET;
            client_addr.sin_port = htons(port);
            inet_pton(AF_INET, ip.c_str(), &client_addr.sin_addr);
            
            sendto(udp_socket, update_data.c_str(), update_data.length(), getSendFlags(),
                   (struct sockaddr*)&client_addr, sizeof(client_addr));
        }
        
        ++it;
    }
}

std::string GameServer::serializePlanetsForUdp() {
    std::lock_guard<std::mutex> lock(planets_mutex);
    
    std::stringstream ss;
    ss << "UPDATE:" << planets.size() << ";";
    
    for (size_t i = 0; i < planets.size(); ++i) {
        auto pos = planets[i].getPosition();
        auto vel = planets[i].getVelocity();
        
        ss << i << ","
           << pos.x << "," << pos.y << ","
           << vel.x << "," << vel.y;
        
        if (i < planets.size() - 1) {
            ss << ";";
        }
    }
    
    return ss.str();
}

void GameServer::applyGravity() {
    for (size_t i = 0; i < planets.size(); i++) {
        sf::Vector2f totalForce(0, 0);
        
        for (size_t j = 0; j < planets.size(); j++) {
            if (i == j) continue;
            
            sf::Vector2f direction = planets[j].getPosition() - planets[i].getPosition();
            float distance = std::sqrt(direction.x * direction.x + direction.y * direction.y);
            
            if (distance < 1.0f) continue;
            
            direction /= distance;
            float forceMagnitude = G * planets[i].getMass() * planets[j].getMass() / 
                                   (distance * distance);
            totalForce += direction * forceMagnitude;
        }
        
        sf::Vector2f newAcceleration = totalForce / planets[i].getMass();
        planets[i].setAcceleration(newAcceleration);
        
        sf::Vector2f newVelocity = planets[i].getVelocity() + newAcceleration * timeStep;
        planets[i].setVelocity(newVelocity);
        
        sf::Vector2f newPosition = planets[i].getPosition() + newVelocity * timeStep;
        planets[i].setPosition(newPosition);
    }
}

void GameServer::applyCollision() {
    for (size_t i = 0; i < planets.size(); i++) {
        for (size_t j = i + 1; j < planets.size(); j++) {
            if (planets[i].isColliding(planets[j])) {
                sf::Vector2f collisionVector = planets[j].getPosition() - planets[i].getPosition();
                float distance = std::sqrt(collisionVector.x * collisionVector.x + 
                                          collisionVector.y * collisionVector.y);
                
                if (distance > 0) {
                    collisionVector /= distance;
                    
                    float m1 = planets[i].getMass(), m2 = planets[j].getMass();
                    float totalMass = m1 + m2;
                    float ratio1 = m2 / totalMass;
                    float ratio2 = m1 / totalMass;
                    
                    float overlap = (planets[i].getRadius() + planets[j].getRadius()) - distance;
                    planets[i].setPosition(planets[i].getPosition() - collisionVector * overlap * ratio1);
                    planets[j].setPosition(planets[j].getPosition() + collisionVector * overlap * ratio2);
                    
                    sf::Vector2f v1 = planets[i].getVelocity();
                    sf::Vector2f v2 = planets[j].getVelocity();
                    
                    float v1n = v1.x * collisionVector.x + v1.y * collisionVector.y;
                    float v2n = v2.x * collisionVector.x + v2.y * collisionVector.y;
                    
                    float restitution = 0.0f;
                    float u1n = ((m1 - restitution * m2) * v1n + (1 + restitution) * m2 * v2n) / (m1 + m2);
                    float u2n = ((m2 - restitution * m1) * v2n + (1 + restitution) * m1 * v1n) / (m1 + m2);
                    
                    planets[i].setVelocity(v1 + collisionVector * (u1n - v1n));
                    planets[j].setVelocity(v2 + collisionVector * (u2n - v2n));
                    
                    sf::Vector2f tangent(-collisionVector.y, collisionVector.x);
                    
                    float v1t = v1.x * tangent.x + v1.y * tangent.y;
                    float v2t = v2.x * tangent.x + v2.y * tangent.y;
                    
                    float relativeTangentialSpeed = std::abs(v1t - v2t);
                    
                    if (relativeTangentialSpeed > 0.1f) {
                        v1t *= (1.0f - FRICTION_COEFFICIENT);
                        v2t *= (1.0f - FRICTION_COEFFICIENT);
                        
                        float new_v1n = planets[i].getVelocity().x * collisionVector.x +
                                        planets[i].getVelocity().y * collisionVector.y;
                        float new_v2n = planets[j].getVelocity().x * collisionVector.x +
                                        planets[j].getVelocity().y * collisionVector.y;
                        
                        planets[i].setVelocity(collisionVector * new_v1n + tangent * v1t);
                        planets[j].setVelocity(collisionVector * new_v2n + tangent * v2t);
                    }
                }
            }
        }
    }
}

void GameServer::runSimulation() {
    std::cout << "Simulation thread started" << std::endl;
    
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        
        std::lock_guard<std::mutex> lock(planets_mutex);
        applyGravity();
        applyCollision();
    }
}