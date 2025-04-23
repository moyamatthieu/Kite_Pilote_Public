/*
 * Implémentation du module d'interface web pour le projet Kite Pilote
 *
 * Ce module implémente une interface web responsive accessible via WiFi pour
 * contrôler et surveiller le système à distance. Il utilise LittleFS pour stocker
 * les fichiers HTML, CSS et JavaScript.
 *
 * Version: 2.0.0.5
 * Créé le: 23/04/2025
 */

#include "modules/web_interface_module.h"
#include "utils/logger.h"

// Constructeur
WebInterfaceModule::WebInterfaceModule() :
    _server(WIFI_WEB_PORT),
    _events("/events"),
    _initialized(false),
    _apMode(true),      // Par défaut, mode point d'accès
    _captivePortalEnabled(true),
    _dnsServer(nullptr),
    _onModeChangeCallback(nullptr),
    _onDirectionChangeCallback(nullptr),
    _onEmergencyCallback(nullptr),
    _eventId(0),
    _fsAvailable(false), // Système de fichiers non disponible par défaut
    _fsType(FileSystemType::FS_NONE)
{
    // L'initialisation complète se fait dans begin()
}

// Destructeur
WebInterfaceModule::~WebInterfaceModule() {
    end(); // S'assurer que les ressources sont libérées
}

// Fonction privée pour servir les fichiers depuis LittleFS
bool WebInterfaceModule::handleFileRead(AsyncWebServerRequest *request, const String& path) {
    LOG_DEBUG("WEB", "HTTP: Requête pour %s", path.c_str());
    
    // Si path se termine par un slash, servir index.html
    String adjustedPath = path;
    if (adjustedPath.endsWith("/")) {
        adjustedPath += "index.html";
    }
    
    // Déterminer le type de contenu en fonction de l'extension
    String contentType = "text/plain";
    if (adjustedPath.endsWith(".html")) contentType = "text/html";
    else if (adjustedPath.endsWith(".css")) contentType = "text/css";
    else if (adjustedPath.endsWith(".js")) contentType = "application/javascript";
    else if (adjustedPath.endsWith(".png")) contentType = "image/png";
    else if (adjustedPath.endsWith(".jpg")) contentType = "image/jpeg";
    else if (adjustedPath.endsWith(".ico")) contentType = "image/x-icon";
    else if (adjustedPath.endsWith(".json")) contentType = "application/json";

    // Vérifier si le fichier existe
    if (LittleFS.exists(adjustedPath)) {
        request->send(LittleFS, adjustedPath, contentType);
        return true;
    }
    
    LOG_DEBUG("WEB", "HTTP: Fichier non trouvé: %s", adjustedPath.c_str());
    return false;
}

// Initialiser le module
bool WebInterfaceModule::begin(bool apMode) {
    _apMode = apMode;
    if (_initialized) {
        return true;
    }

    LOG_INFO("WEB", "WebInterface: Initialisation du module...");
    
    // Initialiser le système de fichiers en spécifiant le label de la partition
    // LittleFS.begin(formatOnFail, basePath, maxOpenFiles, partitionLabel)
    if (!LittleFS.begin(true, "/littlefs", 10, "storage")) { 
        LOG_WARNING("WEB", "WebInterface: Échec du montage de LittleFS sur la partition 'storage', utilisation du mode secours");
        _fsType = FileSystemType::FS_NONE;
        _fsAvailable = false;
        
        // Deuxième tentative avec partition par défaut (sans label spécifique)
        if (LittleFS.begin(true)) {
            LOG_INFO("WEB", "WebInterface: LittleFS initialisé avec la partition par défaut");
            _fsType = FileSystemType::FS_LITTLEFS;
            _fsAvailable = true;
        } else {
            LOG_ERROR("WEB", "WebInterface: Échec de la deuxième tentative d'initialisation LittleFS, mode HTML intégré activé");
            // Continuer sans système de fichiers, l'interface utilisera getEmbeddedHtml()
        }
    } else {
        _fsType = FileSystemType::FS_LITTLEFS;
        _fsAvailable = true;
        LOG_INFO("WEB", "WebInterface: LittleFS initialisé avec succès sur la partition 'storage'");
        
        // Liste des fichiers dans LittleFS pour débogage
        File root = LittleFS.open("/");
        File file = root.openNextFile();
        while (file) {
            String fileName = file.name();
            size_t fileSize = file.size();
            LOG_DEBUG("WEB", "WebInterface: Fichier: %s - Taille: %d octets", fileName.c_str(), fileSize);
            file = root.openNextFile();
        }
    }

    // Configuration du WiFi selon le mode (AP ou client)
    if (_apMode) {
        if (!setupAccessPoint()) {
            LOG_ERROR("WEB", "WebInterface: Échec de configuration du point d'accès");
            return false;
        }
    } else {
        if (!connectToNetwork()) {
            LOG_WARNING("WEB", "WebInterface: Échec de connexion au réseau, basculement en mode point d'accès");
            _apMode = true;
            return begin(true);
        }
    }

    // Configuration des routes API
    setupWebServer();
    
    // Marquer comme initialisé
    _initialized = true;
    return true;
}

