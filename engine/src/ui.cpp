#include "../include/ui.hpp"
#include <iostream>
#include <cctype>

UserInterface::UserInterface() 
    : current_state(State::MAIN_MENU),
      active_field(ActiveField::NONE),
      font_loaded(false) {
    
    host_port_input = "5555";
    join_ip_input = "127.0.0.1";
    join_port_input = "5555";
    
    if (!font.loadFromFile("assets/arial.ttf")) {
        if (!font.loadFromFile("/System/Library/Fonts/Helvetica.ttc") &&
            !font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
            std::cerr << "Failed to load font" << std::endl;
        } else {
            font_loaded = true;
        }
    } else {
        font_loaded = true;
    }
}

void UserInterface::setExitCallback(std::function<void()> callback) {
    on_exit = callback;
}

void UserInterface::handleEvent(const sf::Event& event) {
    switch (current_state) {
        case State::MAIN_MENU:
            handleMainMenuEvent(event);
            break;
        case State::HOST_MENU:
            handleHostMenuEvent(event);
            break;
        case State::JOIN_MENU:
            handleJoinMenuEvent(event);
            break;
        case State::CONNECTED:
            handleConnectedEvent(event);
            break;
        default:
            break;
    }
}

void UserInterface::handleMainMenuEvent(const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed) {
        sf::Vector2f mouse_pos(event.mouseButton.x, event.mouseButton.y);
        
        if (mouse_pos.x >= 300 && mouse_pos.x <= 500 &&
            mouse_pos.y >= 200 && mouse_pos.y <= 250) {
            current_state = State::HOST_MENU;
        }
        else if (mouse_pos.x >= 300 && mouse_pos.x <= 500 &&
                 mouse_pos.y >= 300 && mouse_pos.y <= 350) {
            current_state = State::JOIN_MENU;
        }
        else if (mouse_pos.x >= 300 && mouse_pos.x <= 500 &&
                 mouse_pos.y >= 400 && mouse_pos.y <= 450) {
            if (on_exit) {
                on_exit();
            }
        }
    }
}

void UserInterface::handleHostMenuEvent(const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed) {
        sf::Vector2f mouse_pos(event.mouseButton.x, event.mouseButton.y);
        
        if (mouse_pos.x >= 250 && mouse_pos.x <= 550 &&
            mouse_pos.y >= 200 && mouse_pos.y <= 240) {
            active_field = ActiveField::HOST_PORT;
        }
        else if (mouse_pos.x >= 250 && mouse_pos.x <= 350 &&
                 mouse_pos.y >= 300 && mouse_pos.y <= 340) {
            current_state = State::MAIN_MENU;
            active_field = ActiveField::NONE;
        }
        else if (mouse_pos.x >= 400 && mouse_pos.x <= 500 &&
                 mouse_pos.y >= 300 && mouse_pos.y <= 340) {
            startAsHost();
        }
        else {
            active_field = ActiveField::NONE;
        }
    }
    else if (event.type == sf::Event::TextEntered && 
             active_field == ActiveField::HOST_PORT) {
        if (event.text.unicode == '\b' && !host_port_input.empty()) {
            host_port_input.pop_back();
        }
        else if (event.text.unicode >= '0' && event.text.unicode <= '9') {
            if (host_port_input.length() < 5) {
                host_port_input += static_cast<char>(event.text.unicode);
            }
        }
    }
}

void UserInterface::handleJoinMenuEvent(const sf::Event& event) {
    if (event.type == sf::Event::MouseButtonPressed) {
        sf::Vector2f mouse_pos(event.mouseButton.x, event.mouseButton.y);
        
        if (mouse_pos.x >= 250 && mouse_pos.x <= 550 &&
            mouse_pos.y >= 150 && mouse_pos.y <= 190) {
            active_field = ActiveField::JOIN_IP;
        }
        else if (mouse_pos.x >= 250 && mouse_pos.x <= 550 &&
                 mouse_pos.y >= 220 && mouse_pos.y <= 260) {
            active_field = ActiveField::JOIN_PORT;
        }
        else if (mouse_pos.x >= 250 && mouse_pos.x <= 350 &&
                 mouse_pos.y >= 320 && mouse_pos.y <= 360) {
            current_state = State::MAIN_MENU;
            active_field = ActiveField::NONE;
        }
        else if (mouse_pos.x >= 400 && mouse_pos.x <= 500 &&
                 mouse_pos.y >= 320 && mouse_pos.y <= 360) {
            startAsClient();
        }
        else {
            active_field = ActiveField::NONE;
        }
    }
    else if (event.type == sf::Event::TextEntered) {
        if (active_field == ActiveField::JOIN_IP) {
            if (event.text.unicode == '\b' && !join_ip_input.empty()) {
                join_ip_input.pop_back();
            }
            else if (event.text.unicode == '.' || 
                    (event.text.unicode >= '0' && event.text.unicode <= '9')) {
                join_ip_input += static_cast<char>(event.text.unicode);
            }
        }
        else if (active_field == ActiveField::JOIN_PORT) {
            if (event.text.unicode == '\b' && !join_port_input.empty()) {
                join_port_input.pop_back();
            }
            else if (event.text.unicode >= '0' && event.text.unicode <= '9') {
                if (join_port_input.length() < 5) {
                    join_port_input += static_cast<char>(event.text.unicode);
                }
            }
        }
    }
}

