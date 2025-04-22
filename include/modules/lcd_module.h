/*
 * Module de gestion de l'écran LCD pour le projet Kite Pilote
 * 
 * Contrôle l'affichage sur un écran LCD 20x4 caractères via I2C.
 * Fournit des fonctions pour afficher des informations de manière organisée.
 * Inclut des mécanismes de détection d'erreur et de récupération.
 * 
 * Créé le: 17/04/2025
 * Mis à jour le: 21/04/2025
 */

#ifndef LCD_MODULE_H
#define LCD_MODULE_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "../core/config.h"
#include "../utils/logger.h"

/**
 * @class LcdModule
 * @brief Gère l'affichage sur l'écran LCD du système
 * 
 * Offre des fonctions pour afficher des informations formatées,
 * créer des interfaces utilisateur simples et afficher l'état du système.
 */
class LcdModule {
public:
    // Caractères personnalisés pour l'interface
    static const uint8_t CHAR_WARNING = 0;  // Symbole d'avertissement
    static const uint8_t CHAR_ERROR = 1;    // Symbole d'erreur
    static const uint8_t CHAR_OK = 2;       // Symbole OK
    static const uint8_t CHAR_ARROW = 3;    // Flèche
    
    // Adresses I2C possibles pour les écrans LCD
    static const uint8_t COMMON_LCD_ADDRESSES[4];
    
    // Constructeur
    LcdModule() : 
        _lcd(nullptr),
        _initialized(false),
        _i2cError(false),
        _lastRefreshTime(0),
        _addr(0)
    {
        // Initialisation complète dans begin()
    }

    // Constructeur avec adresse personnalisable
    LcdModule(uint8_t addr, uint8_t cols = LCD_COLS, uint8_t rows = LCD_ROWS)
        : _lcd(nullptr), _initialized(false), _i2cError(false), _lastRefreshTime(0), _addr(addr) {
        // Initialisation complète dans begin()
    }
    
    // Destructeur pour libérer la mémoire allouée
    ~LcdModule() {
        if (_lcd != nullptr) {
            delete _lcd;
            _lcd = nullptr;
        }
    }
    
    /**
     * @brief Recherche les écrans LCD disponibles sur le bus I2C
     * @return Adresse I2C du premier écran LCD trouvé, 0 si aucun
     */
    static uint8_t scanForLcd() {
        LOG_INFO("LCD", "Scan du bus I2C pour les écrans LCD...");
        
        // Initialiser I2C si ce n'est pas déjà fait
        Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
        
        // Tester d'abord les adresses les plus courantes
        for (uint8_t i = 0; i < sizeof(COMMON_LCD_ADDRESSES) / sizeof(COMMON_LCD_ADDRESSES[0]); i++) {
            uint8_t addr = COMMON_LCD_ADDRESSES[i];
            Wire.beginTransmission(addr);
            uint8_t error = Wire.endTransmission();
            
            if (error == 0) {
                LOG_INFO("LCD", "Écran LCD détecté à l'adresse 0x%02X", addr);
                return addr;
            }
        }
        
        // Si aucun écran n'a été trouvé aux adresses courantes, scan complet
        LOG_INFO("LCD", "Aucun écran LCD trouvé aux adresses courantes. Scan complet...");
        for (uint8_t addr = 0x20; addr <= 0x3F; addr++) {
            Wire.beginTransmission(addr);
            uint8_t error = Wire.endTransmission();
            
            if (error == 0) {
                LOG_INFO("LCD", "Écran LCD détecté à l'adresse 0x%02X", addr);
                return addr;
            }
        }
        
        LOG_WARNING("LCD", "Aucun écran LCD détecté sur le bus I2C");
        return 0;
    }

