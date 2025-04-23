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

// Fonction pour afficher un message d'erreur sur tous les périphériques d'affichage
void afficherErreur(const char* message, uint8_t codeErreur = 0) {
    // Affichage sur console série
    Serial.print("ERREUR: ");
    Serial.print(message);
    if (codeErreur > 0) {
        Serial.print(" (Code: ");
        Serial.print(codeErreur);
        Serial.println(")");
    } else {
        Serial.println();
    }
    
    // Affichage sur LCD
    lcd.clear();
    lcd.print("ERREUR:", 0, 0);
    lcd.print(message, 0, 1);
    if (codeErreur > 0) {
        lcd.print("Code: ", 0, 2);
        lcd.print(String(codeErreur).c_str(), 6, 2);
    }
    
    // Affichage sur TFT avec gestion du texte
    tft.fillRect(0, tft.height()-40, tft.width(), 40, ILI9341_RED);
    tft.setCursor(5, tft.height()-35);
    tft.setTextColor(ILI9341_WHITE);
    String txtErreur = "ERREUR: ";
    txtErreur += message;
    tft.println(txtErreur);
    
    if (codeErreur > 0) {
        tft.setCursor(5, tft.height()-15);
        String txtCode = "Code: ";
        txtCode += String(codeErreur);
        tft.println(txtCode);
    }
    
    // Indication LED
    ledError.setPattern(LED_PATTERN_ERROR);
}

// Fonction d'initialisation du système
bool initialiserLEDs() {
    Serial.println("Initialisation des LEDs...");
    if (!ledStatus.begin()) {
        Serial.println("Échec d'initialisation de la LED de statut");
        return false;
    }
    if (!ledError.begin()) {
        Serial.println("Échec d'initialisation de la LED d'erreur");
        return false;
    }
    return true;
}

bool initialiserLCD() {
    Serial.println("Initialisation de l'écran LCD...");
    if (!lcd.begin()) {
        Serial.println("Échec d'initialisation de l'écran LCD");
        // Vérifier si la LED d'erreur est disponible sans utiliser isInitialized()
        ledError.setPattern(LED_PATTERN_ON);
        return false;
    }
    lcd.clear();
    lcd.print("Kite Pilote " VERSION_STRING, 0, 0);
    lcd.print("Demarrage...", 0, 1);
    
    if (!lcd2.begin()) {
        Serial.println("Deuxième écran LCD non détecté, désactivé");
    }
    return true;
}

bool initialiserCapteurs() {
    Serial.println("Initialisation des capteurs...");
    lcd.print("Init capteurs...", 0, 2);
    tft.println("Initialisation capteurs...");
    
    if (!sensors.begin()) {
        Serial.println("Échec d'initialisation des capteurs");
        LOG_ERROR("MAIN", "Échec d'initialisation des capteurs");
        ledError.setPattern(LED_PATTERN_ON);
        lcd.print("Err: capteurs", 0, 3);
        tft.setTextColor(ILI9341_RED);
        tft.println("ERREUR: Initialisation capteurs");
        tft.setTextColor(ILI9341_WHITE);
        return false;
    }
    tft.println("OK: Capteurs initialises");
    return true;
}

bool initialiserServos() {
    Serial.println("Initialisation des servomoteurs...");
    lcd.print("Init servos...", 0, 2);
    tft.println("Initialisation servomoteurs...");
    
    if (!servos.begin()) {
        Serial.println("Échec d'initialisation des servomoteurs");
        LOG_ERROR("MAIN", "Échec d'initialisation des servomoteurs");
        ledError.setPattern(LED_PATTERN_ON);
        lcd.print("Err: servos", 0, 3);
        tft.setTextColor(ILI9341_RED);
        tft.println("ERREUR: Initialisation servomoteurs");
        tft.setTextColor(ILI9341_WHITE);
        return false;
    }
    tft.println("OK: Servomoteurs initialises");
    return true;
}

bool initialiserAutopilot() {
    Serial.println("Initialisation de l'autopilote...");
    lcd.print("Init autopilote...", 0, 2);
    tft.println("Initialisation autopilote...");
    
    if (!autopilot.begin()) {
        Serial.println("Échec d'initialisation de l'autopilote");
        LOG_ERROR("MAIN", "Échec d'initialisation de l'autopilote");
        ledError.setPattern(LED_PATTERN_ON);
        lcd.print("Err: autopilote", 0, 3);
        tft.setTextColor(ILI9341_RED);
        tft.println("ERREUR: Initialisation autopilote");
        tft.setTextColor(ILI9341_WHITE);
        return false;
    }
    tft.println("OK: Autopilote initialise");
    return true;
}

