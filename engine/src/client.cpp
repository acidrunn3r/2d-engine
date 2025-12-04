#include "../include/client.hpp"
#include <iostream>
#include <sstream>
#include <chrono>
#include <cstring>

GameClient::GameClient() 
    : tcp_port(0), udp_port(0), 
      tcp_socket(-1), udp_socket(-1),
      max_trajectory_points(200),
      is_connected(false), is_host(false) {
    
    trajectories.clear();
}

GameClient::~GameClient() {
    disconnect();
}

bool GameClient::connect(const std::string& ip, int port, bool as_host) {
    if (is_connected) {
        disconnect();
    }
    
    server_ip = ip;
    tcp_port = port;
    udp_port = port + 1;
    is_host = as_host;
    
    if (!setupTcpConnection()) {
        std::cerr << "Failed to establish TCP connection" << std::endl;
        return false;
    }
    
    if (!setupUdpConnection()) {
        std::cerr << "Failed to establish UDP connection" << std::endl;
        close(tcp_socket);
        return false;
    }
    
    is_connected = true;
    
    receive_thread = std::thread(&GameClient::receiveTcpData, this);
    udp_thread = std::thread(&GameClient::receiveUdpData, this);
    
    if (connection_callback) {
        connection_callback(true);
    }
    
    std::cout << "Connected to server " << ip << ":" << port 
              << " (Host: " << (as_host ? "Yes" : "No") << ")" << std::endl;
    
    return true;
}

void GameClient::disconnect() {
    if (!is_connected) return;
    
    is_connected = false;
    
    if (udp_socket >= 0) {
        std::string disconnect_msg = "DISCONNECT";
        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(udp_port);
        inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);
        
        sendto(udp_socket, disconnect_msg.c_str(), disconnect_msg.length(), getSendFlags(),
               (struct sockaddr*)&server_addr, sizeof(server_addr));
    }
    
    if (receive_thread.joinable()) receive_thread.join();
    if (udp_thread.joinable()) udp_thread.join();
    
    if (tcp_socket >= 0) {
        close(tcp_socket);
        tcp_socket = -1;
    }
    if (udp_socket >= 0) {
        close(udp_socket);
        udp_socket = -1;
    }
    
    {
        std::lock_guard<std::mutex> lock(planets_mutex);
        planets.clear();
    }
    
    clearTrajectories();
    
    if (connection_callback) {
        connection_callback(false);
    }
    
    std::cout << "Disconnected from server" << std::endl;
}

