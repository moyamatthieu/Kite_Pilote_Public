/*
 * Module de communication pour le projet Kite Pilote
 * 
 * Ce module gère la communication sans fil entre l'ESP32 de la station au sol
 * et l'ESP32 monté sur le kite. Il utilise le protocole ESP-NOW pour une
 * communication à faible latence et bonne portée.
 * 
 * Créé le: 17/04/2025
 */

#ifndef COMMUNICATION_MODULE_H
#define COMMUNICATION_MODULE_H

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "../core/config.h"
#include "../core/data_types.h"
#include "../utils/logger.h"

// Définition des tailles de paquets de données
#define PACKET_SIZE_IMU 16       // Paquet de données IMU (12 octets données + 4 octets contrôle)
#define PACKET_SIZE_COMMAND 8    // Paquet de commandes (4 octets données + 4 octets contrôle)

// Structure pour les données IMU envoyées par le kite
struct IMUPacket {
    float roll;                  // Roulis en degrés
    float pitch;                 // Tangage en degrés
    float yaw;                   // Lacet en degrés
    uint32_t timestamp;          // Horodatage de la mesure
};

// Structure pour les commandes envoyées au kite
struct CommandPacket {
    uint8_t commandType;         // Type de commande
    uint8_t value;               // Valeur principale
    uint8_t option;              // Option supplémentaire
    uint8_t checksum;            // Somme de contrôle
};

// Types de commandes
enum CommandType {
    CMD_NONE = 0,
    CMD_CALIBRATE_IMU = 1,       // Calibration de l'IMU
    CMD_SET_UPDATE_RATE = 2,     // Définir la fréquence d'envoi des données
    CMD_REQUEST_STATUS = 3,      // Demander un état détaillé
    CMD_REBOOT = 4,              // Redémarrer l'ESP32 du kite
    CMD_POWER_SAVE = 5,          // Mode économie d'énergie
    CMD_EMERGENCY = 255          // Signal d'urgence
};

class CommunicationModule {
public:
    // Constructeur
    CommunicationModule() : 
        _initialized(false),
        _isStationMode(true),    // Par défaut, mode station au sol
        _peerRegistered(false),
        _lastReceivedTime(0),
        _packetCount(0),
        _errorCount(0),
        _updateRate(50),         // 50 ms par défaut (20 Hz)
        _onDataReceived(nullptr)
    {
        // Adresse MAC par défaut (sera mise à jour lors de l'initialisation)
        memset(_selfMacAddress, 0, 6);
        memset(_peerMacAddress, 0, 6);
    }
    
    // Initialisation du module
    bool begin(bool isStationMode = true) {
        _isStationMode = isStationMode;
        
        // Initialiser le WiFi en mode Station
        WiFi.mode(WIFI_STA);
        
        // Obtenir l'adresse MAC de cet ESP32
        WiFi.macAddress(_selfMacAddress);
        
        LOG_INFO("COMM", "Initialisation ESP-NOW (%s)", _isStationMode ? "Station Sol" : "Kite");
        LOG_INFO("COMM", "Adresse MAC: %02X:%02X:%02X:%02X:%02X:%02X", 
                 _selfMacAddress[0], _selfMacAddress[1], _selfMacAddress[2], 
                 _selfMacAddress[3], _selfMacAddress[4], _selfMacAddress[5]);
        
        // Initialiser ESP-NOW
        if (esp_now_init() != ESP_OK) {
            LOG_ERROR("COMM", "Échec d'initialisation ESP-NOW");
            return false;
        }
        
        // Définir l'adresse MAC du pair selon le mode
        if (_isStationMode) {
            // Adresse MAC du kite (à configurer)
            _peerMacAddress[0] = 0xAA;  // Exemple, à remplacer par l'adresse réelle
            _peerMacAddress[1] = 0xBB;
            _peerMacAddress[2] = 0xCC;
            _peerMacAddress[3] = 0xDD;
            _peerMacAddress[4] = 0xEE;
            _peerMacAddress[5] = 0xFF;
        } else {
            // Adresse MAC de la station au sol (à configurer)
            _peerMacAddress[0] = 0x11;  // Exemple, à remplacer par l'adresse réelle
            _peerMacAddress[1] = 0x22;
            _peerMacAddress[2] = 0x33;
            _peerMacAddress[3] = 0x44;
            _peerMacAddress[4] = 0x55;
            _peerMacAddress[5] = 0x66;
        }
        
        // Enregistrer les fonctions de rappel (callbacks)
        esp_now_register_send_cb([](const uint8_t *mac_addr, esp_now_send_status_t status) {
            // Callback statique qui sera redirigé vers la méthode de classe
            if (status != ESP_NOW_SEND_SUCCESS) {
                LOG_DEBUG("COMM", "Erreur d'envoi ESP-NOW");
            }
        });
        
        esp_now_register_recv_cb([](const uint8_t *mac_addr, const uint8_t *data, int data_len) {
            // Callback statique qui doit être redirigé vers l'instance
            // Ceci est géré par une fonction globale externe définie après la classe
        });
        
        // Ajouter le pair
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, _peerMacAddress, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;
        
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            LOG_ERROR("COMM", "Échec d'ajout du pair ESP-NOW");
            return false;
        }
        
