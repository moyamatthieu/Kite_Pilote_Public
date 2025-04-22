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
LcdModule lcd(LCD1_I2C_ADDR, LCD_COLS, LCD_ROWS); // LCD principal
LcdModule lcd2(LCD2_I2C_ADDR, LCD_COLS, LCD_ROWS); // Deuxième LCD

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
#define TOUCH_SDA 13
#define TOUCH_SCL 12

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
void handleModeChange(AutopilotMode newMode) {
    LOG_INFO("WEB_CB", "Changement de mode demandé: %d", newMode);
    autopilot.setMode(newMode);
    // Optionnel: Envoyer une confirmation ou mise à jour via webInterface.sendNotification(...)
}

void handleDirectionChange(float angle, float power) {
    LOG_INFO("WEB_CB", "Changement de direction demandé: Angle=%.1f, Power=%.1f", angle, power);
    // Assumer que le mode manuel est actif ou le gérer ici
    servos.setDirectionAngle(angle);
    servos.setTrimAngle(power); // 'power' contrôle probablement le trim ici
}

void handleEmergencyStop() {
    LOG_WARNING("WEB_CB", "ARRÊT D'URGENCE demandé via l'interface web!");
    autopilot.setMode(AUTOPILOT_OFF); // Ou un mode d'urgence spécifique
    servos.emergencyStop(); // Utiliser la méthode d'arrêt d'urgence du module servo
    // Autres actions d'urgence si nécessaire
}

//=============================================================================
// FONCTIONS UTILITAIRES
//=============================================================================

//=============================================================================
// FONCTIONS DE GESTION DES PERFORMANCES ET DE LA SÉCURITÉ
//=============================================================================

// Vérifie et met à jour les informations sur la mémoire disponible
void verifierMemoire() {
    unsigned long currentTime = millis();
    if (currentTime - lastMemoryCheckTime < MEMORY_CHECK_INTERVAL) {
        return;
    }
    lastMemoryCheckTime = currentTime;
    
    systemStatus.freeMemory = ESP.getFreeHeap();
    
    if (systemStatus.freeMemory < MIN_FREE_MEMORY) {
        LOG_WARNING("SYSTEM", "Mémoire faible: %d octets", systemStatus.freeMemory);
        systemStatus.isError = true;
        systemStatus.lastError = ERROR_MEMORY_LOW;
        ledError.setPattern(LED_PATTERN_SLOW_BLINK);
    }
    
    LOG_DEBUG("MEMORY", "Mémoire libre: %d octets", systemStatus.freeMemory);
}

// Effectue des vérifications périodiques de santé des capteurs
void verifierCapteurs() {
    unsigned long currentTime = millis();
    if (currentTime - lastSensorCheckTime < SENSOR_CHECK_INTERVAL) {
        return;
    }
    lastSensorCheckTime = currentTime;
    
    IMUData imuData = sensors.getIMUData();
    LineData lineData = sensors.getLineData();
    
    if (!imuData.isValid || !imuData.isRecent(2000)) {
        LOG_WARNING("SENSOR", "Données IMU invalides ou obsolètes");
        systemStatus.isError = true;
        systemStatus.lastError = ERROR_IMU_DATA;
    } else if (!imuData.hasValidRange()) {
        LOG_WARNING("SENSOR", "Valeurs IMU hors limites: roll=%.1f, pitch=%.1f, yaw=%.1f",
                   imuData.roll, imuData.pitch, imuData.yaw);
    }
    
    if (!lineData.isTensionValid || !lineData.isRecent(2000)) {
        LOG_WARNING("SENSOR", "Données de tension invalides ou obsolètes");
    } else if (!lineData.isTensionSafe(MAX_SAFE_TENSION)) {
        LOG_WARNING("SENSOR", "Tension excessive: %.1f N", lineData.tension);
    }
    
    ServoState servoState = servos.getState();
    if (!servoState.isDirectionInLimits() || !servoState.isTrimInLimits()) {
        LOG_WARNING("SERVO", "Angles servos hors limites: dir=%.1f, trim=%.1f",
                   servoState.directionAngle, servoState.trimAngle);
    }
}

// Envoie un "heartbeat" périodique aux logs pour confirmer que le système fonctionne
void envoyerHeartbeat() {
    unsigned long currentTime = millis();
    if (currentTime - lastHeartbeatTime < HEARTBEAT_INTERVAL) {
        return;
    }
    lastHeartbeatTime = currentTime;
    
    systemStatus.uptime = currentTime;
    systemStatus.cpuTemperature = 45.0f + (random(0, 100) / 100.0f);
    
    char uptimeStr[16];
    systemStatus.getUptimeString(uptimeStr, sizeof(uptimeStr));
    
    LOG_INFO("SYSTEM", "Heartbeat - Uptime: %s, Free: %d KB, Temp: %.1f°C",
             uptimeStr, systemStatus.freeMemory / 1024, systemStatus.cpuTemperature);
}