#ifdef SIMULATION_MODE
bool initialiserSimulation() {
    Serial.println("Initialisation de la simulation...");
    lcd.print("Init simulation...", 0, 2);
    tft.println("Initialisation simulation...");
    
    if (!simulation.begin()) {
        Serial.println("Échec d'initialisation de la simulation");
        LOG_ERROR("MAIN", "Échec d'initialisation de la simulation");
        ledError.setPattern(LED_PATTERN_ON);
        lcd.print("Err: simulation", 0, 3);
        tft.setTextColor(ILI9341_RED);
        tft.println("ERREUR: Initialisation simulation");
        tft.setTextColor(ILI9341_WHITE);
        return false;
    }
    tft.println("OK: Simulation initialisee");
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
    Serial.println("Initialisation de l'écran TFT...");
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(2);
    tft.setCursor(0, 0);
    tft.println("KITE PILOTE v" VERSION_STRING);
    tft.println("Initialisation du systeme...");
    
    if (!ctp.begin(40)) {
        Serial.println("Erreur: écran tactile non détecté !");
        tft.setTextColor(ILI9341_YELLOW);
        tft.println("Ecran tactile non detecte");
        tft.setTextColor(ILI9341_WHITE);
    } else {
        Serial.println("Tactile FT6206 prêt.");
        tft.println("Ecran tactile initialise");
    }
    return true;
}