    /**
     * @brief Initialise l'écran LCD
     * @return true si l'initialisation a réussi, false sinon
     */
    bool begin() {
        // Si aucune adresse n'est spécifiée, rechercher un écran LCD automatiquement
        if (_addr == 0) {
            _addr = scanForLcd();
            if (_addr == 0) {
                LOG_ERROR("LCD", "Aucun écran LCD détecté, impossible de continuer");
                return false;
            }
        }
        
        LOG_INFO("LCD", "Initialisation de l'écran LCD (adresse 0x%02X)...", _addr);
        
        // Initialiser I2C avec les broches définies
        Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
        
        // Vérifier si l'écran LCD est présent sur le bus I2C
        Wire.beginTransmission(_addr);
        _i2cError = (Wire.endTransmission() != 0);
        
        if (_i2cError) {
            LOG_ERROR("LCD", "Échec de communication avec l'écran LCD à l'adresse 0x%02X", _addr);
            return false;
        }
        
        // Créer l'instance de LCD
        _lcd = new LiquidCrystal_I2C(_addr, LCD_COLS, LCD_ROWS);
        if (!_lcd) {
            LOG_ERROR("LCD", "Échec d'allocation mémoire pour l'écran LCD");
            return false;
        }
        
        // Initialiser l'écran LCD
        _lcd->init();
        _lcd->backlight();
        
        // Définir les caractères personnalisés
        defineCustomCharacters();
        
        // Afficher un message de démarrage
        _lcd->clear();
        _lcd->setCursor(0, 0);
        _lcd->print(F("Kite Pilote"));
        _lcd->setCursor(0, 1);
        _lcd->print(F("Version " VERSION_STRING));
        _lcd->setCursor(0, 3);
        _lcd->print(F("Initialisation..."));
        
        _initialized = true;
        _lastRefreshTime = millis();
        LOG_INFO("LCD", "Écran LCD initialisé avec succès");
        
        return true;
    }
    
    /**
     * @brief Teste la connexion avec l'écran LCD
     * @return true si l'écran LCD répond, false sinon
     */
    bool testConnection() {
        if (!_initialized) return false;
        
        // Tenter de communiquer avec l'écran
        Wire.beginTransmission(_addr);
        bool error = (Wire.endTransmission() != 0);
        
        if (error && !_i2cError) {
            LOG_ERROR("LCD", "Connexion à l'écran LCD perdue (0x%02X)", _addr);
            _i2cError = true;
            return false;
        } else if (!error && _i2cError) {
            LOG_INFO("LCD", "Connexion à l'écran LCD rétablie (0x%02X)", _addr);
            _i2cError = false;
        }
        
        return !_i2cError;
    }
    
    /**
     * @brief Affiche un message à une position spécifique
     * @param message Texte à afficher
     * @param col Colonne (0-19)
     * @param row Ligne (0-3)
     * @return true si l'opération a réussi
     */
    bool print(const char* message, uint8_t col = 0, uint8_t row = 0) {
        if (!_initialized || _i2cError || !_lcd) return false;
        
        if (col == 0 && strlen(message) <= LCD_COLS) {
            printDiff(message, row);
            return true;
        }
        
        // Cas général (affichage partiel ou colonne > 0)
        if (col >= LCD_COLS || row >= LCD_ROWS) {
            LOG_WARNING("LCD", "Coordonnées hors limites: %d,%d", col, row);
            return false;
        }
        
        _lcd->setCursor(col, row);
        _lcd->print(message);
        
        // Mettre à jour le tampon _lastLcd si affichage complet sur la ligne
        size_t len = strlen(message);
        if (col == 0 && len <= LCD_COLS) {
            strncpy(_lastLcd[row], message, LCD_COLS);
            for (size_t i = len; i < LCD_COLS; i++) _lastLcd[row][i] = ' ';
            _lastLcd[row][LCD_COLS] = '\0';
        }
        
        return true;
    }
    
    /**
     * @brief Version surchargée pour accepter les chaînes Flash (F())
     */
    bool print(const __FlashStringHelper* message, uint8_t col = 0, uint8_t row = 0) {
        if (!_initialized || _i2cError || !_lcd) return false;
        
        // Vérifier les limites
        if (col >= LCD_COLS || row >= LCD_ROWS) {
            LOG_WARNING("LCD", "Coordonnées hors limites: %d,%d", col, row);
            return false;
        }
        
        _lcd->setCursor(col, row);
        _lcd->print(message);
        return true;
    }
    
