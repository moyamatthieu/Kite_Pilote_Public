/*
 * Programme principal du projet Kite Pilote
 * 
 * Ce fichier est le point d'entrée du programme. Il initialise et coordonne
 * tous les modules du système pour le contrôle automatique d'un kite générateur d'électricité.
 * 
 * Architecture modulaire et extensible pour faciliter la maintenance et l'évolution du projet.
 * 
 * Version: v2.0.0.0 (19/04/2025)
 * Projet: Kite Pilote
 */

#include <Arduino.h>
#include <esp_task_wdt.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_FT6206.h>
#include <WiFi.h>
#include "LittleFS.h"

// Inclusions des modules de base
#include "core/config.h"
#include "core/data_types.h"
#include "utils/logger.h"

#ifdef SIMULATION_MODE
#include "modules/simulation_module.h"
SimulationModule simulation;
#endif

// Inclusions des modules fonctionnels
#include "modules/led_module.h"
#include "modules/lcd_module.h"
#include "modules/autopilot_module.h"
#include "core/tasks.h" // Gestion multitâche
#include "modules/web_interface_module.h" // Interface Web

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

//=============================================================================
// INSTANCES GLOBALES DES MODULES
//=============================================================================

 // Gestionnaire d'affichage des statuts système sur LEDs
LedModule ledStatus(LED_GREEN_PIN, "LED_STATUS");
LedModule ledError(LED_RED_PIN, "LED_ERROR");

// Gestionnaire de l'écran LCD
LcdModule lcd; // LCD principal (détection auto)
LcdModule lcd2; // Deuxième LCD (détection auto)

// Fonction utilitaire pour sélectionner l'écran LCD
LcdModule& getLcd(uint8_t screen = 1) {
    return (screen == 2) ? lcd2 : lcd;
}

// Gestionnaire des capteurs
SensorModule sensors;

// Gestionnaire des servomoteurs
ServoModule servos;

// Gestionnaire de l'autopilote
AutopilotModule autopilot;

// Gestionnaire de l'écran TFT et du tactile
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  4
#define TFT_MOSI 23
#define TFT_CLK  18
#define TFT_MISO 19
#define TFT_LED  21


Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
Adafruit_FT6206 ctp = Adafruit_FT6206();

// Gestionnaire de l'interface web
WebInterfaceModule webInterface;

// Variables système globales
SystemStatus systemStatus;
unsigned long lastDisplayUpdateTime = 0;
unsigned long lastMemoryCheckTime = 0;
unsigned long lastSensorCheckTime = 0;
unsigned long lastHeartbeatTime = 0;

// Intervalles de temps (en millisecondes)
const unsigned long DISPLAY_UPDATE_INTERVAL = 250;   // Mise à jour LCD toutes les 250ms
const unsigned long MEMORY_CHECK_INTERVAL = 10000;   // Vérification mémoire toutes les 10 secondes
const unsigned long SENSOR_CHECK_INTERVAL = 5000;    // Vérification capteurs toutes les 5 secondes
const unsigned long HEARTBEAT_INTERVAL = 5000;       // Log d'activité toutes les 5 secondes

// Tampon statique pour les chaînes de caractères formatées
char messageBuffer[64];  // Tampon pour éviter les allocations dynamiques de String

// Fonctions de callback pour l'interface web
void gererChangementMode(AutopilotMode nouveauMode) {
    LOG_INFO("WEB_CB", "Changement de mode demandé: %d", nouveauMode);
    autopilot.setMode(nouveauMode);
    // Optionnel: Envoyer une confirmation ou mise à jour via webInterface.sendNotification(...)
}

void gererChangementDirection(float angle, float puissance) {
    LOG_INFO("WEB_CB", "Changement de direction demandé: Angle=%.1f, Puissance=%.1f", angle, puissance);
    // Assumer que le mode manuel est actif ou le gérer ici
    servos.setDirectionAngle(angle);
    servos.setTrimAngle(puissance);
}

void gererArretUrgence() {
    LOG_WARNING("WEB_CB", "ARRÊT D'URGENCE demandé via l'interface web!");
    autopilot.setMode(AUTOPILOT_OFF); // Ou un mode d'urgence spécifique
    servos.emergencyStop(); // Utiliser la méthode d'arrêt d'urgence du module servo
    // Autres actions d'urgence si nécessaire
}

//=============================================================================
// FONCTIONS DE GESTION DES PERFORMANCES ET DE LA SÉCURITÉ
//=============================================================================

 // Vérifie et met à jour les informations sur la mémoire disponible
