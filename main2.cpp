#include <SFML/Graphics.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <algorithm>

using namespace std;

// Estructura para almacenar datos de Pokémon
struct Pokemon {
    std::string name;
    std::vector<std::string> types;
};

// Variables globales para los tipos
sf::Texture typesTexture;
std::unordered_map<std::string, sf::Sprite> typeSprites;

// Función para inicializar los sprites de tipos
void initTypeSprites() {
    if (!typesTexture.loadFromFile("tipos.png")) {
        std::cerr << "Error al cargar tipos.png" << std::endl;
        return;
    }

    // Dividir la textura en 3 columnas y 6 filas
    sf::Vector2u textureSize = typesTexture.getSize();
    int cellWidth = textureSize.x / 3;
    int cellHeight = textureSize.y / 6;

    // Orden de los tipos en la imagen
    std::vector<std::string> typeOrder = {
        "Bug", "Dark", "Dragon",
        "Electric", "Fairy", "Fighting",
        "Fire", "Flying", "Ghost",
        "Grass", "Ground", "Ice",
        "Normal", "Poison", "Psychic",
        "Rock", "Steel", "Water"
    };

    for (int i = 0; i < 6; ++i) { // filas
        for (int j = 0; j < 3; ++j) { // columnas
            int index = i * 3 + j;
            if (index < typeOrder.size()) {
                sf::IntRect rect(j * cellWidth, i * cellHeight, cellWidth, cellHeight);
                sf::Sprite sprite(typesTexture, rect);
                
                // Escalar al tamaño deseado (20x20)
                float scaleX = 40.0f / cellWidth;
                float scaleY = 20.0f / cellHeight;
                sprite.setScale(scaleX, scaleY);
                
                typeSprites[typeOrder[index]] = sprite;
            }
        }
    }
}

// Cargar datos de Pokémon desde CSV
std::unordered_map<std::string, Pokemon> loadPokemonData(const std::string& filename, std::vector<std::string>& names) {
    std::unordered_map<std::string, Pokemon> pokedex;
    std::ifstream file(filename);
    std::string line;

    // Saltar encabezado
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string id, name, typesStr;

        std::getline(ss, id, ',');     // saltar número
        std::getline(ss, name, ',');   // obtener nombre
        std::getline(ss, typesStr, ','); // obtener tipos

        Pokemon p;
        p.name = name;
        names.push_back(name);

        std::stringstream typeStream(typesStr);
        std::string type;
        while (typeStream >> type) {
            p.types.push_back(type);
        }

        pokedex[name] = p;
    }
    
    // Ordenar los nombres alfabéticamente
    std::sort(names.begin(), names.end());
    
    return pokedex;
}

// Clase Dropdown modificada con búsqueda alfabética
class Dropdown {
public:
    Dropdown(float x, float y, float width, float height, const std::vector<std::string>& items, sf::Font& font)
        : allItems(items), filteredItems(items), expanded(false), selectedIndex(-1), startIndex(0), maxVisible(7), font(font), 
          isTyping(false), typingText(""), typingClock(), lastTypingTime(0) {
        
        box.setPosition(x, y);
        box.setSize({width, height});
        box.setFillColor(sf::Color(180, 180, 180));

        label.setFont(font);
        label.setString("Seleccionar...");
        label.setCharacterSize(14);
        label.setPosition(x + 5, y + 5);
        label.setFillColor(sf::Color::Black);

        image.setPosition(x, y + height + 5); 
    }

