#include <SFML/Graphics.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <memory>
#include <string>
#include <algorithm>
#include <cmath>

using namespace std;

// Estructura para almacenar datos de Pokémon
struct Pokemon {
    string name;
    vector<string> types;
    string level = "50";
    vector<pair<string, string>> moves; // Nombre y tipo de los ataques
    vector<string> stats; // Para almacenar los stats del pokemon (defensa, ataque, etc.)
};

// Estructura para almacenar datos de Ataques
struct Move {
    string id;
    string name;
    string description;
    string type;
    string category; // "Physical", "Special" o "Status"
    string power;
    string accuracy;
    string pp;
    string z_effect;
    string priority;
    string crit;
};

struct TypeEffectiveness {
    string defense_type1;
    string defense_type2;
    unordered_map<string, float> effectiveness;
};

// Variables globales
sf::Texture typesTexture;
unordered_map<string, sf::Sprite> typeSprites;
unordered_map<string, Move> movesDatabase;
vector<TypeEffectiveness> typeChart;
sf::Font globalFont;

// Función para cargar los ataques desde CSV (sin stoi)
void loadMovesData(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error al abrir el archivo moves.csv" << endl;
        return;
    }

    string line;
    // Leer encabezado
    getline(file, line);

    while (getline(file, line)) {
        stringstream ss(line);
        string token;
        Move m;

        // ID
        getline(ss, m.id, ',');

        // Nombre
        getline(ss, m.name, ',');

        // Descripción (puede contener comas)
        string description;
        getline(ss, description, '"'); // Leer hasta la primera comilla
        getline(ss, description, '"');  // Leer la descripción entre comillas
        m.description = description;
        getline(ss, token, ','); // Leer la coma después de la descripción

        // Tipo
        getline(ss, m.type, ',');

        // Categoría
        getline(ss, m.category, ',');

        // Power
        getline(ss, m.power, ',');

        // Accuracy
        getline(ss, m.accuracy, ',');

        // PP
        getline(ss, m.pp, ',');

        // Z-Effect
        getline(ss, m.z_effect, ',');

        // Priority
        getline(ss, m.priority, ',');

        // Crit
        getline(ss, m.crit, ',');

        movesDatabase[m.id] = m;
    }
}
// Función para cargar la tabla de efectividad de tipos
void loadTypeChart(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error al abrir " << filename << endl;
        return;
    }

    string line;
    // Leer encabezado
    getline(file, line);
    stringstream header(line);
    string token;
    vector<string> attackTypes;

    // Leer tipos de ataque (3ra columna en adelante)
    for (int i = 0; i < 2; i++) getline(header, token, ','); // Saltar primeras 2 columnas
    while (getline(header, token, ',')) {
        attackTypes.push_back(token);
    }

    // Leer el resto del archivo
    while (getline(file, line)) {
        stringstream ss(line);
        TypeEffectiveness te;
        
        // Leer tipos de defensa
        getline(ss, te.defense_type1, ',');
        getline(ss, te.defense_type2, ',');

        // Leer efectividades
        string effStr;
        for (size_t i = 0; i < attackTypes.size(); i++) {
            if (!getline(ss, effStr, ',')) break;
            te.effectiveness[attackTypes[i]] = stof(effStr);
        }

        typeChart.push_back(te);
    }
}


unordered_map<string, vector<string>> loadPokemonStats(const string& filename) {
    unordered_map<string, vector<string>> statsMap;
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error al abrir " << filename << endl;
        return statsMap;
    }

    string line;
    // Saltar encabezado
    getline(file, line);

    while (getline(file, line)) {
        stringstream ss(line);
        vector<string> stats;
        string token;

        while (getline(ss, token, ',')) {
            stats.push_back(token);
        }

        if (stats.size() > 2) {  // Asegurarnos que hay al menos 3 columnas
            // Usar la tercera columna (species) como clave
            string name = stats[2];
            // Eliminar comillas si existen
            if (!name.empty() && name.front() == '"' && name.back() == '"') {
                name = name.substr(1, name.size()-2);
            }
            statsMap[name] = stats;
        }
    }

    return statsMap;
}