    /**
     * @brief Efface une ligne entière
     * @param row Ligne à effacer (0-3)
     */
    void clearLine(uint8_t row) {
        if (!_initialized || _i2cError || !_lcd || row >= LCD_ROWS) return;
        
        char spaces[LCD_COLS + 1];
        memset(spaces, ' ', LCD_COLS);
        spaces[LCD_COLS] = '\0';
        
        _lcd->setCursor(0, row);
        _lcd->print(spaces);
    }
    
    /**
     * @brief Efface tout l'écran
     */
    void clear() {
        if (!_initialized || _i2cError || !_lcd) return;
        
        _lcd->clear();
    }
    
    /**
     * @brief Rafraîchit l'écran si nécessaire (lors de problèmes I2C)
     * @return true si un rafraîchissement a été effectué
     */
    bool refresh() {
        if (!_initialized || !_lcd) return false;
        
        unsigned long currentTime = millis();
        
        // Vérifier la connexion périodiquement
        if (currentTime - _lastRefreshTime > 5000) {  // Toutes les 5 secondes
            _lastRefreshTime = currentTime;
            
            // Tester la connexion et réinitialiser si nécessaire
            if (_i2cError) {
                LOG_INFO("LCD", "Tentative de reconnexion à l'écran LCD (0x%02X)...", _addr);
                Wire.beginTransmission(_addr);
                if (Wire.endTransmission() == 0) {
                    LOG_INFO("LCD", "Connexion rétablie, réinitialisation de l'écran (0x%02X)", _addr);
                    _lcd->init();
                    _lcd->backlight();
                    defineCustomCharacters();
                    _i2cError = false;
                    return true;
                }
            }
        }
        
        return false;
    }
    
    /**
     * @brief Affiche une barre de progression
     * @param row Ligne où afficher la barre (0-3)
     * @param percentage Pourcentage de progression (0-100)
     */
    void showProgressBar(uint8_t row, int percentage) {
        if (!_initialized || _i2cError || !_lcd || row >= LCD_ROWS) return;
        
        percentage = constrain(percentage, 0, 100);
        int filledChars = map(percentage, 0, 100, 0, LCD_COLS);
        char buffer[21];
        
        for (int i = 0; i < LCD_COLS - 4; i++) {
            buffer[i] = (i < filledChars) ? 0xFF : '-';
        }
        snprintf(buffer + LCD_COLS - 4, 5, "%3d%%", percentage);
        buffer[LCD_COLS] = '\0';
        printDiff(buffer, row);
    }
    
    /**
     * @brief Affiche une valeur numérique avec son nom et unité
     * @param name Nom du paramètre
     * @param value Valeur à afficher
     * @param unit Unité de mesure
     * @param row Ligne où afficher (0-3)
     * @param precision Nombre de décimales (défaut: 1)
     */
    void showValue(const char* name, float value, const char* unit, uint8_t row, uint8_t precision = 1) {
        if (!_initialized || _i2cError || !_lcd || row >= LCD_ROWS) return;
        
        char buffer[21];
        char format[10];
        snprintf(format, sizeof(format), "%%.%df", precision);
        char fullFormat[32];
        snprintf(fullFormat, sizeof(fullFormat), "%%-%ds %s %%-%ds", 9, format, 4);
        snprintf(buffer, sizeof(buffer), fullFormat, name, value, unit);
        printDiff(buffer, row);
    }
    
    /**
     * @brief Affiche un message d'état centré
     * @param status Message à afficher
     * @param row Ligne où afficher (0-3)
     * @param icon Caractère spécial à ajouter avant le message (0-255, 255 = aucun)
     */
    void showStatus(const char* status, uint8_t row, uint8_t icon = 255) {
        if (!_initialized || _i2cError || !_lcd || row >= LCD_ROWS) return;
        
        char buffer[21];
        int statusLen = strlen(status);
        int totalLen = statusLen;
        if (icon != 255) totalLen += 2;
        int spaces = (LCD_COLS - totalLen) / 2;
        spaces = max(0, spaces);
        int pos = 0;
        
        if (spaces > 0) {
            memset(buffer, ' ', spaces);
            pos += spaces;
        }
        if (icon != 255) {
            buffer[pos++] = icon;
            buffer[pos++] = ' ';
        }
        strncpy(buffer + pos, status, LCD_COLS - pos);
        pos += statusLen;
        if (pos < LCD_COLS) memset(buffer + pos, ' ', LCD_COLS - pos);
        buffer[LCD_COLS] = '\0';
        printDiff(buffer, row);
    }
    
