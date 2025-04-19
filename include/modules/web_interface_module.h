/*
 * Module d'interface web pour le projet Kite Pilote
 * 
 * Ce module implémente une interface web responsive accessible via WiFi pour
 * contrôler et surveiller le système à distance. Il offre un dashboard interactif,
 * des contrôles et des fonctionnalités de configuration.
 * 
 * Créé le: 17/04/2025
 */

#ifndef WEB_INTERFACE_MODULE_H
#define WEB_INTERFACE_MODULE_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <DNSServer.h>
#include <SPIFFS.h>
#include "../core/config.h"
#include "../core/data_types.h"
#include "../utils/logger.h"

// Taille maximale pour les payloads JSON
#define JSON_BUFFER_SIZE 2048

// Port DNS pour le captive portal
#define DNS_PORT 53

class WebInterfaceModule {
public:
    // Constructeur
    WebInterfaceModule() : 
        _initialized(false),
        _apMode(true),      // Par défaut, mode point d'accès
        _captivePortalEnabled(true),
        _server(WIFI_WEB_PORT),
        _dnsServer(nullptr),
        _onModeChangeCallback(nullptr),
        _onDirectionChangeCallback(nullptr),
        _onEmergencyCallback(nullptr),
        _eventId(0)
    {}
    
    // Destructeur
    ~WebInterfaceModule() {
        if (_dnsServer != nullptr) {
            delete _dnsServer;
        }
    }
    
    // Initialisation du module
    bool begin(bool apMode = true) {
        LOG_INFO("WEB", "Initialisation de l'interface web...");
        
        _apMode = apMode;
        
        // Initialiser le système de fichiers SPIFFS pour servir les fichiers web
        if (!SPIFFS.begin(true)) {
            LOG_ERROR("WEB", "Échec d'initialisation SPIFFS");
            return false;
        }
        
        // Configurer le WiFi selon le mode
        if (_apMode) {
            // Mode point d'accès
            if (!setupAccessPoint()) {
                LOG_ERROR("WEB", "Échec de configuration du point d'accès WiFi");
                return false;
            }
            
            // Initialiser le serveur DNS pour le captive portal
            if (_captivePortalEnabled) {
                _dnsServer = new DNSServer();
                _dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());
                LOG_INFO("WEB", "Captive Portal activé");
            }
        } else {
            // Mode client (connexion à un réseau existant)
            if (!connectToNetwork()) {
                LOG_ERROR("WEB", "Échec de connexion au réseau WiFi");
                return false;
            }
        }
        
        // Configurer les routes du serveur web
        setupWebServer();
        
        // Démarrer le serveur web
        _server.begin();
        LOG_INFO("WEB", "Serveur web démarré sur le port %d", WIFI_WEB_PORT);
        
        if (_apMode) {
            LOG_INFO("WEB", "Interface web accessible à l'adresse http://%s/", WiFi.softAPIP().toString().c_str());
        } else {
            LOG_INFO("WEB", "Interface web accessible à l'adresse http://%s/", WiFi.localIP().toString().c_str());
        }
        
