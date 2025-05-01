#include "modules/lcd_module.h"
#include <cstring> // Pour memset et strncpy

const uint8_t LcdModule::COMMON_LCD_ADDRESSES[4] = {
    0x27, // Adresse la plus courante pour les modules PCF8574
    0x3F, // Deuxième adresse la plus courante pour les modules PCF8574A
    0x20, // Parfois utilisée sur des adaptateurs I2C modifiés
    0x38  // Utilisée par certains fabricants
};

// Constructeur par défaut
LcdModule::LcdModule() :
    _numDetectedLcds(0),
    _anyLcdInitialized(false)
{
    // L'initialisation se fait dans begin() via scanAndInitLcds()
}

// Destructeur
LcdModule::~LcdModule() {
    // Libérer la mémoire allouée pour chaque instance LCD (application du RAII)
    for (auto* lcd : _lcds) {
        if (lcd != nullptr) {
            delete lcd;
        }
    }
    _lcds.clear(); // Vider le vecteur de pointeurs
    _addrs.clear();
    _i2cErrors.clear();
    _lastLcdBuffers.clear();
}

// Nouvelle méthode pour scanner et initialiser tous les écrans
void LcdModule::scanAndInitLcds() {
    LOG_INFO("LCD", "Scan du bus I2C pour les écrans LCD...");
    _numDetectedLcds = 0;
    _anyLcdInitialized = false;

    // Nettoyer les anciennes instances si begin() est appelé plusieurs fois
    for (auto* lcd : _lcds) {
        if (lcd != nullptr) delete lcd;
    }
    _lcds.clear();
    _addrs.clear();
    _i2cErrors.clear();
    _lastLcdBuffers.clear();

    // Initialiser I2C
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    // Fonction lambda pour tester une adresse
    auto testAddress = [&](uint8_t addr) {
        if (_numDetectedLcds >= MAX_LCD_SCREENS) return; // Limite atteinte

        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();

        if (error == 0) {
            // Vérifier si l'adresse n'est pas déjà ajoutée (évite doublons si scan complet après scan commun)
            bool alreadyFound = false;
            for (const auto foundAddr : _addrs) {
                if (foundAddr == addr) {
                    alreadyFound = true;
                    break;
                }
            }
            if (alreadyFound) return;

            LOG_INFO("LCD", "Écran LCD détecté à l'adresse 0x%02X", addr);

            // Créer et initialiser l'écran
            auto* newLcd = new LiquidCrystal_I2C(addr, LCD_COLS, LCD_ROWS);
            if (newLcd == nullptr) {
                LOG_ERROR("LCD", "Échec d'allocation mémoire pour l'écran LCD à 0x%02X", addr);
                return; // Passer à l'adresse suivante
            }

            newLcd->init(); // Tente d'initialiser
            // Une petite pause peut aider après init sur certains matériels
            delay(50);

            // Vérifier à nouveau la communication après init()
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                LOG_INFO("LCD", "Initialisation réussie pour 0x%02X", addr);
                newLcd->backlight();
                _lcds.push_back(newLcd);
                _addrs.push_back(addr);
                _i2cErrors.push_back(false);

                // Initialiser le tampon pour cet écran
                std::vector<std::vector<char>> screenBuffer(LCD_ROWS, std::vector<char>(LCD_COLS + 1, ' '));
                for (auto& rowBuffer : screenBuffer) {
                    rowBuffer[LCD_COLS] = '\0'; // Null-terminator
                }
                _lastLcdBuffers.push_back(screenBuffer);

                defineCustomCharacters(_numDetectedLcds); // Définir les caractères pour ce nouvel écran

                // Afficher un message de démarrage sur cet écran
                newLcd->clear();
                newLcd->setCursor(0, 0);
                newLcd->print(F("Kite Pilote"));
                newLcd->setCursor(0, 1);
                newLcd->print(F("Version " VERSION_STRING));
                newLcd->setCursor(0, 3);
                newLcd->print(F("LCD OK"));

                _numDetectedLcds++;
                _anyLcdInitialized = true;

            } else {
                LOG_ERROR("LCD", "Échec de communication après init() pour 0x%02X", addr);
                delete newLcd; // Nettoyer si l'init échoue
            }
        }
    };

    // Tester d'abord les adresses les plus courantes
    for (uint8_t addr : COMMON_LCD_ADDRESSES) {
        testAddress(addr);
        if (_numDetectedLcds >= MAX_LCD_SCREENS) break;
    }

    // Si moins d'écrans que le max sont trouvés, faire un scan plus large
    if (_numDetectedLcds < MAX_LCD_SCREENS) {
        LOG_INFO("LCD", "Scan complet des adresses 0x20-0x3F...");
        for (uint8_t addr = 0x20; addr <= 0x3F; addr++) {
            testAddress(addr);
            if (_numDetectedLcds >= MAX_LCD_SCREENS) break;
        }
    }

    if (_numDetectedLcds == 0) {
        LOG_WARNING("LCD", "Aucun écran LCD détecté ou initialisable sur le bus I2C");
    } else {
        LOG_INFO("LCD", "%d écran(s) LCD initialisé(s) avec succès.", _numDetectedLcds);
    }
}