    /**
     * @brief Affiche un écran complet d'état du système
     * @param mode Message de mode ou statut actuel
     * @param roll Angle de roulis
     * @param pitch Angle de tangage
     * @param tension Tension de la ligne
     * @param power Puissance générée
     */
    void showSystemScreen(const char* mode, float roll, float pitch, float tension, float power) {
        if (!_initialized || _i2cError || !_lcd) return;
        
        // Ligne 1: Mode avec icône si statut OK
        uint8_t icon = (roll >= -60 && roll <= 60 && pitch >= -60 && pitch <= 60) ? CHAR_OK : 255;
        char buffer[21];
        snprintf(buffer, sizeof(buffer), "Mode: %s", mode);
        if (icon != 255) {
            int len = strlen(buffer);
            if (len < LCD_COLS) buffer[LCD_COLS - 1] = icon;
            buffer[LCD_COLS] = '\0';
        }
        printDiff(buffer, 0);
        
        snprintf(buffer, sizeof(buffer), "R:%5.1f\xDF  P:%5.1f\xDF", roll, pitch);
        printDiff(buffer, 1);
        
        snprintf(buffer, sizeof(buffer), "Tension: %5.1f N", tension);
        printDiff(buffer, 2);
        
        snprintf(buffer, sizeof(buffer), "Puissance: %5.1f W", power);
        printDiff(buffer, 3);
    }
    
    /**
     * @brief Affiche un écran d'erreur
     * @param title Titre de l'erreur
     * @param message Message d'erreur principal
     * @param code Code d'erreur numérique (optionnel)
     */
    void showErrorScreen(const char* title, const char* message, int code = -1) {
        if (!_initialized || _i2cError || !_lcd) return;
        
        // Titre de l'erreur centré avec icône
        showStatus(title, 0, CHAR_ERROR);
        
        // Message d'erreur (peut tenir sur 2 lignes)
        char buffer[LCD_COLS + 1];
        if (strlen(message) <= LCD_COLS) {
            strncpy(buffer, message, LCD_COLS);
            buffer[LCD_COLS] = '\0';
            printDiff(buffer, 1);
            if (code >= 0) {
                snprintf(buffer, sizeof(buffer), "Code: %d", code);
                printDiff(buffer, 2);
            }
        } else {
            strncpy(buffer, message, LCD_COLS);
            buffer[LCD_COLS] = '\0';
            printDiff(buffer, 1);
            strncpy(buffer, message + LCD_COLS, LCD_COLS);
            buffer[LCD_COLS] = '\0';
            printDiff(buffer, 2);
            if (code >= 0) {
                snprintf(buffer, sizeof(buffer), "Code: %d", code);
                printDiff(buffer, 3);
            }
        }
    }
    
    /**
     * @brief Affiche l'état des capteurs principaux
     * @param imuValid État de validité de l'IMU
     * @param tensionValid État de validité du capteur de tension
     * @param windValid État de validité des données de vent
     */
    void showSensorStatus(bool imuValid, bool tensionValid, bool windValid) {
        if (!_initialized || _i2cError || !_lcd) return;
        
        char buffer[21];
        snprintf(buffer, sizeof(buffer), "État des capteurs:");
        printDiff(buffer, 0);
        
        snprintf(buffer, sizeof(buffer), "IMU:      %s", imuValid ? "OK" : "ERR");
        printDiff(buffer, 1);
        
        snprintf(buffer, sizeof(buffer), "Tension:  %s", tensionValid ? "OK" : "ERR");
        printDiff(buffer, 2);
        
        snprintf(buffer, sizeof(buffer), "Vent:     %s", windValid ? "OK" : "N/A");
        printDiff(buffer, 3);
    }
    