// Función para inicializar los sprites de tipos
void initTypeSprites() {
    if (!typesTexture.loadFromFile("tipos.png")) {
        cerr << "Error al cargar tipos.png" << endl;
        return;
    }

    // Dividir la textura en 3 columnas y 6 filas
    sf::Vector2u textureSize = typesTexture.getSize();
    int cellWidth = textureSize.x / 3;
    int cellHeight = textureSize.y / 6;

    // Orden de los tipos en la imagen
    vector<string> typeOrder = {
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
unordered_map<string, Pokemon> loadPokemonData(const string& filename, vector<string>& names) {
    unordered_map<string, Pokemon> pokedex;
    ifstream file(filename);
    string line;

    // Saltar encabezado
    getline(file, line);

    while (getline(file, line)) {
        stringstream ss(line);
        string id, name, typesStr;

        getline(ss, id, ',');     // saltar número
        getline(ss, name, ',');   // obtener nombre
        getline(ss, typesStr, ','); // obtener tipos

        Pokemon p;
        p.name = name;
        names.push_back(name);

        stringstream typeStream(typesStr);
        string type;
        while (typeStream >> type) {
            p.types.push_back(type);
        }

        pokedex[name] = p;
    }
    
    // Ordenar los nombres alfabéticamente
    sort(names.begin(), names.end());
    
    return pokedex;
}



// Clase para el input de nivel
class LevelInput {
public:
    LevelInput(float x, float y, float width, float height, sf::Font& font) 
        : font(font), isActive(false) {
        box.setPosition(x, y);
        box.setSize({width, height});
        box.setFillColor(sf::Color(200, 200, 200));
        box.setOutlineThickness(1);
        box.setOutlineColor(sf::Color::Black);

        label.setFont(font);
        label.setString("Nivel: ");
        label.setCharacterSize(14);
        label.setPosition(x - 50, y + 5);
        label.setFillColor(sf::Color::Black);

        text.setFont(font);
        text.setString("50");
        text.setCharacterSize(14);
        text.setPosition(x + 5, y + 5);
        text.setFillColor(sf::Color::Black);
    }

    void draw(sf::RenderWindow& window) {
        window.draw(box);
        window.draw(label);
        window.draw(text);

        if (isActive) {
            sf::RectangleShape cursor;
            cursor.setSize({2, 16});
            cursor.setPosition(text.getPosition().x + text.getLocalBounds().width + 2, text.getPosition().y);
            cursor.setFillColor(sf::Color::Black);
            window.draw(cursor);
        }
    }

    void handleEvent(sf::Event event, sf::Vector2f mousePos) {
        if (event.type == sf::Event::MouseButtonPressed) {
            if (box.getGlobalBounds().contains(mousePos)) {
                isActive = true;
            } else {
                isActive = false;
            }
        }

        if (isActive && event.type == sf::Event::TextEntered) {
            if (event.text.unicode == 8 && !inputText.empty()) { // Backspace
                inputText.pop_back();
            } else if (event.text.unicode >= 48 && event.text.unicode <= 57) { // Solo números
                if (inputText.size() < 3) {
                    inputText += static_cast<char>(event.text.unicode);
                }
            }

            // Actualizar el texto mostrado
            text.setString(inputText.empty() ? "50" : inputText);
        }
    }

    string getLevel() const {
        return inputText.empty() ? "50" : inputText;
    }

private:
    sf::RectangleShape box;
    sf::Text label;
    sf::Text text;
    sf::Font& font;
    bool isActive;
    string inputText;
};

// Clase para el selector de ataques
class MoveSelector {
public:
    MoveSelector(float x, float y, float width, float height, const vector<string>& moves, sf::Font& font)
        : allMoves(moves), filteredMoves(moves), font(font), isActive(false), selectedIndex(-1), startIndex(0), maxVisible(7) {
        
        background.setPosition(x, y);
        background.setSize({width, height});
        background.setFillColor(sf::Color(220, 220, 220));
        background.setOutlineThickness(1);
        background.setOutlineColor(sf::Color::Black);

        title.setFont(font);
        title.setString("Seleccionar Ataques (Enter para confirmar)");
        title.setCharacterSize(14);
        title.setPosition(x + 5, y + 5);
        title.setFillColor(sf::Color::Black);

        button.setPosition(x, y + height + 5);
        button.setSize({width, 30});
        button.setFillColor(sf::Color(150, 150, 200));

        buttonText.setFont(font);
        buttonText.setString("Seleccionar Ataques");
        buttonText.setCharacterSize(14);
        buttonText.setPosition(x + 5, y + height + 10);
        buttonText.setFillColor(sf::Color::Black);

        // Crear lista de nombres de movimientos
        vector<string> moveNames;
        for (const auto& pair : movesDatabase) {
            moveNames.push_back(pair.second.name);
        }
        sort(moveNames.begin(), moveNames.end());
        allMoves = moveNames;
        filteredMoves = moveNames;
    }

    void draw(sf::RenderWindow& window) {
        window.draw(button);
        window.draw(buttonText);

        if (isActive) {
            window.draw(background);
            window.draw(title);

            // Dibujar los ataques seleccionados
            for (size_t i = 0; i < selectedMoves.size(); ++i) {
                sf::Text moveText;
                moveText.setFont(font);
                moveText.setString(to_string(i+1) + ": " + selectedMoves[i].first);
                moveText.setCharacterSize(14);
                moveText.setPosition(background.getPosition().x + 5, background.getPosition().y + 30 + i * 20);
                moveText.setFillColor(sf::Color::Black);
                window.draw(moveText);

                // Dibujar el tipo del ataque
                if (typeSprites.count(selectedMoves[i].second)) {
                    sf::Sprite typeSprite = typeSprites[selectedMoves[i].second];
                    typeSprite.setPosition(background.getPosition().x + 150, background.getPosition().y + 30 + i * 20);
                    window.draw(typeSprite);
                }
            }

            // Dibujar lista de ataques disponibles si no hemos seleccionado 4
            if (selectedMoves.size() < 4) {
                for (int i = 0; i < maxVisible && startIndex + i < filteredMoves.size(); ++i) {
                    sf::RectangleShape optionBox;
                    optionBox.setSize({background.getSize().x, 20});
                    optionBox.setPosition(background.getPosition().x, background.getPosition().y + 110 + i * 20);
                    optionBox.setFillColor(i == selectedIndex ? sf::Color(180, 180, 250) : sf::Color(240, 240, 240));
                    window.draw(optionBox);

                    sf::Text option;
                    option.setFont(font);
                    option.setString(filteredMoves[startIndex + i]);
                    option.setCharacterSize(14);
                    option.setFillColor(sf::Color::Black);
                    option.setPosition(background.getPosition().x + 5, background.getPosition().y + 110 + i * 20 + 3);
                    window.draw(option);
                }

                if (filteredMoves.size() > maxVisible) {
                    float scrollbarHeight = maxVisible * 20;
                    float thumbHeight = scrollbarHeight * maxVisible / (float)filteredMoves.size();
                    float thumbY = background.getPosition().y + 110 + (scrollbarHeight - thumbHeight) * startIndex / (float)(filteredMoves.size() - maxVisible);

                    sf::RectangleShape scrollbar;
                    scrollbar.setSize({8, scrollbarHeight});
                    scrollbar.setPosition(background.getPosition().x + background.getSize().x - 8, background.getPosition().y + 110);
                    scrollbar.setFillColor(sf::Color(200, 200, 200));
                    window.draw(scrollbar);

                    sf::RectangleShape thumb;
                    thumb.setSize({8, thumbHeight});
                    thumb.setPosition(scrollbar.getPosition().x, thumbY);
                    thumb.setFillColor(sf::Color(120, 120, 120));
                    window.draw(thumb);
                }
            }
        }
    }

    void handleEvent(sf::Event event, sf::Vector2f mousePos) {
        if (event.type == sf::Event::MouseButtonPressed) {
            if (button.getGlobalBounds().contains(mousePos)) {
                isActive = !isActive;
                selectedIndex = -1;
            } else if (isActive && selectedMoves.size() < 4) {
                // Verificar si se hizo click en un ataque de la lista
                for (int i = 0; i < maxVisible && startIndex + i < filteredMoves.size(); ++i) {
                    sf::FloatRect optionBounds(background.getPosition().x, background.getPosition().y + 110 + i * 20, 
                                            background.getSize().x, 20);
                    if (optionBounds.contains(mousePos)) {
                        string moveName = filteredMoves[startIndex + i];
                        // Buscar el tipo del ataque
                        string moveType;
                        for (const auto& pair : movesDatabase) {
                            if (pair.second.name == moveName) {
                                moveType = pair.second.type;
                                break;
                            }
                        }
                        selectedMoves.emplace_back(moveName, moveType);
                        break;
                    }
                }
            }
        }

        if (isActive && event.type == sf::Event::MouseWheelScrolled) {
            if (selectedMoves.size() < 4) {
                startIndex -= (int)event.mouseWheelScroll.delta;
                if (startIndex < 0) startIndex = 0;
                if (startIndex > (int)filteredMoves.size() - maxVisible)
                    startIndex = (int)filteredMoves.size() - maxVisible;
            }
        }

        if (isActive && event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
            isActive = false;
        }

        if (isActive && event.type == sf::Event::TextEntered && selectedMoves.size() < 4) {
            if (event.text.unicode == 8 && !searchText.empty()) { // Backspace
                searchText.pop_back();
                filterMoves();
            } else if (event.text.unicode >= 32 && event.text.unicode <= 126) { // Caracteres imprimibles
                searchText += static_cast<char>(event.text.unicode);
                filterMoves();
            }
        }
    }

    void filterMoves() {
        if (searchText.empty()) {
            filteredMoves = allMoves;
        } else {
            filteredMoves.clear();
            string searchLower = searchText;
            transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);
            
            for (const auto& move : allMoves) {
                string moveLower = move;
                transform(moveLower.begin(), moveLower.end(), moveLower.begin(), ::tolower);
                
                if (moveLower.find(searchLower) != string::npos) {
                    filteredMoves.push_back(move);
                }
            }
        }
        startIndex = 0;
    }

    const vector<pair<string, string>>& getSelectedMoves() const {
        return selectedMoves;
    }

private:
    sf::RectangleShape background;
    sf::RectangleShape button;
    sf::Text title;
    sf::Text buttonText;
    sf::Font& font;
    vector<string> allMoves;
    vector<string> filteredMoves;
    vector<pair<string, string>> selectedMoves; // Nombre y tipo de los ataques
    bool isActive;
    int selectedIndex;
    int startIndex;
    const int maxVisible;
    string searchText;
};

// Clase Dropdown modificada
class Dropdown {
public:
    Dropdown(float x, float y, float width, float height, const vector<string>& items, sf::Font& font)
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

        // Crear input de nivel
        levelInput = make_unique<LevelInput>(x, y + height + 200, 50, 25, font);

        // Crear selector de ataques
        vector<string> moveNames;
        for (const auto& pair : movesDatabase) {
            moveNames.push_back(pair.second.name);
        }
        sort(moveNames.begin(), moveNames.end());
        moveSelector = make_unique<MoveSelector>(x, y + height + 250, width, 200, moveNames, font);
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

            // Dibujar el nivel
            levelInput->draw(window);

            // Dibujar el selector de ataques
            moveSelector->draw(window);
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
            if (event.text.unicode < 128 && event.text.unicode != 8 && event.text.unicode != 13) {
                typingText += static_cast<char>(event.text.unicode);
                filterItems();
                typingClock.restart();
                lastTypingTime = 0;
            } else if (event.text.unicode == 8 && !typingText.empty()) {
                typingText.pop_back();
                filterItems();
                typingClock.restart();
                lastTypingTime = 0;
            }
        }

        if (isTyping && typingClock.getElapsedTime().asSeconds() - lastTypingTime > 3.0f) {
            typingText = "";
            filterItems();
            lastTypingTime = typingClock.getElapsedTime().asSeconds();
        }

        // Manejar eventos del input de nivel
        if (levelInput) {
            levelInput->handleEvent(event, mousePos);
        }

        // Manejar eventos del selector de ataques
        if (moveSelector) {
            moveSelector->handleEvent(event, mousePos);
        }
    }

    void filterItems() {
        if (typingText.empty()) {
            filteredItems = allItems;
        } else {
            filteredItems.clear();
            string searchTextLower = typingText;
            transform(searchTextLower.begin(), searchTextLower.end(), searchTextLower.begin(), ::tolower);
            
            for (const auto& item : allItems) {
                string itemLower = item;
                transform(itemLower.begin(), itemLower.end(), itemLower.begin(), ::tolower);
                
                if (itemLower.find(searchTextLower) == 0) {
                    filteredItems.push_back(item);
                }
            }
        }
        startIndex = 0;
    }

    void loadImage(const string& name) {
        string filename = "Pokemon_Dataset/" + name + ".png";
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

    void setTypes(const vector<string>& types) {
        currentTypes = types;
    }

    string getSelectedItem() const {
        if (selectedIndex >= 0 && selectedIndex < filteredItems.size())
            return filteredItems[selectedIndex];
        return "";
    }

    string getLevel() const {
        return levelInput ? levelInput->getLevel() : "50";
    }

    const vector<pair<string, string>>& getMoves() const {
        return moveSelector ? moveSelector->getSelectedMoves() : emptyMoves;
    }

private:
    sf::RectangleShape box;
    sf::Text label;
    vector<string> allItems;
    vector<string> filteredItems;
    bool expanded;
    int selectedIndex;
    int startIndex;
    int maxVisible;
    sf::Font& font;
    sf::Texture texture;
    sf::Sprite image;
    string selectedImage;
    vector<string> currentTypes;
    
    // Variables para la búsqueda por teclado
    bool isTyping;
    string typingText;
    sf::Clock typingClock;
    float lastTypingTime;

    // Input de nivel
    unique_ptr<LevelInput> levelInput;

    // Selector de ataques
    unique_ptr<MoveSelector> moveSelector;
    static const vector<pair<string, string>> emptyMoves;
};