void UserInterface::handleConnectedEvent(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        disconnect();
    }
}

void UserInterface::update() {
    if (client && client->isConnected()) {
        current_state = State::CONNECTED;
    }
}

void UserInterface::draw(sf::RenderWindow& window) {
    switch (current_state) {
        case State::MAIN_MENU:
            drawMainMenu(window);
            break;
        case State::HOST_MENU:
            drawHostMenu(window);
            break;
        case State::JOIN_MENU:
            drawJoinMenu(window);
            break;
        case State::CONNECTING:
            drawConnecting(window);
            break;
        case State::CONNECTED:
            drawConnected(window);
            break;
        case State::ERROR:
            drawError(window);
            break;
    }
}

void UserInterface::drawMainMenu(sf::RenderWindow& window) {
    if (font_loaded) {
        sf::Text title("Planet Simulation Network", font, 40);
        title.setFillColor(sf::Color::White);
        title.setPosition(200, 100);
        window.draw(title);
    }
    
    drawButton(window, "Host Simulation", 300, 200, 200, 50);
    drawButton(window, "Join Simulation", 300, 300, 200, 50);
    drawButton(window, "Exit", 300, 400, 200, 50);
}

void UserInterface::drawHostMenu(sf::RenderWindow& window) {
    if (font_loaded) {
        sf::Text title("Host Simulation", font, 40);
        title.setFillColor(sf::Color::White);
        title.setPosition(250, 100);
        window.draw(title);
    }
    
    drawTextField(window, "Port:", host_port_input, 250, 200, 300, 40, 
                  active_field == ActiveField::HOST_PORT);
    
    drawButton(window, "Back", 250, 300, 100, 40);
    drawButton(window, "Start Server", 400, 300, 100, 40);
}

void UserInterface::drawJoinMenu(sf::RenderWindow& window) {
    if (font_loaded) {
        sf::Text title("Join Simulation", font, 40);
        title.setFillColor(sf::Color::White);
        title.setPosition(250, 50);
        window.draw(title);
    }
    
    drawTextField(window, "Server IP:", join_ip_input, 250, 150, 300, 40,
                  active_field == ActiveField::JOIN_IP);
    drawTextField(window, "Port:", join_port_input, 250, 220, 300, 40,
                  active_field == ActiveField::JOIN_PORT);
    
    drawButton(window, "Back", 250, 320, 100, 40);
    drawButton(window, "Connect", 400, 320, 100, 40);
}

void UserInterface::drawConnecting(sf::RenderWindow& window) {
    if (font_loaded) {
        sf::Text text("Connecting...", font, 30);
        text.setFillColor(sf::Color::White);
        text.setPosition(300, 250);
        window.draw(text);
    }
}

void UserInterface::drawConnected(sf::RenderWindow& window) {
    (void)window;
}

void UserInterface::drawError(sf::RenderWindow& window) {
    if (font_loaded) {
        sf::Text text("Error: " + error_message, font, 24);
        text.setFillColor(sf::Color::Red);
        text.setPosition(100, 250);
        window.draw(text);
        
        sf::Text back_text("Press any key to return", font, 20);
        back_text.setFillColor(sf::Color::White);
        back_text.setPosition(250, 350);
        window.draw(back_text);
    }
}

