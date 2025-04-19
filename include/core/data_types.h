/*
 * Types de données pour le projet Kite Pilote
 * 
 * Ce fichier contient toutes les structures de données partagées entre les modules.
 * Structures optimisées pour minimiser l'utilisation de la mémoire et éviter la fragmentation.
 * Utilise des tampons statiques (char[]) au lieu de String pour une meilleure stabilité.
 * 
 * Créé le: 17/04/2025
 * Mis à jour le: 19/04/2025
 */

#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <Arduino.h>
#include "config.h"

// Constantes pour les tailles des tampons
#define STATUS_MESSAGE_SIZE 32  // Taille maximale des messages de statut

//===============================================================
// STRUCTURES DE DONNÉES DES CAPTEURS
//===============================================================

/**
 * @struct SensorData
 * @brief Structure de base pour les données de capteurs avec attributs communs
 */
struct SensorData {
    uint32_t timestamp;     // Horodatage de la mesure (millisecondes)
    bool isValid;           // Indique si les données sont valides
    
    // Constructeur par défaut
    SensorData() : timestamp(0), isValid(false) {}
    
    // Méthode pour vérifier si les données sont récentes
    bool isRecent(uint32_t maxAgeMs = 1000) const {
        return isValid && (millis() - timestamp <= maxAgeMs);
    }
    
    // Mise à jour de l'horodatage
    void updateTimestamp() {
        timestamp = millis();
    }
};

/**
 * @struct IMUData
 * @brief Données de l'IMU (Unité de Mesure Inertielle)
 */
struct IMUData : public SensorData {
    float roll;             // Roulis (rotation autour de l'axe X) en degrés
    float pitch;            // Tangage (rotation autour de l'axe Y) en degrés
    float yaw;              // Lacet (rotation autour de l'axe Z) en degrés
    
    // Accélérations brutes (optionnelles, pour analyses avancées)
    float accX;             // Accélération en X (G)
    float accY;             // Accélération en Y (G)
    float accZ;             // Accélération en Z (G)
    
    // Constructeur par défaut avec valeurs initiales
    IMUData() : 
        SensorData(),
        roll(0.0f), 
        pitch(0.0f), 
        yaw(0.0f),
        accX(0.0f),
        accY(0.0f),
        accZ(0.0f)
    {}
    
    // Réinitialiser les données
    void reset() {
        isValid = false;
        roll = pitch = yaw = 0.0f;
        accX = accY = accZ = 0.0f;
    }
    
    // Vérifier si les valeurs sont dans des limites plausibles
    bool hasValidRange() const {
        return isValid && 
               roll >= -180.0f && roll <= 180.0f &&
               pitch >= -90.0f && pitch <= 90.0f &&
               yaw >= 0.0f && yaw < 360.0f;
    }
};

/**
 * @struct WindData
 * @brief Données du vent (vitesse et direction)
 */
struct WindData : public SensorData {
    float speed;            // Vitesse du vent en m/s
    float direction;        // Direction du vent en degrés (0-359, 0 = Nord)
    float gustSpeed;        // Vitesse maximale des rafales en m/s
    
    // Constructeur par défaut
    WindData() : 
        SensorData(),
        speed(0.0f), 
        direction(0.0f),
        gustSpeed(0.0f)
    {}
    
    // Réinitialiser les données
    void reset() {
        isValid = false;
        speed = direction = gustSpeed = 0.0f;
    }
    
    // Convertir m/s en nœuds (pour affichage)
    float getSpeedInKnots() const {
        return speed * 1.94384f;
    }
    
    // Vérifier si les valeurs sont dans des limites plausibles
    bool hasValidRange() const {
        return isValid && 
               speed >= 0.0f && speed <= 50.0f &&  // 0-50 m/s (~0-180 km/h)
               direction >= 0.0f && direction < 360.0f;
    }
    
    // Déterminer si les conditions de vent sont bonnes pour le vol
    bool isSafeForFlight() const {
        return isValid && 
               speed >= 3.0f &&     // Au moins 3 m/s (~11 km/h)
               speed <= 15.0f &&    // Maximum 15 m/s (~54 km/h)
               gustSpeed <= 20.0f;  // Rafales max 20 m/s (~72 km/h)
    }
};

/**
 * @struct LineData
 * @brief Données des lignes du kite (tension et longueur)
 */
struct LineData : public SensorData {
    float tension;           // Tension de la ligne en Newtons
    float length;            // Longueur de la ligne en mètres
    bool isTensionValid;     // Validité du capteur de tension
    bool isLengthValid;      // Validité du capteur de longueur
    float maxTension;        // Tension maximale enregistrée
    
    // Constructeur par défaut
    LineData() : 
        SensorData(),
        tension(0.0f), 
        length(0.0f), 
        isTensionValid(false),
        isLengthValid(false),
        maxTension(0.0f)
    {}
    
    // Réinitialiser les données
    void reset() {
        isValid = isTensionValid = isLengthValid = false;
        tension = length = 0.0f;
        maxTension = 0.0f;
    }
    
    // Mettre à jour la tension maximale
    void updateMaxTension() {
        if (isTensionValid && tension > maxTension) {
            maxTension = tension;
        }
    }
    
    // Convertir la tension en kg (pour affichage)
    float getTensionInKg() const {
        return tension / 9.81f;  // Conversion N en kg (approx)
    }
    
    // Vérifier si la tension est dans des limites sécuritaires
    bool isTensionSafe(float maxSafeTension = 500.0f) const {
        return isTensionValid && tension < maxSafeTension;
    }
};

//===============================================================
// STRUCTURES DE DONNÉES DES ACTIONNEURS
//===============================================================

/**
 * @struct ServoState
 * @brief État des servomoteurs
 */