// Definición de la variable estática
const vector<pair<string, string>> Dropdown::emptyMoves;

// Función para mostrar los tipos
void mostrarTipos(const vector<string>& types, int pokemonNum, Dropdown& dropdown) {
    dropdown.setTypes(types);
    cout << "Tipos del Pokémon " << pokemonNum << ": ";
    for (const auto& type : types)
        cout << type << " ";
    cout << endl;
}



void calcularDanio(const Pokemon& atacante, const Pokemon& defensor, const Move& move, 
                  const unordered_map<string, vector<string>>& pokemonStats) {
    if (move.category == "Status") {
        cout << "El ataque " << move.name << " es de estado y no causa daño." << endl;
        return;
    }

    // Obtener stats del defensor
    auto defensorStats = pokemonStats.find(defensor.name);
    if (defensorStats == pokemonStats.end()) {
        cerr << "No se encontraron stats para " << defensor.name << endl;
        return;
    }

    // Obtener stats del atacante
    auto atacanteStats = pokemonStats.find(atacante.name);
    if (atacanteStats == pokemonStats.end()) {
        cerr << "No se encontraron stats para " << atacante.name << endl;
        return;
    }
    

    // Obtener valores necesarios
    cout<<atacante.level<<"WA"<<atacanteStats->second[11]<<"WA"<<defensorStats->second[13]<<"WA"<<atacanteStats->second[12]<<"WA"<<defensorStats->second[14]<<"WA"<<move.power<<"WA"<<move.accuracy<<endl;
    int N = std::stoi(atacante.level); // Nivel del atacante
    int A, D;
    if (move.category == "Physical") {
        string AA = atacanteStats->second[11];
        string BB = atacanteStats->second[13];
        A = std::stoi(AA); // Ataque (columna 12)
        D = std::stoi(BB); // Defensa (columna 14)
    } else { // Special
        string AA = atacanteStats->second[12];
        string BB = atacanteStats->second[14];
        A = std::stoi(AA); // Ataque especial (columna 13)
        D = std::stoi(BB); // Defensa especial (columna 15)
    }
    string PP = move.power;
    int P = std::stoi(PP);

    // Calcular bonificación (B)
    float B = 1.0f;
    for (const auto& tipo : atacante.types) {
        if (tipo == move.type) {
            B = 1.5f;
            break;
        }
    }

    // Calcular efectividad (E)
    float E = 1.0f;
    for (const auto& entry : typeChart) {
        if ((entry.defense_type1 == defensor.types[0] && 
             (defensor.types.size() < 2 || entry.defense_type2 == defensor.types[1])) ||
            (defensor.types.size() > 1 && entry.defense_type1 == defensor.types[1] && 
             entry.defense_type2 == defensor.types[0])) {
            
            auto it = entry.effectiveness.find(move.type);
            if (it != entry.effectiveness.end()) {
                E = it->second;
            }
            break;
        }
    }

    // Calcular daño para V=85 y V=100
    float danioMin = 0.01f * B * E * 85 * ((((0.2f * N + 1) * A * P) / (25 * D)) + 2);
    float danioMax = 0.01f * B * E * 100 * ((((0.2f * N + 1) * A * P) / (25 * D)) + 2);

    // Mostrar resultados
    cout << "Ataque: " << move.name << " (" << move.type << ")" << endl;
    cout << "  Efectividad (E): " << E << endl;
    cout << "  Bonificación (B): " << B << endl;
    cout << "  Menor posible: " << floor(danioMin) << endl;
    cout << "  Mayor posible: " << floor(danioMax) << endl;
}