void verifierMemoire() {
    static unsigned long lastMemoryCheckTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastMemoryCheckTime < MEMORY_CHECK_INTERVAL) {
        return;
    }
    lastMemoryCheckTime = currentTime;
    
    // Implémentation de mettreAJourEtatDeLaMemoire
    LOG_INFO("MAIN", "Mémoire disponible: %d", ESP.getFreeHeap());
}

// Effectue des vérifications périodiques de santé des capteurs
void verifierCapteurs() {
    static unsigned long lastSensorCheckTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastSensorCheckTime < SENSOR_CHECK_INTERVAL) {
        return;
    }
    lastSensorCheckTime = currentTime;
    
    // Implémentation de verifierEtatDeLImu
    LOG_INFO("MAIN", "Vérification de l'état de l'IMU");
    // Implémentation de verifierEtatDeLaTension
    LOG_INFO("MAIN", "Vérification de l'état de la tension");
    // Implémentation de verifierEtatDesServos
    LOG_INFO("MAIN", "Vérification de l'état des servos");
}

// Envoie un "heartbeat" périodique aux logs pour confirmer que le système fonctionne
void envoyerHeartbeat() {
    static unsigned long lastHeartbeatTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastHeartbeatTime < HEARTBEAT_INTERVAL) {
        return;
    }
    lastHeartbeatTime = currentTime;
    
    // Implémentation de mettreAJourEtatDuSysteme
    LOG_INFO("MAIN", "Mise à jour de l'état du système");
    // Implémentation de afficherHeartbeat
    LOG_INFO("MAIN", "Affichage du heartbeat");
}

// Affichage périodique des informations sur l'écran LCD
void mettreAJourAffichage(uint8_t screen = 1) {
    static unsigned long lastDisplayUpdateTime = 0;
    unsigned long currentTime = millis();
    if (currentTime - lastDisplayUpdateTime < DISPLAY_UPDATE_INTERVAL) {
        return;
    }
    lastDisplayUpdateTime = currentTime;
    
    // Implémentation de afficherInformationsSurLEcran
    LOG_INFO("MAIN", "Affichage des informations sur l'écran %d", screen);
}

// Fonction d'initialisation du système
bool initialiserLEDs() {
    if (!ledStatus.begin()) {
        LOG_ERROR("MAIN", "Échec d'initialisation de la LED de statut");
        return false;
    }
    if (!ledError.begin()) {
        LOG_ERROR("MAIN", "Échec d'initialisation de la LED d'erreur");
        return false;
    }
    return true;
}

bool initialiserLCD() {
    if (!lcd.begin()) {
        LOG_ERROR("MAIN", "Échec d'initialisation de l'écran LCD");
        ledError.setPattern(LED_PATTERN_ON);
        return false;
    }
    if (!lcd2.begin()) {
        LOG_WARNING("MAIN", "Deuxième écran LCD non détecté, désactivé");
    }
    return true;
}

bool initialiserCapteurs() {
    if (!sensors.begin()) {
        LOG_ERROR("MAIN", "Échec d'initialisation des capteurs");
        ledError.setPattern(LED_PATTERN_ON);
        lcd.print("Erreur capteurs", 0, 2);
        return false;
    }
    return true;
}

bool initialiserServos() {
    if (!servos.begin()) {
        LOG_ERROR("MAIN", "Échec d'initialisation des servomoteurs");
        ledError.setPattern(LED_PATTERN_ON);
        lcd.print("Erreur servos", 0, 2);
        return false;
    }
    return true;
}

bool initialiserAutopilot() {
    if (!autopilot.begin()) {
        LOG_ERROR("MAIN", "Échec d'initialisation de l'autopilote");
        ledError.setPattern(LED_PATTERN_ON);
        return false;
    }
    return true;
}

#ifdef SIMULATION_MODE
bool initialiserSimulation() {
    if (!simulation.begin()) {
        LOG_ERROR("MAIN", "Échec d'initialisation de la simulation");
        ledError.setPattern(LED_PATTERN_ON);
        lcd.print("Erreur simulation", 0, 2);
        return false;
    }
    return true;
}
#endif

#if defined(WIFI_ENABLED) && WIFI_ENABLED
bool initialiserWiFi() {
    ledStatus.setPattern(LED_PATTERN_WIFI_CONNECTING);
    LOG_INFO("WIFI", "Connexion au SSID: %s", WIFI_SSID);
    #ifdef SIMULATION_MODE
    WiFi.begin(WIFI_SSID, WIFI_PASS, 6);
    #else
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    #endif
    unsigned long wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 10000) {
        delay(500);
        LOG_DEBUG("WIFI", "En attente de la connexion WiFi...");
    }
    if (WiFi.status() == WL_CONNECTED) {
        LOG_INFO("WIFI", "Connecté, IP: %s", WiFi.localIP().toString().c_str());
        ledStatus.setPattern(LED_PATTERN_WIFI_CONNECTED);
        return true;
    } else {
        LOG_ERROR("MAIN", "Échec de connexion WiFi");
        ledError.setPattern(LED_PATTERN_ERROR);
        return false;
    }
}
#endif