        _initialized = true;
        return true;
    }
    
    // Mise à jour périodique (pour le DNS en mode captive portal)
    void update() {
        if (!_initialized) return;
        
        // Traiter les requêtes DNS pour le captive portal
        if (_apMode && _captivePortalEnabled && _dnsServer != nullptr) {
            _dnsServer->processNextRequest();
        }
        
        // Autres tâches périodiques si nécessaires
    }
    
    // Définir un callback pour le changement de mode d'autopilote
    void setModeChangeCallback(void (*callback)(AutopilotMode)) {
        _onModeChangeCallback = callback;
    }
    
    // Définir un callback pour le changement de direction manuel
    void setDirectionChangeCallback(void (*callback)(float, float)) {
        _onDirectionChangeCallback = callback;
    }
    
    // Définir un callback pour le bouton d'urgence
    void setEmergencyCallback(void (*callback)()) {
        _onEmergencyCallback = callback;
    }
    
    // Envoyer une mise à jour des données système à tous les clients connectés
    void sendSystemUpdate(const SystemStatus& status, const AutopilotStatus& autopilotStatus, 
                         const IMUData& imuData, const LineData& lineData, const WindData& windData) {
        if (!_initialized) return;
        
        // Créer un JSON avec les données système
        DynamicJsonDocument doc(JSON_BUFFER_SIZE);
        JsonObject system = doc.createNestedObject("system");
        system["uptime"] = status.uptime / 1000; // En secondes
        system["battery"] = status.batteryVoltage;
        system["freeMemory"] = status.freeMemory;
        system["temp"] = status.cpuTemperature;
        system["error"] = status.isError;
        
        // Ajouter les données de l'autopilote
        JsonObject autopilot = doc.createNestedObject("autopilot");
        autopilot["mode"] = autopilotStatus.mode;
        autopilot["status"] = autopilotStatus.statusMessage;
        autopilot["power"] = autopilotStatus.powerGenerated;
        autopilot["energy"] = autopilotStatus.totalEnergy;
        autopilot["completion"] = autopilotStatus.completionPercent;
        
        // Ajouter les données de l'IMU
        JsonObject imu = doc.createNestedObject("imu");
        imu["roll"] = imuData.roll;
        imu["pitch"] = imuData.pitch;
        imu["yaw"] = imuData.yaw;
        imu["valid"] = imuData.isValid;
        
        // Ajouter les données des lignes
        JsonObject line = doc.createNestedObject("line");
        line["tension"] = lineData.tension;
        line["length"] = lineData.length;
        
        // Ajouter les données du vent
        JsonObject wind = doc.createNestedObject("wind");
        wind["speed"] = windData.speed;
        wind["direction"] = windData.direction;
        wind["valid"] = windData.isValid;
        
        // Identifiant unique pour cet événement
        doc["eventId"] = _eventId++;
        doc["timestamp"] = millis();
        
        // Sérialiser en JSON
        String jsonData;
        serializeJson(doc, jsonData);
        
        // Envoyer à tous les clients connectés
        _server.getEvents().send(jsonData.c_str(), "system-update", millis());
    }
    
    // Envoyer une notification aux clients connectés
    void sendNotification(const char* message, const char* type = "info") {
        if (!_initialized) return;
        
        DynamicJsonDocument doc(256);
        doc["message"] = message;
        doc["type"] = type;  // "info", "warning", "error", "success"
        doc["timestamp"] = millis();
        
        String jsonData;
        serializeJson(doc, jsonData);
        
        _server.getEvents().send(jsonData.c_str(), "notification", millis());
    }
    
    // Vérifier si l'interface web est active
    bool isActive() const {
        return _initialized;
    }
    
    // Vérifier si le mode point d'accès est actif
    bool isAccessPointMode() const {
        return _apMode;
    }
    
    // Obtenir le nombre de clients connectés au point d'accès
    int getConnectedClientsCount() const {
        if (!_initialized || !_apMode) return 0;
        return WiFi.softAPgetStationNum();
    }
    
