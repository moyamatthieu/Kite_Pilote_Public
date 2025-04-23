/*
 * Configuration globale pour le projet Kite Pilote
 * 
 * Ce fichier contient tous les paramètres de configuration du système,
 * organisés en sections pour faciliter la compréhension.
 * 
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

//===============================================================
// SECTION 1: INFORMATIONS DE VERSION
//===============================================================
#define VERSION_MAJOR 2             // Version majeure
#define VERSION_MINOR 0             // Version mineure
#define VERSION_PATCH 0             // Version correctif
#define VERSION_BUILD 5             // Numéro de build
#define VERSION_STRING "v2.0.0.5"   // Chaîne de version complète
#define FIRMWARE_VERSION VERSION_STRING // Alias pour compatibilité
#define BUILD_DATE "23/04/2025"     // Date de la dernière modification

//===============================================================
// SECTION 2: CONFIGURATION GÉNÉRALE ET JOURNALISATION
//===============================================================
#define DEBUG_ENABLED true          // Active/désactive les messages de débogage
#define SERIAL_BAUD_RATE 115200     // Vitesse de communication série

// Niveaux de journalisation
#define LOG_LEVEL_NONE 0            // Aucun log
#define LOG_LEVEL_ERROR 1           // Erreurs uniquement
#define LOG_LEVEL_WARNING 2         // Erreurs et avertissements
#define LOG_LEVEL_INFO 3            // Erreurs, avertissements et infos
#define LOG_LEVEL_DEBUG 4           // Tout (y compris messages de débogage)
#define LOG_LEVEL_TRACE 5           // Niveau le plus détaillé

// Niveau de journalisation actuel
#define LOG_LEVEL LOG_LEVEL_INFO    // Niveau par défaut: INFO

// Taille du buffer circulaire de logs
#define LOG_BUFFER_SIZE 50          // Nombre de messages récents à conserver

// Active le mode simulation pour Wokwi
// Commentez cette ligne pour utiliser les capteurs réels
#ifndef SIMULATION_MODE
    #define SIMULATION_MODE
#endif

//===============================================================
// SECTION 3: PARAMÈTRES DE SÉCURITÉ ET LIMITES
//===============================================================

// Limites de sécurité pour la tension des lignes (en Newtons)
const float MAX_SAFE_TENSION = 500.0f;   // Tension maximale sécuritaire
const float WARNING_TENSION = 400.0f;    // Seuil d'avertissement pour la tension

// Limites de sécurité pour le vent (en m/s)
const float MIN_SAFE_WIND_SPEED = 3.0f;  // Vitesse minimale pour voler
const float MAX_SAFE_WIND_SPEED = 15.0f; // Vitesse maximale pour voler
const float MAX_SAFE_GUST_SPEED = 20.0f; // Vitesse maximale des rafales

// Seuils de batterie (en volts)
const float BATTERY_MIN_VOLTAGE = 11.0f; // Tension minimale de la batterie
const float BATTERY_LOW_VOLTAGE = 11.5f; // Tension faible de la batterie (avertissement)
const float BATTERY_FULL_VOLTAGE = 12.6f;// Tension batterie pleine

// Paramètres de mémoire et performances
const uint32_t MIN_FREE_MEMORY = 10000;  // Mémoire libre minimale (octets)
const uint32_t WATCHDOG_TIMEOUT_MS = 5000; // Délai du watchdog (ms)

//===============================================================
// SECTION 4: BROCHES (PINS) ESP32
//===============================================================

// --- LEDS ---
const uint8_t LED_GREEN_PIN = 16;   // Pin pour la LED verte (statut)
const uint8_t LED_RED_PIN = 17;     // Pin pour la LED rouge (erreur)

// --- ÉCRAN LCD ---
const uint8_t LCD_I2C_ADDR = 0x27;  // Adresse I2C du LCD
#define LCD1_I2C_ADDR 0x27          // Adresse I2C du premier écran LCD
#define LCD2_I2C_ADDR 0x3F          // Adresse I2C du second écran LCD

const uint8_t LCD_COLS = 20;        // 20 colonnes
const uint8_t LCD_ROWS = 4;         // 4 lignes
const uint8_t I2C_SDA_PIN = 21;     // Pin SDA pour I2C (LCD)
const uint8_t I2C_SCL_PIN = 22;     // Pin SCL pour I2C (LCD)

// --- SERVOMOTEURS ---
const uint8_t SERVO_DIRECTION_PIN = 23;  // Servo pour direction gauche/droite
const uint8_t SERVO_TRIM_PIN = 27;       // Servo pour trim (angle d'incidence)

// --- MOTEUR PAS À PAS (STEPPER) ---
const uint8_t STEPPER_WINCH_PIN1 = 14;   // Pin 1 pour le moteur pas à pas (IN1)
const uint8_t STEPPER_WINCH_PIN2 = 12;   // Pin 2 pour le moteur pas à pas (IN2)
const uint8_t STEPPER_WINCH_PIN3 = 13;   // Pin 3 pour le moteur pas à pas (IN3)
const uint8_t STEPPER_WINCH_PIN4 = 15;   // Pin 4 pour le moteur pas à pas (IN4)

// --- PINS DU MODE SIMULATION (POTENTIOMÈTRES) ---
#ifdef SIMULATION_MODE
const uint8_t SIM_ROLL_PIN = 34;          // Roulis du kite
const uint8_t SIM_PITCH_PIN = 35;         // Tangage du kite
const uint8_t SIM_YAW_PIN = 32;           // Lacet du kite
const uint8_t SIM_TENSION_PIN = 33;       // Tension de la ligne
const uint8_t SIM_WIND_SPEED_PIN = 36;    // Vitesse du vent
const uint8_t SIM_WIND_DIRECTION_PIN = 39;// Direction du vent

// --- BOUTONS DE SIMULATION ---
const uint8_t SIM_MODE_BUTTON_PIN = 4;    // Bouton de changement de mode
const uint8_t SIM_EMERGENCY_BUTTON_PIN = 5;// Bouton d'arrêt d'urgence
#endif

//===============================================================
// SECTION 4: PARAMÈTRES DES COMPOSANTS
//===============================================================

// --- SERVOMOTEURS ---
const uint16_t SERVO_MIN_PULSE = 500;    // Impulsion minimale (µs)
const uint16_t SERVO_MAX_PULSE = 2500;   // Impulsion maximale (µs)

// Limites du servo de direction
const float DIRECTION_MIN_ANGLE = -45.0; // Angle minimum (gauche)
const float DIRECTION_MAX_ANGLE = 45.0;  // Angle maximum (droite)
const float DIRECTION_CENTER = 0.0;      // Position centrale

// Limites du servo de trim
const float TRIM_MIN_ANGLE = -30.0;      // Angle minimum (min puissance)
const float TRIM_MAX_ANGLE = 30.0;       // Angle maximum (max puissance)
const float TRIM_CENTER = 0.0;           // Position centrale

// Paramètres du moteur pas à pas (stepper) pour le treuil/générateur
const float WINCH_MIN_POWER = 0.0;       // Puissance minimale (0%)
const float WINCH_MAX_POWER = 100.0;     // Puissance maximale (100%)
const uint16_t STEPPER_MAX_SPEED = 1000; // Vitesse maximale en pas/seconde
const uint8_t STEPPER_MICROSTEPS = 1;    // Mode microstepping (1, 2, 4, 8, 16, 32)
const uint16_t STEPPER_STEPS_PER_REV = 200; // Nombre de pas par révolution (standard: 200 pour 1.8° par pas)

// --- LED ---
const unsigned long LED_BLINK_INTERVAL = 1000;  // Intervalle de clignotement (ms)

// --- WIFI ---
#define WIFI_ENABLED true               // Active/désactive le WiFi
#define WIFI_DEFAULT_AP_SSID "KitePilote" // Nom du point d'accès WiFi
#define WIFI_DEFAULT_AP_PASS "KitePilote123" // Mot de passe (min 8 caractères)

// Alias pour compatibilité
#define WIFI_AP_SSID WIFI_DEFAULT_AP_SSID
#define WIFI_AP_PASSWORD WIFI_DEFAULT_AP_PASS
#define WIFI_WEB_PORT 80                // Port du serveur web

//===============================================================
// SECTION 4 bis: IDENTIFIANTS WIFI
//===============================================================
#ifdef SIMULATION_MODE
#define WIFI_SSID "Wokwi-GUEST"       // SSID utilisé par Wokwi
#define WIFI_PASS ""                  // Pas de mot de passe en simulation
#else
#define WIFI_SSID "MMA33"           // SSID du réseau réel
#define WIFI_PASS "0231966925" // Remplacez par le mot de passe réel
#endif

//===============================================================
// SECTION 5: DÉFINITIONS D'ÉTATS ET MODES
//===============================================================

// Modes de fonctionnement du treuil/générateur
enum WinchMode {
  WINCH_MODE_GENERATOR,    // Génération d'énergie
  WINCH_MODE_REELING_IN,   // Enroulement (ramène le kite)
  WINCH_MODE_REELING_OUT,  // Déroulement (déploie le kite)
  WINCH_MODE_BRAKE,        // Frein (maintient la position)
  WINCH_MODE_IDLE          // Repos (pas de résistance)
};

// Modes de l'autopilote
enum AutopilotMode {
  AUTOPILOT_OFF,           // Autopilote désactivé
  AUTOPILOT_STANDBY,       // En attente
  AUTOPILOT_LAUNCH,        // Séquence de décollage
  AUTOPILOT_LAND,          // Séquence d'atterrissage
  AUTOPILOT_EIGHT_PATTERN, // Vol en huit
  AUTOPILOT_CIRCULAR,      // Vol circulaire
  AUTOPILOT_POWER_GENERATION // Optimisé pour la génération d'énergie
};

// Motifs de clignotement des LEDs
enum LedPattern {
  LED_PATTERN_OFF,         // LED éteinte
  LED_PATTERN_ON,          // LED allumée
  LED_PATTERN_SLOW_BLINK,  // Clignotement lent (1Hz)
  LED_PATTERN_FAST_BLINK,  // Clignotement rapide (5Hz)
  LED_PATTERN_ERROR,       // Signal d'erreur (SOS)
  LED_PATTERN_WIFI_CONNECTING, // Connexion WiFi en cours
  LED_PATTERN_WIFI_CONNECTED   // WiFi connecté
};

// Codes d'erreur
enum ErrorCode {
  ERROR_NONE = 0,
  // Erreurs d'initialisation
  ERROR_LCD_INIT = 10,
  ERROR_SERVO_INIT = 11,
  ERROR_WIFI_INIT = 12,
  ERROR_SENSOR_INIT = 13,
  // Erreurs des capteurs
  ERROR_IMU_DATA = 20,
  ERROR_TENSION_DATA = 21,
  ERROR_WIND_DATA = 22,
  // Erreurs des actionneurs
  ERROR_SERVO_CONTROL = 30,
  ERROR_WINCH_CONTROL = 31,
  // Erreurs système
  ERROR_MEMORY_LOW = 40,
  ERROR_WATCHDOG = 41,
  ERROR_EMERGENCY_STOP = 50
};

//===============================================================
// SECTION X: PARAMÈTRES TASKS (FreeRTOS)
//===============================================================
// Tailles de pile (en mots) et priorités des tâches
#define TASK_SENSOR_STACK      2048
#define TASK_SENSOR_PRIORITY   5

#define TASK_CONTROL_STACK     2048
#define TASK_CONTROL_PRIORITY  5

#define TASK_DISPLAY_STACK     2048
#define TASK_DISPLAY_PRIORITY  3

#define TASK_WIFI_STACK        4096
#define TASK_WIFI_PRIORITY     4

#define TASK_DIAG_STACK        2048
#define TASK_DIAG_PRIORITY     2

// Longueurs de queues
#define QUEUE_SENSOR_LENGTH    5
#define QUEUE_CONTROL_LENGTH   5

// Bits pour EventGroup diagnostic
#define BIT_SENSOR_OK          (1 << 0)
#define BIT_CONTROL_OK         (1 << 1)
#define BIT_DISPLAY_OK         (1 << 2)
#define BIT_WIFI_OK            (1 << 3)
#define BIT_ALL_OK             (BIT_SENSOR_OK | BIT_CONTROL_OK | BIT_DISPLAY_OK | BIT_WIFI_OK)

// Timeout de diagnostic (en ms)
#define DIAG_CHECK_INTERVAL_MS 5000

#endif // CONFIG_H
