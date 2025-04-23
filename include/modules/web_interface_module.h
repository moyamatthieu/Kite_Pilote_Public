/*
 * Module d'interface web pour le projet Kite Pilote
 * 
 * Ce module implémente une interface web responsive accessible via WiFi pour
 * contrôler et surveiller le système à distance. Il offre un dashboard interactif,
 * des contrôles et des fonctionnalités de configuration.
 * 
 * Version: 2.0.0.5
 * Créé le: 17/04/2025
 * Mis à jour le: 23/04/2025
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
#include <functional>
#include "../core/config.h"
#include "../core/data_types.h"
#include "../utils/logger.h"

// Constantes pour la configuration du serveur web
constexpr size_t JSON_BUFFER_SIZE = 2048; // Taille maximale pour les payloads JSON
constexpr uint16_t DNS_PORT = 53;        // Port DNS pour le captive portal

/**
 * @enum FileSystemType
 * @brief Types de systèmes de fichiers supportés par l'interface web
 */
enum class FileSystemType {
    FS_NONE,    // Aucun système de fichiers disponible
    FS_LITTLEFS // LittleFS est disponible
};

/**
 * @class WebInterfaceModule
 * @brief Gère l'interface web du système Kite Pilote
 * 
 * Fournit une API RESTful et une interface utilisateur web pour contrôler et surveiller
 * le système à distance. Supporte les communications en temps réel avec Server-Sent Events
 * et gère un point d'accès WiFi avec captive portal.
 */
class WebInterfaceModule {
public:
    /**
     * @brief Constructeur par défaut
     */
    WebInterfaceModule();
    
    /**
     * @brief Destructeur, libère les ressources
     */
    ~WebInterfaceModule();
    
    /**
     * @brief Initialise le module et démarre le serveur web
     * @param apMode Si true, crée un point d'accès WiFi, sinon se connecte à un réseau existant
     * @return true si l'initialisation a réussi
     */
    bool begin(bool apMode = true);

    /**
     * @brief Met à jour l'état du module, à appeler régulièrement dans la boucle principale
     * Gère notamment le DNS en mode captive portal
     */
    void update();

    /**
     * @brief Définit un callback pour le changement de mode d'autopilote
     * @param callback Fonction à appeler lors d'un changement de mode
     */
    void setModeChangeCallback(std::function<void(AutopilotMode)> callback);

    /**
     * @brief Définit un callback pour le changement de direction manuel
     * @param callback Fonction à appeler lors d'un changement de direction (angles de roulis et tangage)
     */
    void setDirectionChangeCallback(std::function<void(float, float)> callback);

    /**
     * @brief Définit un callback pour le bouton d'urgence
     * @param callback Fonction à appeler lors d'un arrêt d'urgence
     */
    void setEmergencyCallback(std::function<void()> callback);

    /**
     * @brief Envoie l'état actuel du système à tous les clients connectés
     * @param status État général du système
     * @param autopilotStatus État de l'autopilote
     * @param imuData Données d'orientation
     * @param lineData Données des lignes
     * @param windData Données du vent
     */
    void sendSystemUpdate(const SystemStatus& status, const AutopilotStatus& autopilotStatus,
                          const IMUData& imuData, const LineData& lineData, const WindData& windData);

    /**
     * @brief Gère la lecture des fichiers depuis le système de fichiers
     * @param request Requête HTTP
     * @param path Chemin du fichier demandé
     * @return true si le fichier a été trouvé et servi
     */
    bool handleFileRead(AsyncWebServerRequest *request, const String& path);

    /**
     * @brief Arrête proprement le serveur web et libère les ressources
     */
    void end();

    /**
     * @brief Envoie une notification à tous les clients connectés
     * @param message Message à afficher
     * @param type Type de notification (info, warning, error, success)
     */
    void sendNotification(const char* message, const char* type = "info");

    /**
     * @brief Vérifie si l'interface web est active
     * @return true si le serveur web est initialisé et actif
     */
    bool isActive() const noexcept;

    /**
     * @brief Vérifie si le mode point d'accès est actif
     * @return true si le mode point d'accès est actif
     */
    bool isAccessPointMode() const noexcept;

    /**
     * @brief Obtient le nombre de clients connectés au point d'accès
     * @return Nombre de clients connectés
     */
    int getConnectedClientsCount() const noexcept;

private:
    /**
     * @brief Configuration des routes du serveur web
     */
    void setupWebServer();

    /**
     * @brief Configurer les routes API et les gestionnaires de fichiers
     */
    void setupRoutes();

    /**
     * @brief Configuration du point d'accès WiFi
     * @return true si la configuration a réussi
     */
    bool setupAccessPoint();

    /**
     * @brief Connexion à un réseau WiFi existant
     * @return true si la connexion a réussi
     */
    bool connectToNetwork();

    /**
     * @brief Génère une interface web complète intégrée lorsque le système de fichiers n'est pas disponible
     * @return Code HTML de l'interface intégrée
     */
    String getEmbeddedHtml();

    // Attributs privés
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
    std::function<void(AutopilotMode)> _onModeChangeCallback;
    std::function<void(float, float)> _onDirectionChangeCallback;
    std::function<void()> _onEmergencyCallback;
};

#endif // WEB_INTERFACE_MODULE_H
