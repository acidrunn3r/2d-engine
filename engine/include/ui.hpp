#ifndef UI_HPP
#define UI_HPP

#include "client.hpp"
#include "server.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Font.hpp>
#include <string>
#include <functional>

class UserInterface {
private:
    enum class State {
        MAIN_MENU,
        HOST_MENU,
        JOIN_MENU,
        CONNECTING,
        CONNECTED,
        ERROR
    };
    
    State current_state;
    std::string error_message;
    
    // Текстовые поля
    std::string host_port_input;
    std::string join_ip_input;
    std::string join_port_input;
    
    // Активное текстовое поле
    enum class ActiveField {
        NONE,
        HOST_PORT,
        JOIN_IP,
        JOIN_PORT
    };
    
    ActiveField active_field;
    
    // Шрифты
    sf::Font font;
    bool font_loaded;
    
    // Сеть
    std::unique_ptr<GameServer> server;
    std::unique_ptr<GameClient> client;
    
    // Колбэки
    std::function<void()> on_exit;
    
public:
    UserInterface();
    
    void setExitCallback(std::function<void()> callback);
    void handleEvent(const sf::Event& event);
    void update();
    void draw(sf::RenderWindow& window);
    
    bool isConnected() const;
    bool isHost() const;

    void disconnect();
    
    std::vector<PlanetData> getPlanets();
    std::vector<std::vector<sf::Vertex>> getTrajectories();
    
private:
    void handleMainMenuEvent(const sf::Event& event);
    void handleHostMenuEvent(const sf::Event& event);
    void handleJoinMenuEvent(const sf::Event& event);
    void handleConnectedEvent(const sf::Event& event);
    
    void drawMainMenu(sf::RenderWindow& window);
    void drawHostMenu(sf::RenderWindow& window);
    void drawJoinMenu(sf::RenderWindow& window);
    void drawConnecting(sf::RenderWindow& window);
    void drawConnected(sf::RenderWindow& window);
    void drawError(sf::RenderWindow& window);
    
    void drawButton(sf::RenderWindow& window, const std::string& text, 
                    float x, float y, float width, float height, bool hover = false);
    void drawTextField(sf::RenderWindow& window, const std::string& label, 
                       const std::string& value, float x, float y, 
                       float width, float height, bool active = false);
    
    void startAsHost();
    void startAsClient();

    void onClientConnected(bool connected);
    void onPlanetsUpdated(const std::vector<PlanetData>& planets);
};

#endif // UI_HPP