    void draw(sf::RenderWindow& window) {
        window.draw(box);
        window.draw(label);

        if (isTyping) {
            sf::Text typingDisplay;
            typingDisplay.setFont(font);
            typingDisplay.setString(typingText + "_");
            typingDisplay.setCharacterSize(14);
            typingDisplay.setFillColor(sf::Color::Black);
            typingDisplay.setPosition(box.getPosition().x + 5, box.getPosition().y + 5);
            window.draw(typingDisplay);
        }

        if (expanded) {
            for (int i = 0; i < maxVisible && startIndex + i < filteredItems.size(); ++i) {
                sf::RectangleShape optionBox;
                optionBox.setSize({box.getSize().x, box.getSize().y});
                optionBox.setPosition(box.getPosition().x, box.getPosition().y + box.getSize().y * (i + 1));
                optionBox.setFillColor(sf::Color(220, 220, 220));
                window.draw(optionBox);

                sf::Text option;
                option.setFont(font);
                option.setString(filteredItems[startIndex + i]);
                option.setCharacterSize(14);
                option.setFillColor(sf::Color::Black);
                option.setPosition(box.getPosition().x + 5, box.getPosition().y + box.getSize().y * (i + 1) + 5);
                window.draw(option);
            }

            if (filteredItems.size() > maxVisible) {
                float scrollbarHeight = maxVisible * box.getSize().y;
                float thumbHeight = scrollbarHeight * maxVisible / (float)filteredItems.size();
                float thumbY = box.getPosition().y + box.getSize().y + (scrollbarHeight - thumbHeight) * startIndex / (float)(filteredItems.size() - maxVisible);

                sf::RectangleShape scrollbar;
                scrollbar.setSize({8, scrollbarHeight});
                scrollbar.setPosition(box.getPosition().x + box.getSize().x - 8, box.getPosition().y + box.getSize().y);
                scrollbar.setFillColor(sf::Color(200, 200, 200));
                window.draw(scrollbar);

                sf::RectangleShape thumb;
                thumb.setSize({8, thumbHeight});
                thumb.setPosition(scrollbar.getPosition().x, thumbY);
                thumb.setFillColor(sf::Color(120, 120, 120));
                window.draw(thumb);
            }
        }
        
        if (!selectedImage.empty()) {
            window.draw(image);
            
            // Dibujar los tipos debajo de la imagen
            float startX = image.getPosition().x;
            float startY = image.getPosition().y + image.getGlobalBounds().height + 5;
            
            for (size_t i = 0; i < currentTypes.size(); ++i) {
                if (typeSprites.count(currentTypes[i])) {
                    sf::Sprite typeSprite = typeSprites[currentTypes[i]];
                    typeSprite.setPosition(startX + i * 50, startY);
                    window.draw(typeSprite);
                }
            }
        }
    }

    void handleEvent(sf::Event event, sf::Vector2f mousePos, Dropdown*& currentlyExpanded) {
        if (event.type == sf::Event::MouseButtonPressed) {
            if (box.getGlobalBounds().contains(mousePos)) {
                if (currentlyExpanded != this) {
                    if (currentlyExpanded) currentlyExpanded->expanded = false;
                    expanded = true;
                    currentlyExpanded = this;
                    isTyping = true;
                    typingText = "";
                    typingClock.restart();
                    lastTypingTime = 0;
                } else {
                    expanded = !expanded;
                    currentlyExpanded = expanded ? this : nullptr;
                    isTyping = expanded;
                    if (expanded) {
                        typingText = "";
                        typingClock.restart();
                        lastTypingTime = 0;
                    }
                }
            } else if (expanded) {
                for (int i = 0; i < maxVisible && startIndex + i < filteredItems.size(); ++i) {
                    sf::FloatRect optionBounds(box.getPosition().x, box.getPosition().y + box.getSize().y * (i + 1), box.getSize().x, box.getSize().y);
                    if (optionBounds.contains(mousePos)) {
                        label.setString(filteredItems[startIndex + i]);
                        selectedIndex = startIndex + i;
                        expanded = false;
                        isTyping = false;
                        currentlyExpanded = nullptr;
                        loadImage(filteredItems[selectedIndex]);
                        break;
                    }
                }
            } else {
                expanded = false;
                isTyping = false;
            }
        }

        if (event.type == sf::Event::MouseWheelScrolled && expanded) {
            if (box.getGlobalBounds().contains(mousePos) || 
                (mousePos.y > box.getPosition().y + box.getSize().y && 
                 mousePos.y < box.getPosition().y + box.getSize().y * (maxVisible + 1))) {
                startIndex -= (int)event.mouseWheelScroll.delta;
                if (startIndex < 0) startIndex = 0;
                if (startIndex > (int)filteredItems.size() - maxVisible)
                    startIndex = (int)filteredItems.size() - maxVisible;
            }
        }

        if (isTyping && event.type == sf::Event::TextEntered) {
            // Solo manejar caracteres imprimibles
            if (event.text.unicode < 128 && event.text.unicode != 8 && event.text.unicode != 13) {
                typingText += static_cast<char>(event.text.unicode);
                filterItems();
                typingClock.restart();
                lastTypingTime = 0;
            }
            // Manejar backspace
            else if (event.text.unicode == 8 && !typingText.empty()) {
                typingText.pop_back();
                filterItems();
                typingClock.restart();
                lastTypingTime = 0;
            }
        }

        // Resetear el texto de búsqueda después de 3 segundos de inactividad
        if (isTyping && typingClock.getElapsedTime().asSeconds() - lastTypingTime > 3.0f) {
            typingText = "";
            filterItems();
            lastTypingTime = typingClock.getElapsedTime().asSeconds();
        }
    }