// Méthode d'initialisation principale
bool LcdModule::begin() {
    scanAndInitLcds();
    return _anyLcdInitialized; // Retourne true si au moins un écran fonctionne
}

// Supprimé: testConnection() - la gestion d'erreur est implicite

// Afficher un message à une position spécifique sur tous les écrans
bool LcdModule::print(const char* message, uint8_t col, uint8_t row) {
    if (!_anyLcdInitialized || col >= LCD_COLS || row >= LCD_ROWS) {
        if (!_anyLcdInitialized) {
            LOG_WARNING("LCD", "Aucun LCD initialisé pour print()");
        } else {
            LOG_WARNING("LCD", "Coordonnées hors limites: %d,%d", col, row);
        }
        return false;
    }

    bool success = true;
    for (uint8_t i = 0; i < _numDetectedLcds; ++i) {
        if (!_i2cErrors[i] && _lcds[i] != nullptr) {
            _lcds[i]->setCursor(col, row);
            _lcds[i]->print(message);
            // Mettre à jour le tampon _lastLcd si affichage complet sur la ligne
            // Note: printDiff gère la mise à jour du tampon, donc ceci est redondant si on utilise printDiff partout
            // size_t len = strlen(message);
            // if (col == 0 && len <= LCD_COLS) {
            //     strncpy(_lastLcdBuffers[i][row].data(), message, LCD_COLS);
            //     for (size_t k = len; k < LCD_COLS; k++) _lastLcdBuffers[i][row][k] = ' ';
            //     _lastLcdBuffers[i][row][LCD_COLS] = '\0';
            // }
        } else {
            success = false; // Au moins un écran a échoué
        }
    }
    return success;
}

// Version surchargée pour accepter les chaînes Flash (F())
bool LcdModule::print(const __FlashStringHelper* message, uint8_t col, uint8_t row) {
     if (!_anyLcdInitialized || col >= LCD_COLS || row >= LCD_ROWS) {
        if (!_anyLcdInitialized) {
            LOG_WARNING("LCD", "Aucun LCD initialisé pour print(F())");
        } else {
            LOG_WARNING("LCD", "Coordonnées hors limites: %d,%d", col, row);
        }
        return false;
    }

    bool success = true;
    for (uint8_t i = 0; i < _numDetectedLcds; ++i) {
        if (!_i2cErrors[i] && _lcds[i] != nullptr) {
            _lcds[i]->setCursor(col, row);
            _lcds[i]->print(message);
        } else {
            success = false;
        }
    }
    return success;
}