    /**
     * @brief Contrôle du rétroéclairage
     * @param on true pour allumer, false pour éteindre
     */
    void setBacklight(bool on) {
        if (!_initialized || _i2cError || !_lcd) return;
        
        if (on) {
            _lcd->backlight();
        } else {
            _lcd->noBacklight();
        }
    }
    
    /**
     * @brief Obtenir l'adresse I2C de l'écran LCD
     * @return Adresse I2C (0 si non initialisé)
     */
    uint8_t getAddress() {
        return _addr;
    }
    
private:
    LiquidCrystal_I2C* _lcd;  // Pointeur vers l'instance du pilote LCD
    bool _initialized;        // État d'initialisation
    bool _i2cError;           // Indicateur de problème I2C
    unsigned long _lastRefreshTime; // Dernière tentative de rafraîchissement
    char _lastLcd[4][21] = {"", "", "", ""}; // Tampon de l'affichage précédent (20+1)
    uint8_t _addr;            // Adresse I2C propre à chaque instance

    /**
     * @brief Affiche une chaîne sur une ligne en ne modifiant que les caractères différents
     * @param message Texte à afficher (max 20 caractères)
     * @param row Ligne (0-3)
     */
    void printDiff(const char* message, uint8_t row) {
        if (!_initialized || _i2cError || !_lcd || row >= LCD_ROWS) return;
        
        size_t len = strlen(message);
        if (len > LCD_COLS) len = LCD_COLS;
        
        for (size_t col = 0; col < len; col++) {
            if (_lastLcd[row][col] != message[col]) {
                _lcd->setCursor(col, row);
                _lcd->write(message[col]);
                _lastLcd[row][col] = message[col];
            }
        }
        
        // Effacer les caractères restants si la nouvelle chaîne est plus courte
        for (size_t col = len; col < LCD_COLS; col++) {
            if (_lastLcd[row][col] != ' ') {
                _lcd->setCursor(col, row);
                _lcd->write(' ');
                _lastLcd[row][col] = ' ';
            }
        }
        _lastLcd[row][LCD_COLS] = '\0';
    }

    // Surcharge pour accepter les chaînes Flash (F("..."))
    void printDiff(const __FlashStringHelper* message, uint8_t row) {
        char buffer[LCD_COLS + 1];
        strncpy_P(buffer, (const char*)message, LCD_COLS);
        buffer[LCD_COLS] = '\0';
        printDiff(buffer, row);
    }

    /**
     * @brief Définit les caractères personnalisés dans la mémoire CGRAM du LCD
     */
    void defineCustomCharacters() {
        if (!_lcd) return;
        
        // Caractère d'avertissement
        static const uint8_t warnChar[8] = {
            0b00100,
            0b01110,
            0b01110,
            0b01110,
            0b01110,
            0b00000,
            0b00100,
            0b00000
        };
        
        // Caractère d'erreur (X)
        static const uint8_t errorChar[8] = {
            0b10001,
            0b01010,
            0b00100,
            0b01010,
            0b10001,
            0b00000,
            0b00000,
            0b00000
        };
        
        // Caractère OK (✓)
        static const uint8_t okChar[8] = {
            0b00000,
            0b00001,
            0b00010,
            0b10100,
            0b01000,
            0b00000,
            0b00000,
            0b00000
        };
        
        // Caractère flèche
        static const uint8_t arrowChar[8] = {
            0b00000,
            0b00100,
            0b00110,
            0b11111,
            0b00110,
            0b00100,
            0b00000,
            0b00000
        };
        
        // Créer les caractères personnalisés
        _lcd->createChar(CHAR_WARNING, (uint8_t*)warnChar);
        _lcd->createChar(CHAR_ERROR, (uint8_t*)errorChar);
        _lcd->createChar(CHAR_OK, (uint8_t*)okChar);
        _lcd->createChar(CHAR_ARROW, (uint8_t*)arrowChar);
    }
};

// Définition des adresses I2C courantes pour les écrans LCD

#endif // LCD_MODULE_H