    void filterItems() {
        if (typingText.empty()) {
            filteredItems = allItems;
        } else {
            filteredItems.clear();
            std::string searchTextLower = typingText;
            std::transform(searchTextLower.begin(), searchTextLower.end(), searchTextLower.begin(), ::tolower);
            
            for (const auto& item : allItems) {
                std::string itemLower = item;
                std::transform(itemLower.begin(), itemLower.end(), itemLower.begin(), ::tolower);
                
                if (itemLower.find(searchTextLower) == 0) { // Comienza con el texto de búsqueda
                    filteredItems.push_back(item);
                }
            }
        }
        startIndex = 0;
    }

    void loadImage(const std::string& name) {
        std::string filename = "Pokemon_Dataset/" + name + ".png";
        if (texture.loadFromFile(filename)) {
            image.setTexture(texture);
            if (box.getPosition().x < 300) {
                image.setScale(400.0f / texture.getSize().x, 400.0f / texture.getSize().y);
            } else {
                image.setScale(150.0f / texture.getSize().x, 150.0f / texture.getSize().y);
            }
            selectedImage = filename;
        } else {
            selectedImage.clear();
        }
    }

    void setTypes(const std::vector<std::string>& types) {
        currentTypes = types;
    }

    std::string getSelectedItem() const {
        if (selectedIndex >= 0 && selectedIndex < filteredItems.size())
            return filteredItems[selectedIndex];
        return "";
    }

private:
    sf::RectangleShape box;
    sf::Text label;
    std::vector<std::string> allItems;
    std::vector<std::string> filteredItems;
    bool expanded;
    int selectedIndex;
    int startIndex;
    int maxVisible;
    sf::Font& font;
    sf::Texture texture;
    sf::Sprite image;
    std::string selectedImage;
    std::vector<std::string> currentTypes;
    
    // Variables para la búsqueda por teclado
    bool isTyping;
    std::string typingText;
    sf::Clock typingClock;
    float lastTypingTime;
};

// Función para mostrar los tipos
void mostrarTipos(const std::vector<std::string>& types, int pokemonNum, Dropdown& dropdown) {
    dropdown.setTypes(types);
    std::cout << "Tipos del Pokémon " << pokemonNum << ": ";
    for (const auto& type : types)
        std::cout << type << " ";
    std::cout << std::endl;
}