        _peerRegistered = true;
        _initialized = true;
        LOG_INFO("COMM", "Module communication initialisé avec succès");
        
        return true;
    }
    
    // Définir le callback pour la réception de données
    void setReceiveCallback(void (*callback)(const IMUPacket&)) {
        _onDataReceived = callback;
    }
    
    // Vérifier si la connexion est active
    bool isConnected() const {
        return _initialized && _peerRegistered && 
               (millis() - _lastReceivedTime < CONNECTION_TIMEOUT);
    }
    
    // Envoyer des données IMU (depuis le kite)
    bool sendIMUData(float roll, float pitch, float yaw) {
        if (!_initialized || !_peerRegistered || _isStationMode) {
            return false;
        }
        
        IMUPacket packet;
        packet.roll = roll;
        packet.pitch = pitch;
        packet.yaw = yaw;
        packet.timestamp = millis();
        
        esp_err_t result = esp_now_send(_peerMacAddress, (uint8_t*)&packet, sizeof(IMUPacket));
        if (result != ESP_OK) {
            _errorCount++;
            return false;
        }
        
        _packetCount++;
        return true;
    }
    
    // Envoyer une commande au kite (depuis la station)
    bool sendCommand(CommandType cmdType, uint8_t value = 0, uint8_t option = 0) {
        if (!_initialized || !_peerRegistered || !_isStationMode) {
            return false;
        }
        
        CommandPacket packet;
        packet.commandType = cmdType;
        packet.value = value;
        packet.option = option;
        packet.checksum = calcChecksum(packet);
        
        esp_err_t result = esp_now_send(_peerMacAddress, (uint8_t*)&packet, sizeof(CommandPacket));
        if (result != ESP_OK) {
            _errorCount++;
            return false;
        }
        
        _packetCount++;
        LOG_DEBUG("COMM", "Commande envoyée: %d, valeur: %d", cmdType, value);
        
        return true;
    }
    
    // Mise à jour périodique à appeler dans la boucle principale
    void update() {
        if (!_initialized) return;
        
        // Gérer les timeouts et autres tâches périodiques
        if (_isStationMode && isConnected()) {
            // La station sol peut envoyer des requêtes périodiques
            static unsigned long lastRequestTime = 0;
            if (millis() - lastRequestTime > 5000) {  // Toutes les 5 secondes
                sendCommand(CMD_REQUEST_STATUS);
                lastRequestTime = millis();
            }
        }
    }
    
    // Traitement des données reçues
    void handleReceivedData(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
        if (!_initialized) return;
        
        _lastReceivedTime = millis();
        
        // Vérifier la source (doit correspondre au pair enregistré)
        if (memcmp(mac_addr, _peerMacAddress, 6) != 0) {
            LOG_WARNING("COMM", "Données reçues d'une adresse MAC inconnue");
            return;
        }
        
        // Traiter selon le type de paquet et le mode
        if (_isStationMode && data_len == sizeof(IMUPacket)) {
            // Station au sol reçoit des données IMU du kite
            const IMUPacket* imuPacket = reinterpret_cast<const IMUPacket*>(data);
            LOG_DEBUG("COMM", "IMU reçu: R=%.1f P=%.1f Y=%.1f", 
                     imuPacket->roll, imuPacket->pitch, imuPacket->yaw);
            
            // Appeler le callback si défini
            if (_onDataReceived) {
                _onDataReceived(*imuPacket);
            }
            
        } else if (!_isStationMode && data_len == sizeof(CommandPacket)) {
            // Kite reçoit des commandes de la station au sol
            const CommandPacket* cmdPacket = reinterpret_cast<const CommandPacket*>(data);
            
            // Vérifier le checksum
            if (calcChecksum(*cmdPacket) != cmdPacket->checksum) {
                LOG_WARNING("COMM", "Commande reçue avec checksum invalide");
                return;
            }
            
            LOG_DEBUG("COMM", "Commande reçue: %d, valeur: %d", 
                     cmdPacket->commandType, cmdPacket->value);
            
            // Exécuter la commande (exemple simplifié)
            switch (cmdPacket->commandType) {
                case CMD_CALIBRATE_IMU:
                    LOG_INFO("COMM", "Commande: Calibration IMU");
                    // TODO: Implémenter la calibration de l'IMU
                    break;
                    
                case CMD_SET_UPDATE_RATE:
                    _updateRate = cmdPacket->value * 10;  // 10ms par unité
                    LOG_INFO("COMM", "Nouvelle fréquence d'update: %d ms", _updateRate);
                    break;
                    
                case CMD_EMERGENCY:
                    LOG_WARNING("COMM", "COMMANDE D'URGENCE REÇUE");
                    // TODO: Implémenter la réponse d'urgence
                    break;
                    
                default:
                    LOG_DEBUG("COMM", "Commande inconnue: %d", cmdPacket->commandType);
                    break;
            }
        } else {
            LOG_WARNING("COMM", "Format de données reçues inconnu (taille: %d)", data_len);
        }
    }
    
    // Statistiques
    uint32_t getPacketCount() const { return _packetCount; }
    uint32_t getErrorCount() const { return _errorCount; }
    unsigned long getLastReceiveTime() const { return _lastReceivedTime; }
    