// Effacer une ligne entière sur tous les écrans
void LcdModule::clearLine(uint8_t row) {
    if (!_anyLcdInitialized || row >= LCD_ROWS) {
        return;
    }

    // Utilise std::array pour une gestion plus sûre de la mémoire
    std::array<char, LCD_COLS + 1> spaces;
    spaces.fill(' ');
    spaces[LCD_COLS] = '\0';

    for (uint8_t i = 0; i < _numDetectedLcds; ++i) {
        if (!_i2cErrors[i] && _lcds[i] != nullptr) {
             printDiff(i, spaces.data(), row); // Utiliser printDiff pour optimiser et maj buffer
        }
    }
}

// Effacer tous les écrans
void LcdModule::clear() {
    if (!_anyLcdInitialized) {
        return;
    }

    for (uint8_t i = 0; i < _numDetectedLcds; ++i) {
        if (!_i2cErrors[i] && _lcds[i] != nullptr) {
            _lcds[i]->clear();
            // Réinitialiser aussi le buffer interne après un clear complet
            for (auto& rowBuffer : _lastLcdBuffers[i]) {
                std::fill(rowBuffer.begin(), rowBuffer.end() - 1, ' '); // Remplir de ' ' sauf le \0
            }
        }
    }
}

// Supprimé: refresh() - la gestion d'erreur est implicite

// Afficher une barre de progression sur tous les écrans
// Cette fonction prend un pourcentage (0-100) et affiche une barre de progression
// visuelle sur la ligne spécifiée de l'écran LCD. La barre utilise des caractères
// de bloc complet pour la partie remplie et des tirets pour la partie vide.
// Format: [########----] 45%
// La largeur de la barre s'adapte automatiquement en fonction de la largeur de l'écran
// tout en réservant l'espace nécessaire pour afficher le pourcentage.
void LcdModule::showProgressBar(uint8_t row, int percentage) {
    if (!_anyLcdInitialized || row >= LCD_ROWS) return;

    percentage = constrain(percentage, 0, 100);
    // Ajustement pour que la barre prenne toute la largeur moins la place pour le %
    int barWidth = LCD_COLS - 4; // Ex: "################ 100%"
    if (barWidth < 1) barWidth = 1; // Au moins 1 caractère pour la barre
    int filledChars = map(percentage, 0, 100, 0, barWidth);

    char buffer[LCD_COLS + 1];
    memset(buffer, 0, sizeof(buffer)); // Clear buffer

    // Construire la barre
    for (int i = 0; i < barWidth; i++) {
        // Utiliser un caractère plein (bloc) si disponible, sinon '#' ou autre
        // Le caractère 0xFF est souvent un bloc plein
        buffer[i] = (i < filledChars) ? 0xFF : '-';
    }
    // Ajouter le pourcentage à la fin
    snprintf(buffer + barWidth, sizeof(buffer) - barWidth, " %3d%%", percentage);
    // Assurer la terminaison null au cas où snprintf tronque exactement à la limite
    buffer[LCD_COLS] = '\0';

    // Afficher sur tous les écrans via printDiff
    for (uint8_t i = 0; i < _numDetectedLcds; ++i) {
         if (!_i2cErrors[i] && _lcds[i]) {
            printDiff(i, buffer, row);
         }
    }
}


// Afficher une valeur numérique sur tous les écrans
void LcdModule::showValue(const char* name, float value, const char* unit, uint8_t row, uint8_t precision) {
    if (!_anyLcdInitialized || row >= LCD_ROWS) return;

    char buffer[LCD_COLS + 1];
    char valueStr[10]; // Assez pour " -XXX.YYY"
    dtostrf(value, 1, precision, valueStr); // Convert float to string

    // Formatage pour aligner: "Nom:     VALEUR Unit"
    snprintf(buffer, sizeof(buffer), "%-8s%7s %-4s", name, valueStr, unit);
    // Tronquer si dépasse
    buffer[LCD_COLS] = '\0';

    for (uint8_t i = 0; i < _numDetectedLcds; ++i) {
         if (!_i2cErrors[i] && _lcds[i]) {
            printDiff(i, buffer, row);
         }
    }
}