// Función para procesar la selección
void procesar(Dropdown& mainDropdown, std::vector<Dropdown>& rightDropdowns, const std::unordered_map<std::string, Pokemon>& pokedex) {
    std::string mainName = mainDropdown.getSelectedItem();
    
    auto mainIt = pokedex.find(mainName);
    if (mainIt != pokedex.end()) {
        mostrarTipos(mainIt->second.types, 0, mainDropdown);
    } else {
        std::cout << "Pokémon principal (" << mainName << ") no encontrado en la base de datos." << std::endl;
    }

    for (size_t i = 0; i < rightDropdowns.size(); ++i) {
        std::string name = rightDropdowns[i].getSelectedItem();
        auto it = pokedex.find(name);
        if (it != pokedex.end()) {
            mostrarTipos(it->second.types, i + 1, rightDropdowns[i]);
        } else {
            std::cout << "Pokémon " << i + 1 << " (" << name << ") no encontrado en la base de datos." << std::endl;
        }
    }
}

int main() {
    std::vector<std::string> pokemonNames;
    auto pokedex = loadPokemonData("pokemon_data.csv", pokemonNames);

    if (pokemonNames.empty()) {
        std::cerr << "No se cargaron nombres del CSV. Verifica el archivo." << std::endl;
        return 1;
    }

    int screenWidth = 1200;
    int screenHeight = 800;
    sf::RenderWindow window(sf::VideoMode(screenWidth, screenHeight), "Sistema Experto - Pokémon");

    sf::Font globalFont;
    if (!globalFont.loadFromFile("arial.ttf")) {
        std::cerr << "Error: No se pudo cargar la fuente arial.ttf" << std::endl;
        return 1;
    }

    // Inicializar los sprites de tipos
    initTypeSprites();

    // Cargar fondo
    sf::Texture fondoTexture;
    if (!fondoTexture.loadFromFile("fondo.jpg")) {
        std::cerr << "Error: No se pudo cargar fondo.png" << std::endl;
        return 1;
    }
    sf::Sprite fondoSprite(fondoTexture);

    Dropdown* currentlyExpanded = nullptr;

    Dropdown mainDropdown(40, 50, screenWidth / 3.0f - 80, 30.0f, pokemonNames, globalFont);

    std::vector<Dropdown> rightDropdowns;
    float rightStartX = screenWidth * 1.0f / 2.0f + 40;
    float spacingX = 160;
    float spacingY = 320;
    for (int i = 0; i < 6; ++i) {
        float x = rightStartX + (i % 3) * spacingX;
        float y = 100 + (i / 3) * spacingY;
        rightDropdowns.emplace_back(x, y, 140, 30.0f, pokemonNames, globalFont);
    }

    sf::RectangleShape botonProcesar(sf::Vector2f(200, 40));
    botonProcesar.setPosition(screenWidth - 240, screenHeight - 80);
    botonProcesar.setFillColor(sf::Color(150, 200, 150));

    sf::Text textoProcesar("Procesar", globalFont, 20);
    textoProcesar.setPosition(botonProcesar.getPosition().x + 40, botonProcesar.getPosition().y + 5);
    textoProcesar.setFillColor(sf::Color::Black);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            sf::Vector2f mousePos = (sf::Vector2f)sf::Mouse::getPosition(window);
            mainDropdown.handleEvent(event, mousePos, currentlyExpanded);
            for (auto& dd : rightDropdowns)
                dd.handleEvent(event, mousePos, currentlyExpanded);

            if (event.type == sf::Event::MouseButtonPressed) {
                if (botonProcesar.getGlobalBounds().contains(mousePos)) {
                    procesar(mainDropdown, rightDropdowns, pokedex);
                }
            }
        }

        window.clear(sf::Color::White);
        //window.draw(fondoSprite);
        mainDropdown.draw(window);
        for (auto& dd : rightDropdowns)
            dd.draw(window);
        window.draw(botonProcesar);
        window.draw(textoProcesar);
        window.display();
    }

    return 0;
}