bool GameClient::setupTcpConnection() {
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket < 0) {
        std::cerr << "Failed to create TCP socket" << std::endl;
        return false;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(tcp_port);
    
    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "Invalid server IP address" << std::endl;
        return false;
    }
    
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(tcp_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    if (::connect(tcp_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "TCP connection failed: " << strerror(errno) << std::endl;
        return false;
    }
    
    return true;
}

bool GameClient::setupUdpConnection() {
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        std::cerr << "Failed to create UDP socket" << std::endl;
        return false;
    }
    
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = 0;
    
    if (bind(udp_socket, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
        std::cerr << "Failed to bind UDP socket" << std::endl;
        return false;
    }
    
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    return true;
}

void GameClient::receiveTcpData() {
    uint32_t json_size;
    int bytes_received = recv(tcp_socket, (char*)&json_size, sizeof(json_size), 0);
    
    if (bytes_received != sizeof(json_size)) {
        std::cerr << "Failed to receive JSON size" << std::endl;
        disconnect();
        return;
    }
    
    json_size = ntohl(json_size);
    
    if (json_size == 0 || json_size > 10 * 1024 * 1024) {
        std::cerr << "Invalid JSON size: " << json_size << std::endl;
        disconnect();
        return;
    }
    
    std::vector<char> json_buffer(json_size + 1);
    size_t total_received = 0;
    
    while (total_received < json_size) {
        int received = recv(tcp_socket, json_buffer.data() + total_received, 
                           json_size - total_received, 0);
        
        if (received <= 0) {
            std::cerr << "Failed to receive JSON data" << std::endl;
            disconnect();
            return;
        }
        
        total_received += received;
    }
    
    json_buffer[total_received] = '\0';
    std::string json_data(json_buffer.data());
    
    std::cout << "Received planets JSON (" << total_received << " bytes)" << std::endl;
    parsePlanetsJson(json_data);
    
    close(tcp_socket);
    tcp_socket = -1;
}

void GameClient::parsePlanetsJson(const std::string& json) {
    // Простой парсер JSON (как в сервере)
    std::vector<PlanetData> new_planets;
    std::istringstream iss(json);
    std::string line;
    
    while (std::getline(iss, line)) {
        if (line.find("\"x\"") != std::string::npos) {
            PlanetData pd;
            float x = 0, y = 0, radius = 0, mass = 0, vx = 0, vy = 0;
            int r = 100, g = 150, b = 255;
            
            while (std::getline(iss, line) && line.find("}") == std::string::npos) {
                size_t colon = line.find(':');
                if (colon != std::string::npos) {
                    std::string key = line.substr(0, colon);
                    std::string value = line.substr(colon + 1);
                    
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
                pd.id = new_planets.size();
                pd.position = sf::Vector2f(x, y);
                pd.velocity = sf::Vector2f(vx, vy);
                pd.radius = radius;
                pd.mass = mass;
                pd.color = sf::Color(r, g, b);
                new_planets.push_back(pd);
            }
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(planets_mutex);
        planets = new_planets;
    }
    
    trajectories.resize(new_planets.size());
    
    std::cout << "Parsed " << new_planets.size() << " planets from JSON" << std::endl;
    
    if (planets_callback) {
        planets_callback(new_planets);
    }
}

void GameClient::receiveUdpData() {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr;
    socklen_t server_len = sizeof(server_addr);
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(udp_port);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);
    
    std::string heartbeat = "HEARTBEAT";
    sendto(udp_socket, heartbeat.c_str(), heartbeat.length(), getSendFlags(),
           (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    auto last_heartbeat = std::chrono::steady_clock::now();
    
    while (is_connected) {
        int bytes_received = recvfrom(udp_socket, buffer, sizeof(buffer) - 1, 0,
                                      (struct sockaddr*)&server_addr, &server_len);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string data(buffer);
            
            if (data.find("UPDATE:") == 0) {
                parseUdpUpdate(data);
            }
        }
        
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_heartbeat).count() >= 5) {
            sendto(udp_socket, heartbeat.c_str(), heartbeat.length(), getSendFlags(),
                   (struct sockaddr*)&server_addr, sizeof(server_addr));
            last_heartbeat = now;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void GameClient::parseUdpUpdate(const std::string& data) {
    std::string update_data = data.substr(7);
    
    std::vector<PlanetData> current_planets;
    {
        std::lock_guard<std::mutex> lock(planets_mutex);
        current_planets = planets;
    }
    
    std::istringstream ss(update_data);
    std::string planet_str;
    
    std::getline(ss, planet_str, ';'); // Пропускаем количество
    
    while (std::getline(ss, planet_str, ';')) {
        if (planet_str.empty()) continue;
        
        std::istringstream planet_ss(planet_str);
        std::string token;
        std::vector<std::string> tokens;
        
        while (std::getline(planet_ss, token, ',')) {
            tokens.push_back(token);
        }
        
        if (tokens.size() >= 5) {
            int id = std::stoi(tokens[0]);
            float x = std::stof(tokens[1]);
            float y = std::stof(tokens[2]);
            float vx = std::stof(tokens[3]);
            float vy = std::stof(tokens[4]);
            
            size_t unsigned_id = static_cast<size_t>(id);
            if (id >= 0 && unsigned_id < current_planets.size()) {
                current_planets[id].position = sf::Vector2f(x, y);
                current_planets[id].velocity = sf::Vector2f(vx, vy);
                
                if (unsigned_id < trajectories.size()) {
                    sf::Color trail_color = current_planets[unsigned_id].color;
                    trail_color.a = 150;
                    
                    trajectories[unsigned_id].push_back(sf::Vertex(sf::Vector2f(x, y), trail_color));
                    
                    if (trajectories[unsigned_id].size() > static_cast<size_t>(max_trajectory_points)) {
                        trajectories[unsigned_id].erase(trajectories[unsigned_id].begin());
                    }
                }
            }
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(planets_mutex);
        planets = current_planets;
    }
}

std::vector<PlanetData> GameClient::getPlanets() {
    std::lock_guard<std::mutex> lock(planets_mutex);
    return planets;
}

std::vector<std::vector<sf::Vertex>> GameClient::getTrajectories() const {
    return trajectories;
}

void GameClient::setPlanetsUpdateCallback(PlanetsUpdateCallback callback) {
    planets_callback = callback;
}

void GameClient::setConnectionCallback(ConnectionCallback callback) {
    connection_callback = callback;
}

void GameClient::clearTrajectories() {
    for (auto& trajectory : trajectories) {
        trajectory.clear();
    }
}