// Configurer le point d'accès WiFi
bool WebInterfaceModule::setupAccessPoint() {
    LOG_INFO("WEB", "WebInterface: Configuration en mode point d'accès");
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(WIFI_DEFAULT_AP_SSID, WIFI_DEFAULT_AP_PASS)) {
        LOG_ERROR("WEB", "WebInterface: Échec de création du point d'accès");
        return false;
    }
    
    if (_captivePortalEnabled) {
        _dnsServer = new DNSServer();
        if (_dnsServer == nullptr) {
            LOG_ERROR("WEB", "WebInterface: Échec d'allocation du serveur DNS");
            return false;
        }
        
        _dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());
        LOG_INFO("WEB", "WebInterface: Portail captif activé sur %s", WiFi.softAPIP().toString().c_str());
    }
    
    LOG_INFO("WEB", "WebInterface: Point d'accès créé: %s - IP: %s", 
             WIFI_DEFAULT_AP_SSID, WiFi.softAPIP().toString().c_str());
    return true;
}

// Se connecter à un réseau WiFi existant
bool WebInterfaceModule::connectToNetwork() {
    LOG_INFO("WEB", "WebInterface: Configuration en mode client WiFi");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    
    constexpr int maxAttempts = 20;
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
        delay(500);
        LOG_DEBUG("WEB", "WebInterface: En attente de connexion WiFi...");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        LOG_INFO("WEB", "WebInterface: Connecté au réseau WiFi - IP: %s", WiFi.localIP().toString().c_str());
        return true;
    } else {
        LOG_ERROR("WEB", "WebInterface: Échec de connexion au réseau WiFi");
        return false;
    }
}

// Configuration du serveur web et des routes
void WebInterfaceModule::setupWebServer() {
    // Configurer les routes et le serveur
    setupRoutes();
    
    // Démarrer le serveur
    _server.begin();
    LOG_INFO("WEB", "WebInterface: Serveur web démarré sur le port %d", WIFI_WEB_PORT);
}

