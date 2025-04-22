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
#include <FS.h>
#include <LittleFS.h>
#include "../core/config.h"
#include "../core/data_types.h"
#include "../utils/logger.h"

// Taille maximale pour les payloads JSON
#define JSON_BUFFER_SIZE 2048

// Port DNS pour le captive portal
#define DNS_PORT 53

// Types de systèmes de fichiers supportés
enum FileSystemType {
    FS_NONE,    // Aucun système de fichiers disponible
    FS_LITTLEFS // LittleFS est disponible
};

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
        _eventId(0),
        _events("/events"), // Initialize SSE endpoint
        _fsAvailable(false), // Système de fichiers non disponible par défaut
        _fsType(FS_NONE)   // Aucun système de fichiers par défaut
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
        
        // Essayer d'initialiser LittleFS
        bool fsAvailable = false;
        
        // Tenter d'initialiser LittleFS (true = formater si nécessaire)
        LOG_INFO("WEB", "Tentative d'initialisation de LittleFS...");
        if (LittleFS.begin(true)) {
            fsAvailable = true;
            _fsType = FS_LITTLEFS;
            LOG_INFO("WEB", "LittleFS initialisé avec succès");
        } else {
            LOG_WARNING("WEB", "LittleFS non disponible, interface web utilisera du contenu intégré");
            _fsType = FS_NONE;
        }
        
        _fsAvailable = fsAvailable;
        
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
        
        // Envoyer à tous les clients connectés via SSE
        _events.send(jsonData.c_str(), "system-update", millis());
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
        
        _events.send(jsonData.c_str(), "notification", millis());
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
    bool _fsAvailable;           // Indique si un système de fichiers est disponible
    FileSystemType _fsType;      // Type de système de fichiers utilisé
    AsyncWebServer _server;      // Serveur web asynchrone
    AsyncEventSource _events;    // Source pour Server-Sent Events
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
        // Servir les fichiers statiques depuis LittleFS si disponible
        if (_fsAvailable && _fsType == FS_LITTLEFS) {
            LOG_INFO("WEB", "Configuration du serveur web avec LittleFS");
            _server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
        } else {
            // Route par défaut qui renvoie une page HTML intégrée si aucun système de fichiers n'est disponible
            LOG_INFO("WEB", "Configuration du serveur web avec HTML intégré");
            _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
                request->send(200, "text/html", getEmbeddedHtml());
            });
        }

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
        _events.onConnect([](AsyncEventSourceClient *client) {
             if(client->lastId()){
                LOG_DEBUG("WEB", "SSE Client reconnected, last ID: %u", client->lastId());
             }
             client->send("Connected to Kite Pilote event stream", "connection", millis(), 10000);
        });
        _server.addHandler(&_events); // Attach event source handler
        
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
    
    // Génère une interface web complète intégrée lorsque le système de fichiers n'est pas disponible
    String getEmbeddedHtml() {
        // Définition d'une interface web complète directement dans le code
        // Cette interface est similaire à celle que nous avons dans le dossier data/
        String html = F("<!DOCTYPE html><html lang=\"fr\"><head>"
            "<meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
            "<title>Kite Pilote</title>"
            "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>"
            "<style>"
            "body{font-family:Arial,sans-serif;margin:0;padding:0;background-color:#f5f5f5;color:#333;}"
            "header{background-color:#1a73e8;color:white;padding:1rem;text-align:center;}"
            ".container{max-width:1200px;margin:0 auto;padding:1rem;}"
            ".dashboard{display:grid;grid-template-columns:repeat(auto-fit,minmax(300px,1fr));gap:1rem;margin-bottom:1rem;}"
            ".card{background:white;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);padding:1rem;}"
            ".card h2{margin-top:0;border-bottom:1px solid #eee;padding-bottom:0.5rem;font-size:1.2rem;}"
            ".card.large{grid-column:span 2;}"
            ".controls{background:white;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);padding:1rem;margin-bottom:1rem;}"
            ".controls h2{margin-top:0;border-bottom:1px solid #eee;padding-bottom:0.5rem;}"
            ".mode-buttons{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:0.5rem;margin-bottom:1rem;}"
            ".tabs{display:flex;border-bottom:1px solid #ddd;margin-bottom:1rem;}"
            ".tab{padding:0.5rem 1rem;cursor:pointer;border:1px solid transparent;border-bottom:none;border-radius:4px 4px 0 0;}"
            ".tab.active{background:#fff;border-color:#ddd;margin-bottom:-1px;padding-bottom:calc(0.5rem + 1px);}"
            ".tab-content{display:none;}"
            ".tab-content.active{display:block;}"
            "button{background-color:#1a73e8;color:white;border:none;border-radius:4px;padding:0.5rem 1rem;cursor:pointer;font-size:0.9rem;transition:background-color 0.3s;}"
            "button:hover{background-color:#1557b0;}"
            "button.emergency{background-color:#dc3545;}"
            "button.emergency:hover{background-color:#b02a37;}"
            ".value{font-weight:bold;font-size:1.2rem;}"
            ".gauge-container{width:100%;height:20px;background-color:#eee;border-radius:10px;margin:0.5rem 0;overflow:hidden;}"
            ".gauge{height:100%;background-color:#1a73e8;transition:width 0.5s;}"
            ".status-ok{color:#198754;}.status-warning{color:#fd7e14;}.status-error{color:#dc3545;}"
            ".kite-visualizer{width:100%;height:300px;position:relative;background:#e6f7ff;border-radius:8px;overflow:hidden;}"
            ".kite{position:absolute;width:20px;height:20px;background:#1a73e8;border-radius:50%;transform-origin:center;transition:all 0.5s;}"
            ".kite::before{content:'';position:absolute;top:-10px;left:50%;margin-left:-10px;border-left:10px solid transparent;border-right:10px solid transparent;border-bottom:10px solid #1a73e8;}"
            ".kite-path{position:absolute;width:100%;height:100%;}"
            ".kite-controller{display:grid;grid-template-columns:repeat(3,1fr);gap:0.5rem;text-align:center;margin-top:1rem;}"
            ".control-pad{display:grid;grid-template-columns:repeat(3,1fr);grid-template-rows:repeat(3,1fr);gap:0.25rem;}"
            ".control-btn{padding:0.5rem;background:#f0f0f0;border-radius:4px;cursor:pointer;user-select:none;}"
            ".control-btn:hover{background:#e0e0e0;}"
            ".range-control{display:flex;align-items:center;margin:0.5rem 0;}"
            ".range-control label{flex:0 0 120px;}"
            ".range-control input{flex:1;}"
            ".range-control .value{flex:0 0 50px;text-align:right;}"
            ".chart-container{width:100%;height:200px;margin-top:1rem;}"
            ".notifications{position:fixed;top:1rem;right:1rem;width:300px;z-index:1000;}"
            ".notification{background:white;border-radius:4px;box-shadow:0 4px 8px rgba(0,0,0,0.1);padding:0.75rem 1rem;margin-bottom:0.5rem;opacity:1;transition:opacity 0.5s;}"
            ".notification.info{border-left:4px solid #1a73e8;}"
            ".notification.warning{border-left:4px solid #fd7e14;}"
            ".notification.error{border-left:4px solid #dc3545;}"
            ".notification.success{border-left:4px solid #198754;}"
            ".notification.fade{opacity:0;}"
            "@media(max-width:768px){.dashboard{grid-template-columns:1fr;}.mode-buttons{grid-template-columns:1fr 1fr;}.card.large{grid-column:span 1;}}"
            "</style></head><body>"
            "<header><h1>Kite Pilote</h1><p>Système de contrôle de cerf-volant générateur d'énergie</p></header>"
            "<div class=\"container\">"
            "<div class=\"tabs\">"
            "<div class=\"tab active\" data-tab=\"tab-dashboard\">Tableau de bord</div>"
            "<div class=\"tab\" data-tab=\"tab-manual\">Contrôle Manuel</div>"
            "<div class=\"tab\" data-tab=\"tab-config\">Configuration</div>"
            "</div>"
            
            "<div class=\"tab-content active\" id=\"tab-dashboard\">"
            "<div class=\"controls\">"
            "<h2>Contrôle du système</h2>"
            "<div class=\"mode-buttons\">"
            "<button data-mode=\"0\">Désactivé</button>"
            "<button data-mode=\"1\">Standby</button>"
            "<button data-mode=\"2\">Décollage</button>"
            "<button data-mode=\"3\">Atterrissage</button>"
            "<button data-mode=\"4\">Vol en 8</button>"
            "<button data-mode=\"5\">Vol circulaire</button>"
            "<button data-mode=\"6\">Génération</button>"
            "</div><div style=\"text-align:center;\">"
            "<button class=\"emergency\">ARRÊT D'URGENCE</button>"
            "</div></div>"
            
            "<div class=\"card large\">"
            "<h2>Visualisation du Kite</h2>"
            "<div class=\"kite-visualizer\">"
            "<canvas class=\"kite-path\" id=\"kite-path-canvas\"></canvas>"
            "<div class=\"kite\" id=\"kite-position\"></div>"
            "</div>"
            "</div>"
            
            "<div class=\"dashboard\">"
            "<div class=\"card\"><h2>Statut Système</h2>"
            "<p>Temps de fonctionnement: <span id=\"uptime\" class=\"value\">0s</span></p>"
            "<p>Batterie: <span id=\"battery\" class=\"value\">12.6V</span></p>"
            "<p>Mémoire libre: <span id=\"memory\" class=\"value\">0 Ko</span></p>"
            "<p>Température: <span id=\"temp\" class=\"value\">0°C</span></p>"
            "</div>"
            "<div class=\"card\"><h2>Autopilote</h2>"
            "<p>Mode actuel: <span id=\"mode\" class=\"value\">Inactif</span></p>"
            "<p>État: <span id=\"status\" class=\"value\">En attente</span></p>"
            "<p>Progression: <span id=\"completion\" class=\"value\">0%</span></p>"
            "<div class=\"gauge-container\"><div id=\"completion-gauge\" class=\"gauge\" style=\"width:0%\"></div></div>"
            "</div>"
            "<div class=\"card\"><h2>Génération d'énergie</h2>"
            "<p>Puissance instantanée: <span id=\"power\" class=\"value\">0W</span></p>"
            "<p>Énergie totale: <span id=\"energy\" class=\"value\">0Wh</span></p>"
            "<p>Efficacité: <span id=\"efficiency\" class=\"value\">0%</span></p>"
            "<div class=\"chart-container\">"
            "<canvas id=\"power-chart\"></canvas>"
            "</div>"
            "</div>"
            "<div class=\"card\"><h2>Capteurs du kite</h2>"
            "<p>Roulis: <span id=\"roll\" class=\"value\">0°</span></p>"
            "<p>Tangage: <span id=\"pitch\" class=\"value\">0°</span></p>"
            "<p>Lacet: <span id=\"yaw\" class=\"value\">0°</span></p>"
            "<p>Altitude estimée: <span id=\"altitude\" class=\"value\">0m</span></p>"
            "</div>"
            "<div class=\"card\"><h2>Conditions de vol</h2>"
            "<p>Tension ligne: <span id=\"tension\" class=\"value\">0N</span></p>"
            "<p>Vitesse du vent: <span id=\"wind-speed\" class=\"value\">0m/s</span></p>"
            "<p>Direction du vent: <span id=\"wind-dir\" class=\"value\">0°</span></p>"
            "</div>"
            "</div></div>"
            "<div id=\"notifications\" class=\"notifications\"></div>"
            "<script>"
            "function showNotification(message,type='info'){const notifications=document.getElementById('notifications');const notification=document.createElement('div');notification.className=`notification ${type}`;notification.innerHTML=message;notifications.appendChild(notification);setTimeout(()=>{notification.classList.add('fade');setTimeout(()=>{notification.remove();},500);},5000);}"
            "function updateUI(data){if(data.system){document.getElementById('uptime').textContent=`${data.system.uptime}s`;document.getElementById('battery').textContent=`${data.system.battery.toFixed(1)}V`;document.getElementById('memory').textContent=`${Math.round(data.system.freeMemory/1024)}Ko`;if(data.system.battery<11.5){document.getElementById('battery').className='value status-error';}else if(data.system.battery<12.0){document.getElementById('battery').className='value status-warning';}else{document.getElementById('battery').className='value status-ok';}}if(data.autopilot){const modeNames=['Désactivé','Standby','Décollage','Atterrissage','Vol en 8','Vol circulaire','Génération'];document.getElementById('mode').textContent=modeNames[data.autopilot.mode]||'Inconnu';document.getElementById('status').textContent=data.autopilot.status;document.getElementById('completion').textContent=`${data.autopilot.completion.toFixed(1)}%`;document.getElementById('completion-gauge').style.width=`${data.autopilot.completion}%`;document.getElementById('power').textContent=`${data.autopilot.power.toFixed(1)}W`;document.getElementById('energy').textContent=`${data.autopilot.energy.toFixed(2)}Wh`;}if(data.imu){if(data.imu.valid){document.getElementById('roll').textContent=`${data.imu.roll.toFixed(1)}°`;document.getElementById('pitch').textContent=`${data.imu.pitch.toFixed(1)}°`;document.getElementById('yaw').textContent=`${data.imu.yaw.toFixed(1)}°`;}else{document.getElementById('roll').textContent='N/A';document.getElementById('pitch').textContent='N/A';document.getElementById('yaw').textContent='N/A';}}}"
            "document.querySelectorAll('.mode-buttons button').forEach(button=>{button.addEventListener('click',async()=>{try{const mode=button.getAttribute('data-mode');const response=await fetch('/api/mode',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:`mode=${mode}`});if(response.ok){const result=await response.json();if(result.success){showNotification(`Mode changé avec succès`,'success');}}}catch(error){showNotification(`Erreur de connexion: ${error.message}`,'error');}});});"
            "document.querySelector('.emergency').addEventListener('click',async()=>{if(confirm('Êtes-vous sûr de vouloir activer l\\'ARRÊT D\\'URGENCE?')){try{const response=await fetch('/api/emergency',{method:'POST'});if(!response.ok){showNotification(`Erreur lors de l'arrêt d'urgence: ${response.status}`,'error');}}catch(error){showNotification(`Erreur de connexion: ${error.message}`,'error');}}});"
            "const eventSource=new EventSource('/events');eventSource.addEventListener('system-update',(event)=>{try{const data=JSON.parse(event.data);updateUI(data);}catch(error){console.error('Error parsing system update:',error);}});"
            "eventSource.addEventListener('notification',(event)=>{try{const data=JSON.parse(event.data);showNotification(data.message,data.type);}catch(error){console.error('Error parsing notification:',error);}});"
            "eventSource.addEventListener('connection',(event)=>{console.log('SSE Connected:',event.data);showNotification('Connecté au serveur','success');});"
            "eventSource.addEventListener('error',()=>{console.error('SSE Connection Error');showNotification('Erreur de connexion au serveur','error');});"
            "fetch('/api/status').then(response=>response.json()).then(data=>{console.log('Initial status:',data);showNotification(`Connecté au système Kite Pilote v${data.version}`,'info');}).catch(error=>{console.error('Error fetching initial status:',error);showNotification('Impossible de récupérer le statut initial','error');});"
            "</script></body></html>");
        return html;
    }
};

#endif // WEB_INTERFACE_MODULE_H