private:
    bool _initialized;            // État d'initialisation
    bool _isStationMode;          // true = station au sol, false = kite
    bool _peerRegistered;         // Pair enregistré avec succès
    uint8_t _selfMacAddress[6];   // Adresse MAC de cet ESP32
    uint8_t _peerMacAddress[6];   // Adresse MAC du pair (kite ou station)
    unsigned long _lastReceivedTime; // Dernier paquet reçu
    uint32_t _packetCount;        // Nombre de paquets envoyés
    uint32_t _errorCount;         // Nombre d'erreurs de transmission
    uint16_t _updateRate;         // Intervalle entre les messages (ms)
    
    // Callback pour la réception de données
    void (*_onDataReceived)(const IMUPacket&);
    
    // Constantes
    static const unsigned long CONNECTION_TIMEOUT = 5000;  // Timeout de connexion (5s)
    
    // Calcul du checksum pour la validation des paquets
    uint8_t calcChecksum(const CommandPacket& packet) const {
        return (packet.commandType + packet.value + packet.option) & 0xFF;
    }
};

// Variable globale pour stocker l'instance (permet d'accéder depuis le callback static)
CommunicationModule* g_commInstance = nullptr;

// Fonction globale qui sera utilisée comme callback de réception ESP-NOW
void IRAM_ATTR onEspNowDataReceived(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
    if (g_commInstance) {
        g_commInstance->handleReceivedData(mac_addr, data, data_len);
    }
}

// Fonction d'initialisation à appeler dans le setup() après la création de l'instance
void registerCommInstance(CommunicationModule* instance) {
    g_commInstance = instance;
    esp_now_register_recv_cb(onEspNowDataReceived);
}

#endif // COMMUNICATION_MODULE_H
