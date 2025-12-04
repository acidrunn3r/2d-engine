#include "engine/include/ui.hpp"
#include <iostream>

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "Planet Simulation Network");
    window.setFramerateLimit(60);
    
    UserInterface ui;
    ui.setExitCallback([&window]() {
        window.close();
    });
    
    sf::View simulation_view = window.getDefaultView();
    float camera_speed = 5.0f;
    int scroll_border = 20;
    
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            
            ui.handleEvent(event);
            
            if (ui.isConnected()) {
                if (event.type == sf::Event::MouseWheelScrolled) {
                    if (event.mouseWheelScroll.delta > 0) {
                        simulation_view.zoom(0.9f);
                    } else {
                        simulation_view.zoom(1.1f);
                    }
                }
                
                if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::Escape) {
                        ui.disconnect();
                    }
                    else if (event.key.code == sf::Keyboard::R) {
                        simulation_view = window.getDefaultView();
                    }
                }
            }
        }
        
        if (ui.isConnected()) {
            sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);
            sf::Vector2u window_size = window.getSize();
            
            if (mouse_pos.x < scroll_border) simulation_view.move(-camera_speed, 0);
            if (mouse_pos.x > window_size.x - scroll_border) simulation_view.move(camera_speed, 0);
            if (mouse_pos.y < scroll_border) simulation_view.move(0, -camera_speed);
            if (mouse_pos.y > window_size.y - scroll_border) simulation_view.move(0, camera_speed);
            
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) simulation_view.move(-camera_speed, 0);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) simulation_view.move(camera_speed, 0);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) simulation_view.move(0, -camera_speed);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) simulation_view.move(0, camera_speed);
        }
        
        ui.update();
        
        window.clear(sf::Color(30, 30, 50));
        
        if (ui.isConnected()) {
            window.setView(simulation_view);
            
            auto trajectories = ui.getTrajectories();
            for (const auto& trajectory : trajectories) {
                if (trajectory.size() > 1) {
                    window.draw(&trajectory[0], trajectory.size(), sf::LineStrip);
                }
            }
            
            auto planets = ui.getPlanets();
            for (const auto& planet : planets) {
                sf::CircleShape shape(planet.radius);
                shape.setPosition(planet.position.x - planet.radius, 
                                 planet.position.y - planet.radius);
                shape.setFillColor(planet.color);
                shape.setOutlineThickness(2);
                shape.setOutlineColor(sf::Color::Black);
                window.draw(shape);
            }
            
            window.setView(window.getDefaultView());
            
            sf::Font font;
            if (font.loadFromFile("assets/arial.ttf")) {
                sf::Text info_text;
                info_text.setFont(font);
                info_text.setCharacterSize(16);
                info_text.setFillColor(sf::Color::White);
                info_text.setString("Connected" + std::string(ui.isHost() ? " (Host)" : "") + 
                                   " | ESC to disconnect");
                info_text.setPosition(10, 10);
                window.draw(info_text);
            }
        } else {
            window.setView(window.getDefaultView());
        }
        
        ui.draw(window);
        
        window.display();
    }
    
    return 0;
}