// Affichage périodique des informations sur l'écran LCD
void mettreAJourAffichage(uint8_t screen = 1) {
    LcdModule& lcdTarget = getLcd(screen);
    unsigned long currentTime = millis();
    if (currentTime - lastDisplayUpdateTime < DISPLAY_UPDATE_INTERVAL) {
        return;
    }
    lastDisplayUpdateTime = currentTime;
    IMUData imuData = sensors.getIMUData();
    LineData lineData = sensors.getLineData();
    WindData windData = sensors.getWindData();
    AutopilotStatus autopilotStatus = autopilot.getStatus();
    if (screen == 1) {
        lcdTarget.showSystemScreen(
            autopilotStatus.statusMessage,
            imuData.roll,
            imuData.pitch,
            lineData.tension,
            autopilotStatus.powerGenerated
        );
        if (autopilotStatus.mode == AUTOPILOT_LAUNCH || autopilotStatus.mode == AUTOPILOT_LAND) {
            lcdTarget.showProgressBar(3, autopilotStatus.completionPercent);
        }
        if (systemStatus.isError) {
            if ((currentTime / 10000) % 2 == 0) {
                snprintf(messageBuffer, sizeof(messageBuffer), "ERR#%d", systemStatus.lastError);
                lcdTarget.print(messageBuffer, 16, 0);
            }
        }
    } else if (screen == 2) {
        char buffer[21];
        snprintf(buffer, sizeof(buffer), "WindDir: %5.1f deg", windData.direction);
        lcdTarget.print(buffer, 0, 0);
        snprintf(buffer, sizeof(buffer), "WindSpd: %5.1f m/s", windData.speed);
        lcdTarget.print(buffer, 0, 1);
        snprintf(buffer, sizeof(buffer), "Tension: %5.1f N", lineData.tension);
        lcdTarget.print(buffer, 0, 2);
    }
}

// Fonction d'initialisation du système
bool initialiserSysteme() {
    bool success = true;
    
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println("\n\n=== Démarrage du système Kite Pilote ===");
    Serial.println("Version " VERSION_STRING " - " BUILD_DATE);
    
    Logger::begin(LL_INFO);
    LOG_INFO("MAIN", "Initialisation du système...");
    
    if (!ledStatus.begin()) {
        LOG_ERROR("MAIN", "Échec d'initialisation de la LED de statut");
        success = false;
    }
    
    if (!ledError.begin()) {
        LOG_ERROR("MAIN", "Échec d'initialisation de la LED d'erreur");
        success = false;
    }
    
    if (!lcd.begin()) {
        LOG_ERROR("MAIN", "Échec d'initialisation de l'écran LCD");
        ledError.setPattern(LED_PATTERN_ON);
        success = false;
    }
    
    if (!lcd2.begin()) {
        LOG_WARNING("MAIN", "Deuxième écran LCD non détecté, désactivé");
    }
    
    if (!sensors.begin()) {
        LOG_ERROR("MAIN", "Échec d'initialisation des capteurs");
        ledError.setPattern(LED_PATTERN_ON);
        lcd.print("Erreur capteurs", 0, 2);
        success = false;
    }
    
    if (!servos.begin()) {
        LOG_ERROR("MAIN", "Échec d'initialisation des servomoteurs");
        ledError.setPattern(LED_PATTERN_ON);
        lcd.print("Erreur servos", 0, 2);
        success = false;
    }
    
    if (!autopilot.begin()) {
        LOG_ERROR("MAIN", "Échec d'initialisation de l'autopilote");
        ledError.setPattern(LED_PATTERN_ON);
        lcd.print("Erreur autopilote", 0, 2);
        success = false;
    }
    
    #ifdef SIMULATION_MODE
    if (!simulation.begin()) {
        LOG_ERROR("MAIN", "Échec d'initialisation de la simulation");
        ledError.setPattern(LED_PATTERN_ON);
        lcd.print("Erreur simulation", 0, 2);
        success = false;
    }
    #endif
    
    #if defined(WIFI_ENABLED) && WIFI_ENABLED
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
    } else {
        LOG_ERROR("WIFI", "Échec de connexion WiFi");
        ledError.setPattern(LED_PATTERN_ERROR);
        success = false;
    }
    #endif

    // Initialiser l'interface web (après le WiFi)
    #if defined(WIFI_ENABLED) && WIFI_ENABLED
    if (!webInterface.begin(WiFi.getMode() == WIFI_AP)) {
        LOG_ERROR("MAIN", "Échec d'initialisation de l'interface web");
        // Ne pas marquer comme échec critique pour l'instant
        // success = false;

        // Configurer les callbacks de l'interface web
        webInterface.setModeChangeCallback(handleModeChange);
        webInterface.setDirectionChangeCallback(handleDirectionChange);
        webInterface.setEmergencyCallback(handleEmergencyStop);
    }
    #endif
    
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