void procesar(Dropdown& mainDropdown, vector<Dropdown>& rightDropdowns, 
             const unordered_map<string, Pokemon>& pokedex,
             const unordered_map<string, vector<string>>& pokemonStats) {
    string mainName = mainDropdown.getSelectedItem();
    
    // Obtener Pokémon principal (solo para mostrar info)
    auto mainIt = pokedex.find(mainName);
    if (mainIt != pokedex.end()) {
        cout << "Pokémon principal: " << mainName << " (Nivel " << mainDropdown.getLevel() << ")" << endl;
        cout << "Tipos: ";
        for (const auto& type : mainIt->second.types) cout << type << " ";
        cout << endl;
    }

    // Procesar cada Pokémon de la derecha
    for (size_t i = 0; i < rightDropdowns.size(); ++i) {
        string name = rightDropdowns[i].getSelectedItem();
        if (name.empty()) continue;

        auto it = pokedex.find(name);
        if (it != pokedex.end()) {
            cout << "\nPokémon rival " << i+1 << ": " << name << endl;
            cout << "Tipos: ";
            for (const auto& type : it->second.types) cout << type << " ";
            cout << endl;

            // Calcular daño para cada ataque
            const auto& moves = rightDropdowns[i].getMoves();
            if (moves.empty()) {
                cout << "  No tiene ataques seleccionados" << endl;
                continue;
            }

            for (const auto& movePair : moves) {
                auto moveIt = find_if(movesDatabase.begin(), movesDatabase.end(),
                    [&](const pair<string, Move>& m) { return m.second.name == movePair.first; });
                
                if (moveIt != movesDatabase.end()) {
                    // Crear Pokémon atacante con los datos del principal
                    Pokemon atacante;
                    atacante.name = mainName;
                    atacante.level = mainDropdown.getLevel();
                    atacante.types = mainIt->second.types;

                    calcularDanio(atacante, it->second, moveIt->second, pokemonStats);
                }
            }
        }
    }
}