// Configurer les routes API et les gestionnaires de fichiers
void WebInterfaceModule::setupRoutes() {
    // Route par défaut pour servir les fichiers statiques depuis LittleFS
    if (_fsAvailable) {
        _server.serveStatic("/", LittleFS, "/").setCacheControl("max-age=31536000");
    }
    
    // Gestionnaire de fichiers non trouvés (fallback)
    _server.onNotFound([this](AsyncWebServerRequest *request) {
        // Essayer de servir un fichier
        if (!handleFileRead(request, request->url())) {
            // Si fichier non trouvé et système de fichiers non disponible, générer HTML intégré
            if (!_fsAvailable) {
                request->send(200, "text/html", getEmbeddedHtml());
            } else {
                request->send(404, "text/plain", "Fichier non trouvé");
            }
        }
    });

    // API pour obtenir le statut
    _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        DynamicJsonDocument doc(JSON_BUFFER_SIZE);
        
        doc["version"] = FIRMWARE_VERSION;
        doc["uptime"] = millis() / 1000;
        doc["fsType"] = (_fsType == FileSystemType::FS_LITTLEFS) ? "LittleFS" : "Aucun";
        doc["freeHeap"] = ESP.getFreeHeap();
        
        serializeJson(doc, *response);
        request->send(response);
    });
    
    // API pour changer de mode
    _server.on("/api/mode", HTTP_POST, [this](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        DynamicJsonDocument doc(JSON_BUFFER_SIZE);
        bool success = false;
        
        if (request->hasParam("mode", true)) {
            int mode = request->getParam("mode", true)->value().toInt();
            
            // Exécuter le callback si défini
            if (_onModeChangeCallback) {
                _onModeChangeCallback(static_cast<AutopilotMode>(mode));
                success = true;  // Considérer comme réussi si callback exécuté
            } else {
                success = false;  // Échec si pas de callback défini
            }
            
            doc["success"] = success;
            doc["mode"] = mode;
        } else {
            doc["success"] = false;
            doc["error"] = "Paramètre 'mode' manquant";
        }
        
        serializeJson(doc, *response);
        request->send(response);
    });
    
    // API pour contrôle manuel de direction
    _server.on("/api/direction", HTTP_POST, [this](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        DynamicJsonDocument doc(JSON_BUFFER_SIZE);
        bool success = false;
        
        if (request->hasParam("roll", true) && request->hasParam("pitch", true)) {
            float roll = request->getParam("roll", true)->value().toFloat();
            float pitch = request->getParam("pitch", true)->value().toFloat();
            
            // Exécuter le callback si défini
            if (_onDirectionChangeCallback) {
                _onDirectionChangeCallback(roll, pitch);
                success = true;
            } else {
                success = false;
            }
            
            doc["success"] = success;
            doc["roll"] = roll;
            doc["pitch"] = pitch;
        } else {
            doc["success"] = false;
            doc["error"] = "Paramètres 'roll' et/ou 'pitch' manquants";
        }
        
        serializeJson(doc, *response);
        request->send(response);
    });
    
    // API pour arrêt d'urgence
    _server.on("/api/emergency", HTTP_POST, [this](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        DynamicJsonDocument doc(JSON_BUFFER_SIZE);
        bool success = false;
        
        // Exécuter le callback si défini
        if (_onEmergencyCallback) {
            _onEmergencyCallback();
            success = true;  // Considérer comme réussi si callback exécuté
        } else {
            success = false;  // Échec si pas de callback défini
        }
        
        doc["success"] = success;
        
        serializeJson(doc, *response);
        request->send(response);
    });
    
    // Configuration des événements côté serveur (SSE)
    _server.addHandler(&_events);
}

// Mettre à jour l'interface avec de nouvelles données
void WebInterfaceModule::sendSystemUpdate(const SystemStatus& status, const AutopilotStatus& autopilotStatus, 
                          const IMUData& imuData, const LineData& lineData, const WindData& windData) {
    if (!_initialized) {
        return;
    }
    
    // Créer un document JSON avec les données du système
    DynamicJsonDocument doc(JSON_BUFFER_SIZE);
    
    // Système
    JsonObject system = doc.createNestedObject("system");
    system["uptime"] = millis() / 1000;
    system["battery"] = status.batteryVoltage;
    system["temp"] = status.cpuTemperature;
    system["freeMemory"] = ESP.getFreeHeap();
    
    // Autopilote
    JsonObject autopilot = doc.createNestedObject("autopilot");
    autopilot["mode"] = autopilotStatus.mode;
    autopilot["statut"] = autopilotStatus.statusMessage;
    autopilot["completion"] = autopilotStatus.completionPercent;
    autopilot["puissance"] = autopilotStatus.powerGenerated;
    autopilot["energie"] = autopilotStatus.totalEnergy;
     
    // IMU
    JsonObject imu = doc.createNestedObject("imu");
    imu["valide"] = imuData.isValid;
    imu["roulis"] = imuData.roll;
    imu["tangage"] = imuData.pitch;
    imu["lacet"] = imuData.yaw;
     
    // Conditions
    JsonObject line = doc.createNestedObject("ligne");
    line["tension"] = lineData.tension;
     
    JsonObject wind = doc.createNestedObject("vent");
    wind["valide"] = windData.isValid;
    wind["vitesse"] = windData.speed;
    wind["direction"] = windData.direction;
    
    // Convertir en chaîne JSON
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Envoyer l'événement avec les données
    _events.send(jsonString.c_str(), "system-update", _eventId++);
}