// Afficher un message d'état centré sur tous les écrans
void LcdModule::showStatus(const char* status, uint8_t row, uint8_t icon) {
    if (!_anyLcdInitialized || row >= LCD_ROWS) return;

    char buffer[LCD_COLS + 1];
    int statusLen = strlen(status);
    int totalLen = statusLen;
    int iconSpace = 0;
    if (icon != 255) {
        totalLen += 1; // Espace pour l'icône
        iconSpace = 1;
        if (statusLen > 0) { // Ajouter un espace après l'icône si texte suit
             totalLen += 1;
             iconSpace = 2;
        }
    }

    int spaces = (LCD_COLS - totalLen) / 2;
    spaces = max(0, spaces);
    int pos = 0;

    // Espaces avant
    memset(buffer, ' ', spaces);
    pos += spaces;

    // Icône (si présente)
    if (icon != 255) {
        if (pos < LCD_COLS) buffer[pos++] = icon;
        if (iconSpace == 2 && pos < LCD_COLS) buffer[pos++] = ' '; // Espace après icône
    }

    // Texte du statut
    strncpy(buffer + pos, status, LCD_COLS - pos);
    pos += statusLen; // Avancer même si tronqué

    // Espaces après
    if (pos < LCD_COLS) {
        memset(buffer + pos, ' ', LCD_COLS - pos);
    }
    buffer[LCD_COLS] = '\0'; // Assurer la terminaison

    for (uint8_t i = 0; i < _numDetectedLcds; ++i) {
         if (!_i2cErrors[i] && _lcds[i]) {
            printDiff(i, buffer, row);
         }
    }
}

// Afficher un écran complet d'état du système sur tous les écrans
void LcdModule::showSystemScreen(const char* mode, float roll, float pitch, float tension, float power) {
    if (!_anyLcdInitialized) return;

    // Ligne 0: Mode
    char buffer0[LCD_COLS + 1];
    snprintf(buffer0, sizeof(buffer0), "Mode: %-13s", mode); // Laisser espace pour icône éventuelle
    buffer0[LCD_COLS] = '\0';
    // Ajouter une icône d'état global si pertinent (ex: basé sur tension/power?)
    // Pour l'instant, pas d'icône ici.
    for (uint8_t i = 0; i < _numDetectedLcds; ++i) {
        if (!_i2cErrors[i] && _lcds[i]) printDiff(i, buffer0, 0);
    }


    // Ligne 1: Roll & Pitch
    char buffer1[LCD_COLS + 1];
    char rollStr[8], pitchStr[8];
    dtostrf(roll, 4, 1, rollStr); // Format X.Y
    dtostrf(pitch, 4, 1, pitchStr);
    snprintf(buffer1, sizeof(buffer1), "R:%6s%c P:%6s%c", rollStr, (char)223, pitchStr, (char)223); // 223 = degré °
    buffer1[LCD_COLS] = '\0';
     for (uint8_t i = 0; i < _numDetectedLcds; ++i) {
        if (!_i2cErrors[i] && _lcds[i]) printDiff(i, buffer1, 1);
    }

    // Ligne 2: Tension
    showValue("Tension", tension, "N", 2, 1);

    // Ligne 3: Puissance
    showValue("Power", power, "W", 3, 1);
}

