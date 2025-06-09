#include <SFML/Graphics.hpp>
#include <vector>
#include <unordered_map>
#include <memory>
#include "logic/pokemon.h"
#include "logic/data_loader.h"
#include "logic/damage_calc.h"
#include "graphics/sprites.h"
#include "graphics/dropdown.h"
#include "graphics/results_display.h"
#include <iostream>

int main() {
    // Cargar datos necesarios
    auto typeChart = loadTypeChart("type-chart.csv");
    auto pokemonStats = loadPokemonStats("pokemon.csv");
    auto movesDatabase = loadMovesData("moves.csv");

    std::vector<std::string> pokemonNames;
    auto pokedex = loadPokemonData("pokemon_data.csv", pokemonNames);

    if (pokemonNames.empty()) {
        std::cerr << "No se cargaron nombres del CSV. Verifica el archivo." << std::endl;
        return 1;
    }

    // Configuración de ventana
    int screenWidth = 1600;
    int screenHeight = 900;
    sf::RenderWindow window(sf::VideoMode(screenWidth, screenHeight), "Sistema Experto - Pokémon");

    // Cargar fuente global
    sf::Font globalFont;
    if (!globalFont.loadFromFile("arial.ttf")) {
        std::cerr << "Error: No se pudo cargar la fuente arial.ttf" << std::endl;
        return 1;
    }

    // Inicializar sprites de tipos
    TypeSprites::init();

    // Cargar fondo
    sf::Texture fondoTexture;
    if (!fondoTexture.loadFromFile("fondo.jpg")) {
        std::cerr << "Error: No se pudo cargar fondo.jpg" << std::endl;
    }
    sf::Sprite fondoSprite(fondoTexture);

    // Variables para la interfaz
    Dropdown* currentlyExpanded = nullptr;
    std::vector<AttackResult> currentResults;

    // Crear dropdown principal (izquierda)
    Dropdown mainDropdown(40, 50, screenWidth / 3.0f - 80, 30.0f, pokemonNames, globalFont);

    // Crear dropdowns de la derecha (6 en total)
    std::vector<Dropdown> rightDropdowns;
    float rightStartX = screenWidth * 1.0f / 2.0f - 160;
    float spacingX = 160;
    float spacingY = 320;
    for (int i = 0; i < 6; ++i) {
        float x = rightStartX + (i % 3) * spacingX;
        float y = 100 + (i / 3) * spacingY;
        rightDropdowns.emplace_back(x, y, 140, 30.0f, pokemonNames, globalFont);
    }

    // Botón de procesar
    sf::RectangleShape botonProcesar(sf::Vector2f(200, 40));
    botonProcesar.setPosition(screenWidth - 240, screenHeight - 80);
    botonProcesar.setFillColor(sf::Color(150, 200, 150));

    sf::Text textoProcesar("Procesar", globalFont, 20);
    textoProcesar.setPosition(botonProcesar.getPosition().x + 40, botonProcesar.getPosition().y + 5);
    textoProcesar.setFillColor(sf::Color::Black);

    // Bucle principal
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            sf::Vector2f mousePos = static_cast<sf::Vector2f>(sf::Mouse::getPosition(window));
            
            // Manejar eventos en los dropdowns
            mainDropdown.handleEvent(event, mousePos, currentlyExpanded);
            for (auto& dd : rightDropdowns) {
                dd.handleEvent(event, mousePos, currentlyExpanded);
            }

            // Manejar clic en botón procesar
            if (event.type == sf::Event::MouseButtonPressed) {
                if (botonProcesar.getGlobalBounds().contains(mousePos)) {
                    // Obtener datos de los dropdowns
                    std::string mainName = mainDropdown.getSelectedItem();
                    std::string mainLevel = mainDropdown.getLevel();
                    
                    std::vector<std::string> rightNames;
                    std::vector<std::vector<std::pair<std::string, std::string>>> rightMoves;
                    
                    for (auto& dd : rightDropdowns) {
                        rightNames.push_back(dd.getSelectedItem());
                        rightMoves.push_back(dd.getMoves());
                    }
                    
                    // Calcular resultados
                    currentResults = procesar(mainName, rightNames, pokedex, pokemonStats, 
                                           movesDatabase, typeChart, mainLevel, rightMoves);
                }
            }
        }

        // Renderizado
        window.clear(sf::Color::White);
        
        // Dibujar fondo
        if (fondoTexture.getSize().x > 0) {
            window.draw(fondoSprite);
        }
        
        // Dibujar UI
        mainDropdown.draw(window);
        for (auto& dd : rightDropdowns) {
            dd.draw(window);
        }
        
        window.draw(botonProcesar);
        window.draw(textoProcesar);
        
        // Dibujar resultados
        ResultsDisplay::draw(window, currentResults, globalFont);
        
        window.display();
    }

    return 0;
}