private:
    bool _initialized;           // État d'initialisation
    bool _apMode;                // Mode point d'accès ou client
    bool _captivePortalEnabled;  // Captive portal activé
    AsyncWebServer _server;      // Serveur web asynchrone
    DNSServer* _dnsServer;       // Serveur DNS pour captive portal
    uint32_t _eventId;           // Identifiant incrémental pour les événements SSE
    
    // Callbacks
    void (*_onModeChangeCallback)(AutopilotMode);
    void (*_onDirectionChangeCallback)(float, float);
    void (*_onEmergencyCallback)();
    
    // Configuration du point d'accès WiFi
    bool setupAccessPoint() {
        LOG_INFO("WEB", "Configuration du point d'accès WiFi...");
        
        // Définir le mode AP
        WiFi.mode(WIFI_AP);
        
        // Configurer le point d'accès
        bool result = WiFi.softAP(WIFI_DEFAULT_AP_SSID, WIFI_DEFAULT_AP_PASS);
        
        if (result) {
            LOG_INFO("WEB", "Point d'accès WiFi créé: SSID=%s, IP=%s", 
                    WIFI_DEFAULT_AP_SSID, WiFi.softAPIP().toString().c_str());
        } else {
            LOG_ERROR("WEB", "Échec de création du point d'accès WiFi");
        }
        
        return result;
    }
    
    // Connexion à un réseau WiFi existant
    bool connectToNetwork() {
        LOG_INFO("WEB", "Connexion au réseau WiFi...");
        
        // Définir le mode client
        WiFi.mode(WIFI_STA);
        
        // Configurer le réseau (à personnaliser selon les besoins)
        WiFi.begin("VotreSSID", "VotrePassword");
        
        // Attendre la connexion (avec timeout)
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < 30000) {
            delay(500);
            LOG_DEBUG("WEB", "Tentative de connexion WiFi...");
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            LOG_INFO("WEB", "Connecté au réseau WiFi: IP=%s", WiFi.localIP().toString().c_str());
            return true;
        } else {
            LOG_ERROR("WEB", "Échec de connexion au réseau WiFi");
            return false;
        }
    }
    
    // Configuration des routes du serveur web
    void setupWebServer() {
        // Servir les fichiers statiques depuis SPIFFS
        _server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
        
        // Route pour l'API de statut système
        _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
            DynamicJsonDocument doc(512);
            doc["system"] = "Kite Pilote";
            doc["version"] = VERSION_STRING;
            doc["uptime"] = millis() / 1000;  // En secondes
            
            if (_apMode) {
                doc["wifi"]["mode"] = "AP";
                doc["wifi"]["ssid"] = WIFI_DEFAULT_AP_SSID;
                doc["wifi"]["clientCount"] = WiFi.softAPgetStationNum();
            } else {
                doc["wifi"]["mode"] = "Client";
                doc["wifi"]["ssid"] = WiFi.SSID();
                doc["wifi"]["rssi"] = WiFi.RSSI();
            }
            
            String response;
            serializeJson(doc, response);
            
            request->send(200, "application/json", response);
        });
        
        // Route pour changer le mode d'autopilote
        _server.on("/api/mode", HTTP_POST, [this](AsyncWebServerRequest *request) {
            if (!request->hasParam("mode", true)) {
                request->send(400, "application/json", "{\"error\":\"Parameter 'mode' required\"}");
                return;
            }
            
            String modeStr = request->getParam("mode", true)->value();
            int mode = modeStr.toInt();
            
            if (mode < AUTOPILOT_OFF || mode > AUTOPILOT_POWER_GENERATION) {
                request->send(400, "application/json", "{\"error\":\"Invalid mode value\"}");
                return;
            }
            
            // Appeler le callback si défini
            if (_onModeChangeCallback) {
                _onModeChangeCallback(static_cast<AutopilotMode>(mode));
                
                DynamicJsonDocument doc(128);
                doc["success"] = true;
                doc["mode"] = mode;
                
                String response;
                serializeJson(doc, response);
                
                request->send(200, "application/json", response);
                
                // Envoyer une notification
                String modeName;
                switch (mode) {
                    case AUTOPILOT_OFF: modeName = "Désactivé"; break;
                    case AUTOPILOT_STANDBY: modeName = "Standby"; break;
                    case AUTOPILOT_LAUNCH: modeName = "Décollage"; break;
                    case AUTOPILOT_LAND: modeName = "Atterrissage"; break;
                    case AUTOPILOT_EIGHT_PATTERN: modeName = "Vol en 8"; break;
                    case AUTOPILOT_CIRCULAR: modeName = "Vol circulaire"; break;
                    case AUTOPILOT_POWER_GENERATION: modeName = "Génération"; break;
                    default: modeName = "Inconnu";
                }
                
                char notifMsg[64];
                snprintf(notifMsg, sizeof(notifMsg), "Mode changé pour: %s", modeName.c_str());
                sendNotification(notifMsg, "info");
            } else {
                request->send(503, "application/json", "{\"error\":\"Mode change handler not configured\"}");
            }
        });
        
        // Route pour le contrôle manuel de la direction
        _server.on("/api/direction", HTTP_POST, [this](AsyncWebServerRequest *request) {
            if (!request->hasParam("angle", true) || !request->hasParam("power", true)) {
                request->send(400, "application/json", "{\"error\":\"Parameters 'angle' and 'power' required\"}");
                return;
            }
            
            float angle = request->getParam("angle", true)->value().toFloat();
            float power = request->getParam("power", true)->value().toFloat();
            
            // Valider les plages
            if (angle < DIRECTION_MIN_ANGLE || angle > DIRECTION_MAX_ANGLE ||
                power < TRIM_MIN_ANGLE || power > TRIM_MAX_ANGLE) {
                request->send(400, "application/json", "{\"error\":\"Invalid angle or power values\"}");
                return;
            }
            
            // Appeler le callback si défini
            if (_onDirectionChangeCallback) {
                _onDirectionChangeCallback(angle, power);
                
                DynamicJsonDocument doc(128);
                doc["success"] = true;
                doc["angle"] = angle;
                doc["power"] = power;
                
                String response;
                serializeJson(doc, response);
                
                request->send(200, "application/json", response);
            } else {
                request->send(503, "application/json", "{\"error\":\"Direction change handler not configured\"}");
            }
        });
        
        // Route pour le bouton d'arrêt d'urgence
        _server.on("/api/emergency", HTTP_POST, [this](AsyncWebServerRequest *request) {
            if (_onEmergencyCallback) {
                _onEmergencyCallback();
                
                sendNotification("ARRÊT D'URGENCE ACTIVÉ", "error");
                request->send(200, "application/json", "{\"success\":true,\"message\":\"Emergency stop activated\"}");
            } else {
                request->send(503, "application/json", "{\"error\":\"Emergency handler not configured\"}");
            }
        });
        
        // Configuration des événements SSE (Server-Sent Events)
        AsyncEventSource& events = _server.getEvents();
        events.onConnect([](AsyncEventSourceClient *client) {
            client->send("Connected to Kite Pilote event stream", "connection", millis());
        });
        _server.addHandler(&events);
        
        // Gestionnaire pour les requêtes non trouvées (404)
        _server.onNotFound([](AsyncWebServerRequest *request) {
            if (request->method() == HTTP_OPTIONS) {
                // Gestion des requêtes OPTIONS (CORS)
                request->send(200);
            } else {
                // Page non trouvée - Rediriger vers la racine en mode captive portal
                request->redirect("/");
            }
        });
    }
};

#endif // WEB_INTERFACE_MODULE_H
