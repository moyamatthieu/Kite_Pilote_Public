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
    LcdModule();

    // Constructeur avec adresse personnalisable
    LcdModule(uint8_t addr, uint8_t cols, uint8_t rows);
    
    // Destructeur pour libérer la mémoire allouée
    ~LcdModule();
    
    /**
     * @brief Recherche les écrans LCD disponibles sur le bus I2C
     * @return Adresse I2C du premier écran LCD trouvé, 0 si aucun
     */
    static uint8_t scanForLcd();

    /**
     * @brief Initialise l'écran LCD
     * @return true si l'initialisation a réussi, false sinon
     */
    bool begin();
    
    /**
     * @brief Teste la connexion avec l'écran LCD
     * @return true si l'écran LCD répond, false sinon
     */
    bool testConnection();
    
    /**
     * @brief Affiche un message à une position spécifique
     * @param message Texte à afficher
     * @param col Colonne (0-19)
     * @param row Ligne (0-3)
     * @return true si l'opération a réussi
     */
    bool print(const char* message, uint8_t col, uint8_t row);
    
    /**
     * @brief Version surchargée pour accepter les chaînes Flash (F())
     */
    bool print(const __FlashStringHelper* message, uint8_t col, uint8_t row);
    
    /**
     * @brief Efface une ligne entière
     * @param row Ligne à effacer (0-3)
     */
    void clearLine(uint8_t row);
    
    /**
     * @brief Efface tout l'écran
     */
    void clear();
    
    /**
     * @brief Rafraîchit l'écran si nécessaire (lors de problèmes I2C)
     * @return true si un rafraîchissement a été effectué
     */
    bool refresh();
    
    /**
     * @brief Affiche une barre de progression
     * @param row Ligne où afficher la barre (0-3)
     * @param percentage Pourcentage de progression (0-100)
     */
    void showProgressBar(uint8_t row, int percentage);
    
    /**
     * @brief Affiche une valeur numérique avec son nom et unité
     * @param name Nom du paramètre
     * @param value Valeur à afficher
     * @param unit Unité de mesure
     * @param row Ligne où afficher (0-3)
     * @param precision Nombre de décimales (défaut: 1)
     */
    void showValue(const char* name, float value, const char* unit, uint8_t row, uint8_t precision = 1);
    
    /**
     * @brief Affiche un message d'état centré
     * @param status Message à afficher
     * @param row Ligne où afficher (0-3)
     * @param icon Caractère spécial à ajouter avant le message (0-255, 255 = aucun)
     */
    void showStatus(const char* status, uint8_t row, uint8_t icon = 255);
    
    /**
     * @brief Affiche un écran complet d'état du système
     * @param mode Message de mode ou statut actuel
     * @param roll Angle de roulis
     * @param pitch Angle de tangage
     */
    void showSystemScreen(const char* mode, float roll, float pitch, float tension, float power);
    
    /**
     * @brief Affiche un écran d'erreur
     * @param title Titre de l'erreur
     * @param message Message d'erreur principal
     * @param code Code d'erreur numérique (optionnel)
     */
    void showErrorScreen(const char* title, const char* message, int code = -1);
    
    /**
     * @brief Affiche l'état des capteurs principaux
     * @param imuValid État de validité de l'IMU
     * @param tensionValid État de validité du capteur de tension
     * @param windValid État de validité des données de vent
     */
    void showSensorStatus(bool imuValid, bool tensionValid, bool windValid);
    
    /**
     * @brief Contrôle du rétroéclairage
     * @param on true pour allumer, false pour éteindre
     */
    void setBacklight(bool on);
    
    /**
     * @brief Obtenir l'adresse I2C de l'écran LCD
     * @return Adresse I2C (0 si non initialisé)
     */
    uint8_t getAddress();

private:
    LiquidCrystal_I2C* _lcd;  // Pointeur vers l'instance du pilote LCD
    bool _initialized;        // État d'initialisation
    bool _i2cError;           // Indicateur de problème I2C
    unsigned long _lastRefreshTime; // Dernière tentative de rafraîchissement
    char _lastLcd[4][21]; // Tampon de l'affichage précédent (20+1)
    uint8_t _addr;            // Adresse I2C propre à chaque instance

    /**
     * @brief Affiche une chaîne sur une ligne en ne modifiant que les caractères différents
     * @param message Texte à afficher (max 20 caractères)
     */
    void printDiff(const char* message, uint8_t row);

    // Surcharge pour accepter les chaînes Flash (F("..."))
    void printDiff(const __FlashStringHelper* message, uint8_t row);

    /**
     * @brief Définit les caractères personnalisés dans la mémoire CGRAM du LCD
     */
    void defineCustomCharacters();
};

#endif