// Envoyer une notification à l'interface web
void WebInterfaceModule::sendNotification(const char* message, const char* type) {
    if (!_initialized) {
        return;
    }
    
    // Créer un document JSON pour la notification
    DynamicJsonDocument doc(256);
    doc["message"] = message;
    doc["type"] = type;
    
    // Convertir en chaîne JSON
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Envoyer l'événement avec les données de notification
    _events.send(jsonString.c_str(), "notification", _eventId++);
}

// Définir le callback pour le changement de mode d'autopilote
void WebInterfaceModule::setModeChangeCallback(std::function<void(AutopilotMode)> callback) {
    _onModeChangeCallback = callback;
}

// Définir le callback pour le changement de direction manuel
void WebInterfaceModule::setDirectionChangeCallback(std::function<void(float, float)> callback) {
    _onDirectionChangeCallback = callback;
}

// Définir le callback pour le bouton d'urgence
void WebInterfaceModule::setEmergencyCallback(std::function<void()> callback) {
    _onEmergencyCallback = callback;
}

// Mise à jour périodique (pour le DNS en mode captive portal)
void WebInterfaceModule::update() {
    if (_captivePortalEnabled && _dnsServer != nullptr) {
        _dnsServer->processNextRequest();
    }
}

// Arrêter proprement le module et libérer les ressources
void WebInterfaceModule::end() {
    if (!_initialized) {
        return;
    }
    
    // Nettoyage du DNS si actif
    if (_dnsServer != nullptr) {
        _dnsServer->stop();
        delete _dnsServer;
        _dnsServer = nullptr;
    }
    
    // Arrêter le serveur web
    _server.end();
    
    // Déconnecter WiFi
    if (_apMode) {
        WiFi.softAPdisconnect(true);
    } else {
        WiFi.disconnect(true);
    }
    
    _initialized = false;
    LOG_INFO("WEB", "WebInterface: Module arrêté");
}

// Vérifier si l'interface web est active
bool WebInterfaceModule::isActive() const noexcept {
    return _initialized;
}

// Vérifier si le mode point d'accès est actif
bool WebInterfaceModule::isAccessPointMode() const noexcept {
    return _apMode;
}

// Obtenir le nombre de clients connectés au point d'accès
int WebInterfaceModule::getConnectedClientsCount() const noexcept {
    if (_apMode && _initialized) {
        return WiFi.softAPgetStationNum();
    }
    return 0;
}

// Génère une interface web intégrée quand LittleFS n'est pas disponible
String WebInterfaceModule::getEmbeddedHtml() {
    // Interface simple avec indication d'erreur de système de fichiers
    return F("<html>"
             "<head>"
             "<meta charset='utf-8'>"
             "<meta name='viewport' content='width=device-width, initial-scale=1'>"
             "<title>Kite Pilote</title>"
             "<style>"
             "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; }"
             "h1 { color: #3498db; }"
             ".error { background-color: #f8d7da; color: #721c24; padding: 10px; border-radius: 5px; }"
             ".info { margin-top: 20px; }"
             "table { width: 100%; border-collapse: collapse; margin-top: 20px; }"
             "th, td { padding: 8px; text-align: left; border-bottom: 1px solid #ddd; }"
             "th { background-color: #f2f2f2; }"
             "</style>"
             "</head>"
             "<body>"
             "<h1>Kite Pilote - Interface Web</h1>"
             "<div class='error'>"
             "<strong>Erreur:</strong> Système de fichiers non disponible. Interface limitée."
             "</div>"
             "<div class='info'>"
             "<h2>Information Système</h2>"
             "<table>"
             "<tr><th>Version:</th><td>" VERSION_STRING "</td></tr>"
             "<tr><th>Date de compilation:</th><td>" BUILD_DATE "</td></tr>"
             "</table>"
             "<h2>État</h2>"
             "<p>Pour accéder à l'interface complète, veuillez vérifier l'installation de LittleFS.</p>"
             "</div>"
             "</body>"
             "</html>");
}