// Afficher un écran d'erreur sur tous les écrans
void LcdModule::showErrorScreen(const char* title, const char* message, int code) {
    if (!_anyLcdInitialized) return;

    clear(); // Effacer d'abord tous les écrans

    // Ligne 0: Titre centré avec icône d'erreur
    showStatus(title, 0, CHAR_ERROR);

    // Lignes suivantes: Message d'erreur (peut occuper plusieurs lignes)
    char buffer[LCD_COLS + 1];
    int msgLen = strlen(message);
    int currentLine = 1;

    for (int offset = 0; offset < msgLen && currentLine < LCD_ROWS; offset += LCD_COLS) {
        strncpy(buffer, message + offset, LCD_COLS);
        buffer[LCD_COLS] = '\0'; // Assurer terminaison
        for (uint8_t i = 0; i < _numDetectedLcds; ++i) {
            if (!_i2cErrors[i] && _lcds[i]) printDiff(i, buffer, currentLine);
        }
        currentLine++;
    }

    // Afficher le code d'erreur s'il est fourni et s'il reste de la place
    if (code >= 0 && currentLine < LCD_ROWS) {
        snprintf(buffer, sizeof(buffer), "Code: %d", code);
        buffer[LCD_COLS] = '\0';
        for (uint8_t i = 0; i < _numDetectedLcds; ++i) {
             if (!_i2cErrors[i] && _lcds[i]) printDiff(i, buffer, currentLine);
        }
    }
}

// Afficher l'état des capteurs sur tous les écrans
void LcdModule::showSensorStatus(bool imuValid, bool tensionValid, bool windValid) {
     if (!_anyLcdInitialized) return;

    char buffer[LCD_COLS + 1];

    snprintf(buffer, sizeof(buffer), "Sensor Status:");
    buffer[LCD_COLS] = '\0';
    for (uint8_t i = 0; i < _numDetectedLcds; ++i) {
        if (!_i2cErrors[i] && _lcds[i]) printDiff(i, buffer, 0);
    }

    snprintf(buffer, sizeof(buffer), "IMU:%-6s Tension:%-3s", imuValid ? "OK" : "ERR", tensionValid ? "OK" : "ERR");
    buffer[LCD_COLS] = '\0';
     for (uint8_t i = 0; i < _numDetectedLcds; ++i) {
        if (!_i2cErrors[i] && _lcds[i]) printDiff(i, buffer, 1);
    }

    snprintf(buffer, sizeof(buffer), "Vent:%-6s", windValid ? "OK" : "N/A"); // Vent peut prendre plus de place
    buffer[LCD_COLS] = '\0';
     for (uint8_t i = 0; i < _numDetectedLcds; ++i) {
        if (!_i2cErrors[i] && _lcds[i]) printDiff(i, buffer, 2);
    }

    // Ligne 3 libre ou pour autre statut
    clearLine(3);

}

// Contrôle du rétroéclairage sur tous les écrans
void LcdModule::setBacklight(bool on) {
    if (!_anyLcdInitialized) return;

    for (uint8_t i = 0; i < _numDetectedLcds; ++i) {
        if (!_i2cErrors[i] && _lcds[i]) {
            if (on) {
                _lcds[i]->backlight();
            } else {
                _lcds[i]->noBacklight();
            }
        }
    }
}

// Retourne le nombre d'écrans détectés
uint8_t LcdModule::getNumDetectedLcds() const {
    return _numDetectedLcds;
}

// Supprimé: getAddress()