#if defined(WIFI_ENABLED) && WIFI_ENABLED
bool initialiserWebInterface() {
    if (!webInterface.begin(WiFi.getMode() == WIFI_AP)) {
        LOG_ERROR("MAIN", "Échec d'initialisation de l'interface web");
        // Ne pas marquer comme échec critique pour l'instant
        // success = false;
        return false;
    }
    
    // Configurer les callbacks de l'interface web
    webInterface.setModeChangeCallback(gererChangementMode);
    webInterface.setDirectionChangeCallback(gererChangementDirection);
    webInterface.setEmergencyCallback(gererArretUrgence);
    return true;
}
#endif

bool initialiserTFT() {
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    if (!ctp.begin(40)) {
        Serial.println("Erreur: écran tactile non détecté !");
    } else {
        Serial.println("Tactile FT6206 prêt.");
    }
    return true;
}

bool initialiserSysteme() {
    bool success = true;
    
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println("\n\n=== Démarrage du système Kite Pilote ===");
    Serial.println("Version " VERSION_STRING " - " BUILD_DATE);
    
    Logger::begin(LL_INFO);
    LOG_INFO("MAIN", "Initialisation du système...");
    
    success &= initialiserLEDs();
    success &= initialiserLCD();
    success &= initialiserCapteurs();
    success &= initialiserServos();
    success &= initialiserAutopilot();
    
    #ifdef SIMULATION_MODE
    success &= initialiserSimulation();
    #endif
    
    #if defined(WIFI_ENABLED) && WIFI_ENABLED
    success &= initialiserWiFi();
    #endif

    // Initialiser l'interface web (après le WiFi)
    #if defined(WIFI_ENABLED) && WIFI_ENABLED
    success &= initialiserWebInterface();
    #endif
    
    success &= initialiserTFT();
    
    esp_task_wdt_init(10, true);
    esp_task_wdt_add(NULL);
    
    systemStatus.isInitialized = success;
    systemStatus.lastError = success ? ERROR_NONE : ERROR_SENSOR_INIT;
    
    if (success) {
        LOG_INFO("MAIN", "Initialisation réussie");
        ledStatus.setPattern(LED_PATTERN_SLOW_BLINK);
        ledError.setPattern(LED_PATTERN_OFF);
        lcd.print("Initialisation OK", 0, 2);
        delay(1000);
    } else {
        LOG_ERROR("MAIN", "Erreurs pendant l'initialisation");
        ledStatus.setPattern(LED_PATTERN_OFF);
        ledError.setPattern(LED_PATTERN_SLOW_BLINK);
        lcd.print("ERREUR INIT SYSTEME", 0, 0);
    }
    
    return success;
}

void simulateKite() {
    #ifdef SIMULATION_MODE
    // Code spécifique à la simulation du cerf-volant
    simulation.updateSensors(sensors);
    simulation.handleButtons(autopilot, servos);
    #endif
}

//=============================================================================
// FONCTIONS PRINCIPALES ARDUINO
//=============================================================================

void setup() {
    Serial.begin(115200);
    Serial.println("Initialisation de LittleFS...");

    if (!LittleFS.begin(true)) {
        Serial.println("Erreur : Impossible de monter LittleFS");
        return;
    }
    Serial.println("LittleFS monté avec succès !");

    // Exemple de lecture d'un fichier
    File fichier = LittleFS.open("/index.html", "r");
    if (!fichier) {
        Serial.println("Erreur : Impossible d'ouvrir le fichier /index.html");
    } else {
        Serial.println("Contenu de /index.html :");
        while (fichier.available()) {
            Serial.write(fichier.read());
        }
        fichier.close();
    }

    if (!initialiserSysteme()) {
        while (true) {
            ledError.update();
            delay(100);
        }
    }
    autopilot.setMode(AUTOPILOT_STANDBY);
    lcd.clear();
    lcd.print("Kite Pilote v" VERSION_STRING, 0, 0);
    lcd.print("Systeme pret", 0, 1);
    lcd.print("Mode: Attente", 0, 2);
    
    vCreateTasks();
    vTaskDelete(NULL);
}

void loop() {
    // Mettre à jour l'interface web (pour DNS/Captive Portal)
    #if defined(WIFI_ENABLED) && WIFI_ENABLED
    webInterface.update();
    #endif

    // En mode simulation, mettre à jour la simulation
    simulateKite();
    
    // Laisser du temps aux autres tâches
    vTaskDelay(pdMS_TO_TICKS(10));
}