bool initialiserSystemeReorganise() {
    bool success = true;
    
    // Logger déjà initialisé dans setup()
    LOG_INFO("MAIN", "Initialisation des modules...");
    
    // LEDs, LCD et TFT déjà initialisés dans setup()
    
    // Continuer avec les autres initialisations
    success &= initialiserCapteurs();
    success &= initialiserServos();
    success &= initialiserAutopilot();
    
    #ifdef SIMULATION_MODE
    success &= initialiserSimulation();
    #endif
    
    #if defined(WIFI_ENABLED) && WIFI_ENABLED
    lcd.clear();
    lcd.print("Init WiFi...", 0, 1);
    tft.println("Connexion WiFi...");
    success &= initialiserWiFi();
    #endif

    #if defined(WIFI_ENABLED) && WIFI_ENABLED
    lcd.clear();
    lcd.print("Init interface web", 0, 1);
    lcd.print("En cours...", 0, 2);
    tft.println("Initialisation interface web...");
    success &= initialiserWebInterface();
    #endif
    
    esp_task_wdt_init(10, true);
    esp_task_wdt_add(NULL);
    
    systemStatus.isInitialized = success;
    systemStatus.lastError = success ? ERROR_NONE : ERROR_SENSOR_INIT;
    
    if (success) {
        LOG_INFO("MAIN", "Initialisation réussie");
        ledStatus.setPattern(LED_PATTERN_SLOW_BLINK);
        ledError.setPattern(LED_PATTERN_OFF);
        lcd.clear();
        lcd.print("Kite Pilote " VERSION_STRING, 0, 0);
        lcd.print("Initialisation OK", 0, 1);
        lcd.print("Systeme pret", 0, 2);
        
        tft.println("Initialisation complete!");
        tft.println("Systeme pret");
        delay(1000);
    } else {
        LOG_ERROR("MAIN", "Erreurs pendant l'initialisation");
        ledStatus.setPattern(LED_PATTERN_OFF);
        ledError.setPattern(LED_PATTERN_SLOW_BLINK);
        
        lcd.clear();
        lcd.print("ERREUR INIT SYSTEME", 0, 0);
        
        tft.setTextColor(ILI9341_RED);
        tft.println("ERREUR D'INITIALISATION");
        tft.println("Verifiez les logs");
        tft.setTextColor(ILI9341_WHITE);
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
    // 1. Initialiser le port série en tout premier
    Serial.begin(115200);
    Serial.println("\n\n========= KITE PILOTE v" VERSION_STRING " =========");
    Serial.println("Démarrage du système - " BUILD_DATE);
    
    // 2. Initialiser les LEDs d'état en priorité pour indiquer le démarrage
    initialiserLEDs();
    ledStatus.setPattern(LED_PATTERN_SLOW_BLINK); // Indique démarrage en cours
    
    // 3. Initialiser l'écran TFT avant les premiers tests
    initialiserTFT();
    
    // 4. Initialiser LCD pour afficher les messages
    initialiserLCD();
    
    // 5. Initialiser le logger (seulement après avoir initialisé le port série)
    Logger::begin(LL_INFO);
    LOG_INFO("MAIN", "Initialisation du système Kite Pilote v%s", VERSION_STRING);
    
    // 6. Initialiser le système de fichiers
    lcd.print("Init LittleFS...", 0, 2);
    tft.println("Montage LittleFS...");
    Serial.println("Initialisation de LittleFS...");
    bool fsOk = false;
    
    // Première tentative avec partition spécifique
    if (LittleFS.begin(true, "/littlefs", 10, "storage")) {
        Serial.println("LittleFS monté avec succès sur la partition 'storage'!");
        tft.println("LittleFS OK: partition storage");
        fsOk = true;
    } else {
        Serial.println("Échec sur partition 'storage', tentative avec configuration par défaut...");
        tft.setTextColor(ILI9341_YELLOW);
        tft.println("Échec LittleFS: partition storage");
        tft.println("Tentative par défaut...");
        tft.setTextColor(ILI9341_WHITE);
        
        // Deuxième tentative sans spécifier de partition
        if (LittleFS.begin(true)) {
            Serial.println("LittleFS monté avec succès en mode par défaut!");
            tft.println("LittleFS OK: mode par défaut");
            fsOk = true;
        } else {
            Serial.println("AVERTISSEMENT: Initialisation de LittleFS impossible - fonctionnement dégradé");
            tft.setTextColor(ILI9341_RED);
            tft.println("ERREUR: LittleFS indisponible");
            tft.println("Fonctionnement dégradé");
            tft.setTextColor(ILI9341_WHITE);
            // Nous continuons quand même, le système fonctionnera sans accès aux fichiers
        }
    }
    
    // Vérification optionnelle des fichiers seulement si le montage a réussi
    if (fsOk) {
        // Exemple de lecture d'un fichier
        File fichier = LittleFS.open("/index.html", "r");
        if (!fichier) {
            Serial.println("Avertissement: Impossible d'ouvrir le fichier /index.html");
            tft.setTextColor(ILI9341_YELLOW);
            tft.println("Fichier index.html introuvable");
            tft.setTextColor(ILI9341_WHITE);
        } else {
            Serial.println("Fichier index.html trouvé (taille: " + String(fichier.size()) + " octets)");
            tft.println("Fichier index.html OK");
            // Lecture du contenu (limité aux premiers caractères pour éviter de surcharger la console)
            Serial.println("Début du fichier:");
            int i = 0;
            while (fichier.available() && i < 200) {
                Serial.write(fichier.read());
                i++;
            }
            if (fichier.available()) {
                Serial.println("\n[...]");
            }
            fichier.close();
        }
    }

    // 7. Finaliser l'initialisation du système avec affichage
    lcd.print("Init modules...", 0, 3);
    tft.println("\nInitialisation des modules...");
    
    if (!initialiserSystemeReorganise()) {
        // Affichage d'erreur critique sur tous les périphériques
        lcd.clear();
        lcd.print("ERREUR CRITIQUE", 0, 0);
        lcd.print("Systeme bloque", 0, 1);
        
        tft.fillScreen(ILI9341_RED);
        tft.setCursor(0, 0);
        tft.setTextColor(ILI9341_WHITE);
        tft.println("ERREUR CRITIQUE");
        tft.println("Initialisation echouee");
        tft.println("Redemarrage necessaire");
        
        Serial.println("\n\n****** ERREUR CRITIQUE D'INITIALISATION ******");
        Serial.println("Le système ne peut pas démarrer, redémarrage nécessaire");
        
        // Boucle d'erreur avec clignotement
        while (true) {
            ledError.update();
            delay(100);
        }
    }
    
    // Passage en mode veille après l'initialisation
    autopilot.setMode(AUTOPILOT_STANDBY);
    lcd.clear();
    lcd.print("Kite Pilote v" VERSION_STRING, 0, 0);
    lcd.print("Systeme pret", 0, 1);
    lcd.print("Mode: Attente", 0, 2);
    
    tft.println("\nDémarrage des tâches...");
    vCreateTasks();
    tft.println("Système opérationnel");
    
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