// Méthode privée pour afficher les différences sur un écran spécifique
void LcdModule::printDiff(uint8_t lcdIndex, const char* message, uint8_t row) {
    // Vérifications de base
    if (lcdIndex >= _numDetectedLcds || !_lcds[lcdIndex] || row >= LCD_ROWS) {
        return;
    }
    
    // Vérifier l'état I2C avant d'écrire
    Wire.beginTransmission(_addrs[lcdIndex]);
    uint8_t error = Wire.endTransmission();
    if (error != 0) {
        _i2cErrors[lcdIndex] = true;
        LOG_ERROR("LCD", "Erreur I2C détectée sur l'écran %d à l'adresse 0x%02X", lcdIndex, _addrs[lcdIndex]);
        return;
    }
    

    LiquidCrystal_I2C* lcd = _lcds[lcdIndex];
    std::vector<char>& lastRowBuffer = _lastLcdBuffers[lcdIndex][row];

    size_t len = strlen(message);
    if (len > LCD_COLS) len = LCD_COLS; // Tronquer si nécessaire

    bool charWritten = false; // Pour savoir si on a besoin de setCursor

    for (size_t col = 0; col < len; col++) {
        if (lastRowBuffer[col] != message[col]) {
            if (!charWritten) { // Premier changement sur la ligne
                lcd->setCursor(col, row);
                charWritten = true;
            }
            lcd->write(message[col]);
            lastRowBuffer[col] = message[col];
        } else if (charWritten) {
            // Si le caractère précédent a été écrit, mais celui-ci est identique,
            // il faut quand même déplacer le curseur implicite du LCD.
            // On peut soit faire setCursor(col + 1, row) soit écrire le même caractère.
            // Écrire le même caractère est souvent plus simple.
             lcd->write(message[col]); // Réécrire pour avancer le curseur interne du LCD
        }
    }

    // Effacer les caractères restants sur la ligne si la nouvelle chaîne est plus courte
    for (size_t col = len; col < LCD_COLS; col++) {
        if (lastRowBuffer[col] != ' ') {
             if (!charWritten) { // Premier changement sur la ligne (effacement)
                lcd->setCursor(col, row);
                charWritten = true;
            }
            lcd->write(' ');
            lastRowBuffer[col] = ' ';
        } else if (charWritten) {
             lcd->write(' '); // Réécrire pour avancer le curseur interne du LCD
        }
    }
    // Le '\0' est déjà géré par l'initialisation du buffer
}


// Surcharge pour accepter les chaînes Flash (F("..."))
void LcdModule::printDiff(uint8_t lcdIndex, const __FlashStringHelper* message, uint8_t row) {
    // Vérifications de base
    if (lcdIndex >= _numDetectedLcds || !_lcds[lcdIndex] || _i2cErrors[lcdIndex] || row >= LCD_ROWS) {
        return;
    }
    // Copier la chaîne Flash dans un buffer temporaire
    char buffer[LCD_COLS + 1];
    strncpy_P(buffer, (const char*)message, LCD_COLS);
    buffer[LCD_COLS] = '\0'; // Assurer la terminaison null
    printDiff(lcdIndex, buffer, row); // Appeler la version char*
}


// Définir les caractères personnalisés pour un écran spécifique
void LcdModule::defineCustomCharacters(uint8_t lcdIndex) {
     if (lcdIndex >= _numDetectedLcds || !_lcds[lcdIndex] || _i2cErrors[lcdIndex]) {
        return;
    }
    LiquidCrystal_I2C* lcd = _lcds[lcdIndex];

    // Caractère d'avertissement
    static const uint8_t warnChar[8] = {
        0b00100, 0b01110, 0b01110, 0b01110, 0b01110, 0b00000, 0b00100, 0b00000
    };
    // Caractère d'erreur (X)
    static const uint8_t errorChar[8] = {
        0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b00000, 0b00000, 0b00000
    };
    // Caractère OK (✓)
    static const uint8_t okChar[8] = {
        0b00000, 0b00001, 0b00010, 0b10100, 0b01000, 0b00000, 0b00000, 0b00000
    };
    // Caractère flèche
    static const uint8_t arrowChar[8] = {
        0b00000, 0b00100, 0b00110, 0b11111, 0b00110, 0b00100, 0b00000, 0b00000
    };

    // Créer les caractères personnalisés
    lcd->createChar(CHAR_WARNING, (uint8_t*)warnChar);
    lcd->createChar(CHAR_ERROR, (uint8_t*)errorChar);
    lcd->createChar(CHAR_OK, (uint8_t*)okChar);
    lcd->createChar(CHAR_ARROW, (uint8_t*)arrowChar);
}
