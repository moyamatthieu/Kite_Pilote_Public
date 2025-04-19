/*
 * Module de gestion de l'écran LCD pour le projet Kite Pilote
 * 
 * Contrôle l'affichage sur un écran LCD 20x4 caractères via I2C.
 * Fournit des fonctions pour afficher des informations de manière organisée.
 * Inclut des mécanismes de détection d'erreur et de récupération.
 * 
 * Créé le: 17/04/2025
 * Mis à jour le: 19/04/2025
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
    
    // Constructeur
    LcdModule() : 
        _lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS), 
        _initialized(false),
        _i2cError(false),
        _lastRefreshTime(0)
    {
        // Initialisation complète dans begin()
    }
    
    /**
     * @brief Initialise l'écran LCD
     * @return true si l'initialisation a réussi, false sinon
     */
    bool begin() {
        LOG_INFO("LCD", "Initialisation de l'écran LCD (adresse 0x%02X)...", LCD_I2C_ADDR);
        
        // Initialiser I2C avec les broches définies
        Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
        
        // Vérifier si l'écran LCD est présent sur le bus I2C
        Wire.beginTransmission(LCD_I2C_ADDR);
        _i2cError = (Wire.endTransmission() != 0);
        
        if (_i2cError) {
            LOG_ERROR("LCD", "Échec de communication avec l'écran LCD à l'adresse 0x%02X", LCD_I2C_ADDR);
            return false;
        }
        
        // Initialiser l'écran LCD
        _lcd.init();
        _lcd.backlight();
        
        // Définir les caractères personnalisés
        defineCustomCharacters();
        
        // Afficher un message de démarrage
        _lcd.clear();
        _lcd.setCursor(0, 0);
        _lcd.print(F("Kite Pilote"));
        _lcd.setCursor(0, 1);
        _lcd.print(F("Version " VERSION_STRING));
        _lcd.setCursor(0, 3);
        _lcd.print(F("Initialisation..."));
        
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
        Wire.beginTransmission(LCD_I2C_ADDR);
        bool error = (Wire.endTransmission() != 0);
        
        if (error && !_i2cError) {
            LOG_ERROR("LCD", "Connexion à l'écran LCD perdue");
            _i2cError = true;
            return false;
        } else if (!error && _i2cError) {
            LOG_INFO("LCD", "Connexion à l'écran LCD rétablie");
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
        if (!_initialized || _i2cError) return false;
        
        // Vérifier les limites
        if (col >= LCD_COLS || row >= LCD_ROWS) {
            LOG_WARNING("LCD", "Coordonnées hors limites: %d,%d", col, row);
            return false;
        }
        
        _lcd.setCursor(col, row);
        _lcd.print(message);
        return true;
    }
    
    /**
     * @brief Version surchargée pour accepter les chaînes Flash (F())
     */
    bool print(const __FlashStringHelper* message, uint8_t col = 0, uint8_t row = 0) {
        if (!_initialized || _i2cError) return false;
        
        // Vérifier les limites
        if (col >= LCD_COLS || row >= LCD_ROWS) {
            LOG_WARNING("LCD", "Coordonnées hors limites: %d,%d", col, row);
            return false;
        }
        
        _lcd.setCursor(col, row);
        _lcd.print(message);
        return true;
    }
    
    /**
     * @brief Efface une ligne entière
     * @param row Ligne à effacer (0-3)
     */
    void clearLine(uint8_t row) {
        if (!_initialized || _i2cError || row >= LCD_ROWS) return;
        
        char spaces[LCD_COLS + 1];
        memset(spaces, ' ', LCD_COLS);
        spaces[LCD_COLS] = '\0';
        
        _lcd.setCursor(0, row);
        _lcd.print(spaces);
    }
    
    /**
     * @brief Efface tout l'écran
     */
    void clear() {
        if (!_initialized || _i2cError) return;
        
        _lcd.clear();
    }
    
    /**
     * @brief Rafraîchit l'écran si nécessaire (lors de problèmes I2C)
     * @return true si un rafraîchissement a été effectué
     */
    bool refresh() {
        if (!_initialized) return false;
        
        unsigned long currentTime = millis();
        
        // Vérifier la connexion périodiquement
        if (currentTime - _lastRefreshTime > 5000) {  // Toutes les 5 secondes
            _lastRefreshTime = currentTime;
            
            // Tester la connexion et réinitialiser si nécessaire
            if (_i2cError) {
                LOG_INFO("LCD", "Tentative de reconnexion à l'écran LCD...");
                Wire.beginTransmission(LCD_I2C_ADDR);
                if (Wire.endTransmission() == 0) {
                    LOG_INFO("LCD", "Connexion rétablie, réinitialisation de l'écran");
                    _lcd.init();
                    _lcd.backlight();
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
        if (!_initialized || _i2cError || row >= LCD_ROWS) return;
        
        // Limiter le pourcentage entre 0 et 100
        percentage = constrain(percentage, 0, 100);
        
        // Calculer le nombre de caractères remplis (sur 20 colonnes)
        int filledChars = map(percentage, 0, 100, 0, LCD_COLS);
        
        // Caractères pour afficher une barre de progression plus fluide
        static const char barChars[] = { ' ', char(0x1), char(0x2), char(0x3), char(0xFF) };
        
        // Partie entièrement remplie
        _lcd.setCursor(0, row);
        for (int i = 0; i < filledChars; i++) {
            _lcd.write(0xFF); // Caractère "bloc plein"
        }
        
        // Partie vide
        for (int i = filledChars; i < LCD_COLS; i++) {
            _lcd.write('-');  // Caractère pour partie vide
        }
        
        // Afficher le pourcentage à la fin
        char buffer[5];
        snprintf(buffer, sizeof(buffer), "%3d%%", percentage);
        _lcd.setCursor(LCD_COLS - 4, row);
        _lcd.print(buffer);
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
        if (!_initialized || _i2cError || row >= LCD_ROWS) return;
        
        char buffer[21]; // Buffer pour stocker le texte formaté (20 colonnes + null)
        char format[10]; // Buffer pour le format
        
        // Construire le format selon la précision demandée
        snprintf(format, sizeof(format), "%%.%df", precision);
        
        // Générer le format complet pour snprintf
        char fullFormat[32];
        snprintf(fullFormat, sizeof(fullFormat), "%%-%ds %s %%-%ds", 
                9, format, 4);
        
        // Formater la chaîne finale
        snprintf(buffer, sizeof(buffer), fullFormat, name, value, unit);
        
        _lcd.setCursor(0, row);
        _lcd.print(buffer);
    }
    
    /**
     * @brief Affiche un message d'état centré
     * @param status Message à afficher
     * @param row Ligne où afficher (0-3)
     * @param icon Caractère spécial à ajouter avant le message (0-255, 255 = aucun)
     */
    void showStatus(const char* status, uint8_t row, uint8_t icon = 255) {
        if (!_initialized || _i2cError || row >= LCD_ROWS) return;
        
        // Effacer la ligne d'abord
        clearLine(row);
        
        // Calculer la longueur totale avec l'icône
        int statusLen = strlen(status);
        int totalLen = statusLen;
        if (icon != 255) totalLen += 2; // Icône + espace
        
        // Centrer le message d'état
        int spaces = (LCD_COLS - totalLen) / 2;
        spaces = max(0, spaces); // Éviter les valeurs négatives
        
        _lcd.setCursor(spaces, row);
        
        // Afficher l'icône si nécessaire
        if (icon != 255) {
            _lcd.write(icon);
            _lcd.write(' ');
        }
        
        _lcd.print(status);
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
        if (!_initialized || _i2cError) return;
        
        clear();
        
        // Utiliser un tampon statique pour économiser la mémoire
        static char buffer[21];
        
        // Ligne 1: Mode avec icône si statut OK
        uint8_t icon = (roll >= -60 && roll <= 60 && pitch >= -60 && pitch <= 60) ? CHAR_OK : 255;
        
        // Afficher le mode avec une icône si tout va bien
        snprintf(buffer, sizeof(buffer), "Mode: %s", mode);
        
        if (icon != 255) {
            print(buffer, 0, 0);
            _lcd.setCursor(LCD_COLS - 1, 0);
            _lcd.write(icon);
        } else {
            print(buffer, 0, 0);
        }
        
        // Ligne 2: Orientation (roll et pitch)
        snprintf(buffer, sizeof(buffer), "R:%5.1f\xDF  P:%5.1f\xDF", roll, pitch);
        print(buffer, 0, 1);
        
        // Ligne 3: Tension de ligne
        snprintf(buffer, sizeof(buffer), "Tension: %5.1f N", tension);
        print(buffer, 0, 2);
        
        // Ligne 4: Puissance générée
        snprintf(buffer, sizeof(buffer), "Puissance: %5.1f W", power);
        print(buffer, 0, 3);
    }
    
    /**
     * @brief Affiche un écran d'erreur
     * @param title Titre de l'erreur
     * @param message Message d'erreur principal
     * @param code Code d'erreur numérique (optionnel)
     */
    void showErrorScreen(const char* title, const char* message, int code = -1) {
        if (!_initialized || _i2cError) return;
        
        clear();
        
        // Titre de l'erreur centré avec icône
        showStatus(title, 0, CHAR_ERROR);
        
        // Message d'erreur (peut tenir sur 2 lignes)
        if (strlen(message) <= LCD_COLS) {
            // Cas simple: message sur une ligne
            print(message, 0, 1);
        } else {
            // Message sur deux lignes
            char buffer[LCD_COLS + 1];
            strncpy(buffer, message, LCD_COLS);
            buffer[LCD_COLS] = '\0';
            print(buffer, 0, 1);
            
            if (strlen(message) > LCD_COLS) {
                strncpy(buffer, message + LCD_COLS, LCD_COLS);
                buffer[LCD_COLS] = '\0';
                print(buffer, 0, 2);
            }
        }
        
        // Afficher le code d'erreur s'il est valide
        if (code >= 0) {
            char buffer[21];
            snprintf(buffer, sizeof(buffer), "Code: %d", code);
            print(buffer, 0, 3);
        }
    }
    
    /**
     * @brief Affiche l'état des capteurs principaux
     * @param imuValid État de validité de l'IMU
     * @param tensionValid État de validité du capteur de tension
     * @param windValid État de validité des données de vent
     */
    void showSensorStatus(bool imuValid, bool tensionValid, bool windValid) {
        if (!_initialized || _i2cError) return;
        
        clear();
        print(F("État des capteurs:"), 0, 0);
        
        // Affichage de l'état avec icône
        print(F("IMU:      "), 0, 1);
        _lcd.setCursor(10, 1);
        _lcd.write(imuValid ? CHAR_OK : CHAR_ERROR);
        print(imuValid ? F(" OK") : F(" ERR"), 12, 1);
        
        print(F("Tension:  "), 0, 2);
        _lcd.setCursor(10, 2);
        _lcd.write(tensionValid ? CHAR_OK : CHAR_ERROR);
        print(tensionValid ? F(" OK") : F(" ERR"), 12, 2);
        
        print(F("Vent:     "), 0, 3);
        _lcd.setCursor(10, 3);
        _lcd.write(windValid ? CHAR_OK : CHAR_WARNING);
        print(windValid ? F(" OK") : F(" N/A"), 12, 3);
    }
    
    /**
     * @brief Contrôle du rétroéclairage
     * @param on true pour allumer, false pour éteindre
     */
    void setBacklight(bool on) {
        if (!_initialized || _i2cError) return;
        
        if (on) {
            _lcd.backlight();
        } else {
            _lcd.noBacklight();
        }
    }
    
private:
    LiquidCrystal_I2C _lcd;   // Instance du pilote LCD
    bool _initialized;        // État d'initialisation
    bool _i2cError;           // Indicateur de problème I2C
    unsigned long _lastRefreshTime; // Dernière tentative de rafraîchissement
    
    /**
     * @brief Définit les caractères personnalisés dans la mémoire CGRAM du LCD
     */
    void defineCustomCharacters() {
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
        _lcd.createChar(CHAR_WARNING, (uint8_t*)warnChar);
        _lcd.createChar(CHAR_ERROR, (uint8_t*)errorChar);
        _lcd.createChar(CHAR_OK, (uint8_t*)okChar);
        _lcd.createChar(CHAR_ARROW, (uint8_t*)arrowChar);
    }
};

#endif // LCD_MODULE_H