struct ServoState {
    float directionAngle;    // Angle du servo de direction (-45 à +45)
    float trimAngle;         // Angle du servo de trim (-30 à +30)
    WinchMode winchMode;     // Mode actuel du treuil
    float winchPower;        // Puissance actuelle du treuil (0-100%)
    uint32_t lastUpdateTime; // Dernier horodatage de mise à jour
    
    // Constructeur par défaut
    ServoState() : 
        directionAngle(DIRECTION_CENTER), 
        trimAngle(TRIM_CENTER), 
        winchMode(WINCH_MODE_IDLE),
        winchPower(0.0f),
        lastUpdateTime(0)
    {}
    
    // Vérifier si les valeurs d'angle sont dans les limites
    bool isDirectionInLimits() const {
        return directionAngle >= DIRECTION_MIN_ANGLE && directionAngle <= DIRECTION_MAX_ANGLE;
    }
    
    bool isTrimInLimits() const {
        return trimAngle >= TRIM_MIN_ANGLE && trimAngle <= TRIM_MAX_ANGLE;
    }
    
    bool isWinchPowerInLimits() const {
        return winchPower >= WINCH_MIN_POWER && winchPower <= WINCH_MAX_POWER;
    }
    
    // Limiter les valeurs aux plages acceptables
    void clampValues() {
        directionAngle = constrain(directionAngle, DIRECTION_MIN_ANGLE, DIRECTION_MAX_ANGLE);
        trimAngle = constrain(trimAngle, TRIM_MIN_ANGLE, TRIM_MAX_ANGLE);
        winchPower = constrain(winchPower, WINCH_MIN_POWER, WINCH_MAX_POWER);
    }
};

//===============================================================
// STRUCTURES DE DONNÉES SYSTÈME
//===============================================================

/**
 * @struct SystemStatus
 * @brief État du système
 */
struct SystemStatus {
    bool isInitialized;      // Système correctement initialisé
    bool isError;            // Erreur système active
    ErrorCode lastError;     // Dernier code d'erreur
    uint32_t uptime;         // Temps de fonctionnement en ms
    float batteryVoltage;    // Tension de la batterie en volts
    float cpuTemperature;    // Température du CPU en °C
    uint32_t freeMemory;     // Mémoire libre en octets
    uint32_t wifiConnections;// Nombre de connexions WiFi actives
    
    // Constructeur par défaut
    SystemStatus() : 
        isInitialized(false), 
        isError(false), 
        lastError(ERROR_NONE),
        uptime(0),
        batteryVoltage(0.0f),
        cpuTemperature(0.0f),
        freeMemory(0),
        wifiConnections(0)
    {}
    
    // Vérifier la santé globale du système
    bool isHealthy() const {
        return isInitialized && 
               !isError && 
               batteryVoltage > 11.0f &&    // Seuil minimal batterie
               cpuTemperature < 80.0f;      // Seuil température
    }
    
    // Convertir le temps de fonctionnement en format lisible (HH:MM:SS)
    void getUptimeString(char* buffer, size_t bufferSize) const {
        uint32_t seconds = uptime / 1000;
        uint8_t hours = seconds / 3600;
        uint8_t minutes = (seconds % 3600) / 60;
        uint8_t secs = seconds % 60;
        
        snprintf(buffer, bufferSize, "%02d:%02d:%02d", hours, minutes, secs);
    }
};

/**
 * @struct AutopilotStatus
 * @brief État de l'autopilote
 */
struct AutopilotStatus {
    AutopilotMode mode;                 // Mode actuel de l'autopilote
    float completionPercent;            // Pourcentage de complétion (pour launch/land)
    char statusMessage[STATUS_MESSAGE_SIZE]; // Message de statut textuel
    float powerGenerated;               // Puissance générée en watts
    float totalEnergy;                  // Énergie totale générée en watt-heures
    uint32_t sequenceStartTime;         // Horodatage du début de séquence
    uint16_t flightCycles;              // Compteur de cycles de vol
    bool isTargetReached;               // Indique si la cible actuelle est atteinte
    
    // Constructeur par défaut
    AutopilotStatus() : 
        mode(AUTOPILOT_OFF), 
        completionPercent(0.0f), 
        powerGenerated(0.0f),
        totalEnergy(0.0f),
        sequenceStartTime(0),
        flightCycles(0),
        isTargetReached(false)
    {
        strncpy(statusMessage, "Inactif", STATUS_MESSAGE_SIZE - 1);
        statusMessage[STATUS_MESSAGE_SIZE - 1] = '\0'; // S'assurer que la chaîne est terminée
    }
    
    // Définir le message de statut de manière sécurisée
    void setStatusMessage(const char* message) {
        strncpy(statusMessage, message, STATUS_MESSAGE_SIZE - 1);
        statusMessage[STATUS_MESSAGE_SIZE - 1] = '\0';
    }
    
    // Calculer la durée écoulée depuis le début de la séquence
    uint32_t getSequenceDuration() const {
        return (sequenceStartTime > 0) ? (millis() - sequenceStartTime) : 0;
    }
    
    // Démarrer une nouvelle séquence
    void startNewSequence() {
        sequenceStartTime = millis();
        completionPercent = 0.0f;
        isTargetReached = false;
    }
    
    // Incrémenter le compteur de cycles avec sécurité
    void incrementFlightCycle() {
        if (flightCycles < UINT16_MAX) {
            flightCycles++;
        }
    }
    
    // Ajouter de l'énergie au compteur total avec protection contre les valeurs négatives
    void addEnergy(float wattHours) {
        if (wattHours > 0) {
            totalEnergy += wattHours;
        }
    }
};

#endif // DATA_TYPES_H