int main() {
    // Cargar datos necesarios
    loadTypeChart("type-chart.csv");
    auto pokemonStats = loadPokemonStats("pokemon.csv");
    loadMovesData("moves.csv");

    vector<string> pokemonNames;
    auto pokedex = loadPokemonData("pokemon_data.csv", pokemonNames);

    if (pokemonNames.empty()) {
        cerr << "No se cargaron nombres del CSV. Verifica el archivo." << endl;
        return 1;
    }

    int screenWidth = 1200;
    int screenHeight = 800;
    sf::RenderWindow window(sf::VideoMode(screenWidth, screenHeight), "Sistema Experto - Pokémon");

    if (!globalFont.loadFromFile("arial.ttf")) {
        cerr << "Error: No se pudo cargar la fuente arial.ttf" << endl;
        return 1;
    }

    // Inicializar los sprites de tipos
    initTypeSprites();

    // Cargar fondo
    sf::Texture fondoTexture;
    if (!fondoTexture.loadFromFile("fondo.jpg")) {
        cerr << "Error: No se pudo cargar fondo.png" << endl;
        return 1;
    }
    sf::Sprite fondoSprite(fondoTexture);

    Dropdown* currentlyExpanded = nullptr;

    Dropdown mainDropdown(40, 50, screenWidth / 3.0f - 80, 30.0f, pokemonNames, globalFont);

    vector<Dropdown> rightDropdowns;
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
                    procesar(mainDropdown, rightDropdowns, pokedex, pokemonStats);
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