void UserInterface::drawButton(sf::RenderWindow& window, const std::string& text, 
                               float x, float y, float width, float height, bool hover) {
    sf::RectangleShape button(sf::Vector2f(width, height));
    button.setPosition(x, y);
    button.setFillColor(hover ? sf::Color(100, 100, 200) : sf::Color(60, 60, 120));
    button.setOutlineThickness(2);
    button.setOutlineColor(sf::Color::White);
    window.draw(button);
    
    if (font_loaded) {
        sf::Text button_text(text, font, 20);
        button_text.setFillColor(sf::Color::White);
        
        sf::FloatRect bounds = button_text.getLocalBounds();
        button_text.setPosition(x + (width - bounds.width) / 2,
                               y + (height - bounds.height) / 2 - 5);
        window.draw(button_text);
    }
}

void UserInterface::drawTextField(sf::RenderWindow& window, const std::string& label, 
                                  const std::string& value, float x, float y, 
                                  float width, float height, bool active) {
    if (font_loaded) {
        sf::Text label_text(label, font, 18);
        label_text.setFillColor(sf::Color::White);
        label_text.setPosition(x, y - 25);
        window.draw(label_text);
    }
    
    sf::RectangleShape field(sf::Vector2f(width, height));
    field.setPosition(x, y);
    field.setFillColor(active ? sf::Color(80, 80, 80) : sf::Color(60, 60, 60));
    field.setOutlineThickness(2);
    field.setOutlineColor(active ? sf::Color::Yellow : sf::Color::White);
    window.draw(field);
    
    if (font_loaded) {
        sf::Text value_text(value, font, 18);
        value_text.setFillColor(sf::Color::White);
        value_text.setPosition(x + 5, y + 5);
        window.draw(value_text);
        
        if (active) {
            sf::Text cursor("|", font, 18);
            cursor.setFillColor(sf::Color::White);
            cursor.setPosition(x + 5 + value_text.getLocalBounds().width, y + 5);
            window.draw(cursor);
        }
    }
}

void UserInterface::startAsHost() {
    try {
        int port = std::stoi(host_port_input);
        
        if (port < 1 || port > 65535) {
            error_message = "Invalid port number";
            current_state = State::ERROR;
            return;
        }
        
        server = std::make_unique<GameServer>(port);
        
        if (!server->start()) {
            error_message = "Failed to start server";
            current_state = State::ERROR;
            server.reset();
            return;
        }
        
        client = std::make_unique<GameClient>();
        client->setConnectionCallback([this](bool connected) {
            onClientConnected(connected);
        });
        
        if (!client->connect("127.0.0.1", port, true)) {
            error_message = "Failed to connect to local server";
            current_state = State::ERROR;
            server->stop();
            server.reset();
            client.reset();
            return;
        }
        
        current_state = State::CONNECTING;
        
    } catch (const std::exception& e) {
        error_message = "Invalid input: " + std::string(e.what());
        current_state = State::ERROR;
    }
}

void UserInterface::startAsClient() {
    try {
        int port = std::stoi(join_port_input);
        
        if (port < 1 || port > 65535) {
            error_message = "Invalid port number";
            current_state = State::ERROR;
            return;
        }
        
        client = std::make_unique<GameClient>();
        client->setConnectionCallback([this](bool connected) {
            onClientConnected(connected);
        });
        
        if (!client->connect(join_ip_input, port, false)) {
            error_message = "Failed to connect to server";
            current_state = State::ERROR;
            client.reset();
            return;
        }
        
        current_state = State::CONNECTING;
        
    } catch (const std::exception& e) {
        error_message = "Invalid input: " + std::string(e.what());
        current_state = State::ERROR;
    }
}

void UserInterface::disconnect() {
    if (client) {
        client->disconnect();
        client.reset();
    }
    
    if (server) {
        server->stop();
        server.reset();
    }
    
    current_state = State::MAIN_MENU;
    active_field = ActiveField::NONE;
}

void UserInterface::onClientConnected(bool connected) {
    if (!connected) {
        error_message = "Connection failed";
        current_state = State::ERROR;
        
        if (client) {
            client.reset();
        }
        if (server) {
            server->stop();
            server.reset();
        }
    }
}

bool UserInterface::isConnected() const {
    return client && client->isConnected();
}

bool UserInterface::isHost() const {
    return client && client->isHost();
}

std::vector<PlanetData> UserInterface::getPlanets() {
    if (client) {
        return client->getPlanets();
    }
    return {};
}

std::vector<std::vector<sf::Vertex>> UserInterface::getTrajectories() {
    if (client) {
        return client->getTrajectories();
    }
    return {};
}