/*
 * Module de gestion des capteurs pour le projet Kite Pilote
 * 
 * Ce module centralise la gestion de tous les capteurs du système:
 * - IMU (Inertial Measurement Unit) pour l'orientation du kite
 * - Capteur de tension des lignes
 * - Capteur de longueur des lignes
 * - Anémomètre/Girouette pour les données de vent
 * 
 * Créé le: 17/04/2025
 */

#ifndef SENSOR_MODULE_H
#define SENSOR_MODULE_H

#include <Arduino.h>
#include "../core/config.h"
#include "../core/data_types.h"
#include "../utils/logger.h"

class SensorModule {
public:
    // Constructeur
    SensorModule() {
        resetAllData();
    }
    
    // Initialisation des capteurs
    bool begin() {
        bool success = true;
        
        LOG_INFO("CAPTEUR", "Initialisation des capteurs...");
        
        // En mode simulation, les capteurs sont virtuels
        #ifdef SIMULATION_MODE
            LOG_INFO("CAPTEUR", "Mode simulation activé, utilisation de capteurs virtuels");
            _imuInitialized = true;
            _tensionSensorInitialized = true;
            _lengthSensorInitialized = true;
            _windSensorInitialized = true;
        #else
            // Dans un système réel, ici nous initialiserions chaque capteur physique
            // Exemple non fonctionnel uniquement pour illustrer:
            // _imuInitialized = initIMU();
            // _tensionSensorInitialized = initTensionSensor();
            // _lengthSensorInitialized = initLengthSensor();
            // _windSensorInitialized = initWindSensor();
            
            // TODO: Implémenter l'initialisation des capteurs réels
            LOG_WARNING("CAPTEUR", "L'initialisation des capteurs réels n'est pas implémentée");
            
            // Pour cette version on simule que tout est correct
            _imuInitialized = true;
            _tensionSensorInitialized = true;
            _lengthSensorInitialized = true;
            _windSensorInitialized = true;
        #endif
        
        // Vérifier que l'initialisation a réussi pour tous les capteurs essentiels
        if (!_imuInitialized) {
            LOG_ERROR("CAPTEUR", "Échec de l'initialisation de l'IMU");
            success = false;
        }
        
        if (!_tensionSensorInitialized) {
            LOG_ERROR("CAPTEUR", "Échec de l'initialisation du capteur de tension");
            success = false;
        }
        
        // L'anémomètre est optionnel, donc pas d'échec global si pas initialisé
        if (!_windSensorInitialized) {
            LOG_WARNING("CAPTEUR", "Anémomètre non disponible");
        }
        
        return success;
    }
    
    // Mise à jour de tous les capteurs (à appeler dans la boucle principale)
    void update() {
        // En mode simulation, les données sont mises à jour par la fonction externe
        // updateSensorsWithSimulationData() dans simulation.cpp
        #ifndef SIMULATION_MODE
            // En mode normal, nous lirions les capteurs physiques ici
            updateIMU();
            updateLineTension();
            updateLineLength();
            updateWindSensor();
        #endif
        
        // Filtrer les données brutes pour éliminer le bruit
        filterData();
    }
    
    // Obtenir les données actuelles de l'IMU
    IMUData getIMUData() const {
        return _imuData;
    }
    
    // Obtenir les données actuelles des lignes
    LineData getLineData() const {
        return _lineData;
    }
    
    // Obtenir les données actuelles du vent
    WindData getWindData() const {
        return _windData;
    }
    
    // Raccourcis vers les valeurs individuelles les plus utilisées
    float getRoll() const { return _imuData.roll; }
    float getPitch() const { return _imuData.pitch; }
    float getYaw() const { return _imuData.yaw; }
    float getLineTension() const { return _lineData.tension; }
    float getLineLength() const { return _lineData.length; }
    float getWindSpeed() const { return _windData.speed; }
    float getWindDirection() const { return _windData.direction; }
    
    // Vérifier la validité de tous les capteurs essentiels
    bool allSensorsValid() const {
        return _imuData.isValid && _lineData.isTensionValid;
    }
    
    // Méthodes d'accès pour la simulation
    void setIMUData(float roll, float pitch, float yaw) {
        _imuData.roll = roll;
        _imuData.pitch = pitch;
        _imuData.yaw = yaw;
        _imuData.isValid = true;
        _imuData.timestamp = millis();
    }
    
    void setLineTension(float tension) {
        _lineData.tension = tension;
        _lineData.isTensionValid = true;
        _lineData.timestamp = millis();
    }
    
    void setLineLength(float length) {
        _lineData.length = length;
        _lineData.isLengthValid = true;
        _lineData.timestamp = millis();
    }
    
    void setWindData(float speed, float direction) {
        _windData.speed = speed;
        _windData.direction = direction;
        _windData.isValid = true;
        _windData.timestamp = millis();
    }

private:
    // Données des capteurs
    IMUData _imuData;
    LineData _lineData;
    WindData _windData;
    
    // États d'initialisation des capteurs
    bool _imuInitialized;
    bool _tensionSensorInitialized;
    bool _lengthSensorInitialized;
    bool _windSensorInitialized;
    
    // Constantes pour le filtrage
    const float IMU_FILTER_ALPHA = 0.8f;  // Coefficient du filtre passe-bas pour l'IMU
    const float TENSION_FILTER_ALPHA = 0.7f; // Coefficient pour la tension
    const float LENGTH_FILTER_ALPHA = 0.9f;  // Coefficient pour la longueur
    const float WIND_FILTER_ALPHA = 0.7f;    // Coefficient pour le vent
    
    // Valeurs filtrées précédentes (pour le filtre passe-bas)
    float _filteredRoll;
    float _filteredPitch;
    float _filteredYaw;
    float _filteredTension;
    float _filteredLength;
    float _filteredWindSpeed;
    float _filteredWindDirection;
    
    // Réinitialiser toutes les données capteurs
    void resetAllData() {
        // Réinitialiser les données de l'IMU
        _imuData = IMUData();
        
        // Réinitialiser les données des lignes
        _lineData = LineData();
        
        // Réinitialiser les données du vent
        _windData = WindData();
        
        // Réinitialiser les états d'initialisation
        _imuInitialized = false;
        _tensionSensorInitialized = false;
        _lengthSensorInitialized = false;
        _windSensorInitialized = false;
        
        // Réinitialiser les valeurs filtrées
        _filteredRoll = 0.0f;
        _filteredPitch = 0.0f;
        _filteredYaw = 0.0f;
        _filteredTension = 0.0f;
        _filteredLength = 0.0f;
        _filteredWindSpeed = 0.0f;
        _filteredWindDirection = 0.0f;
    }
    
    // Lecture de l'IMU (implémentation pour capteurs réels)
    bool updateIMU() {
        // TODO: Implémenter la lecture réelle de l'IMU
        // Exemple fictif:
        /*
        if (_imuInitialized) {
            // Lire les données brutes du capteur
            float rawRoll = imu.getRoll();
            float rawPitch = imu.getPitch();
            float rawYaw = imu.getYaw();
            
            // Mettre à jour les données
            _imuData.roll = rawRoll;
            _imuData.pitch = rawPitch;
            _imuData.yaw = rawYaw;
            _imuData.isValid = true;
            _imuData.timestamp = millis();
            
            return true;
        }
        */
        return false; // Non implémenté dans cette version
    }
    
    // Lecture du capteur de tension
    bool updateLineTension() {
        // TODO: Implémenter la lecture réelle du capteur de tension
        return false; // Non implémenté dans cette version
    }
    
    // Lecture du capteur de longueur
    bool updateLineLength() {
        // TODO: Implémenter la lecture réelle du capteur de longueur
        return false; // Non implémenté dans cette version
    }
    
    // Lecture de l'anémomètre/girouette
    bool updateWindSensor() {
        // TODO: Implémenter la lecture réelle des capteurs de vent
        return false; // Non implémenté dans cette version
    }
    
    // Filtrage des données pour réduire le bruit
    void filterData() {
        // Filtre passe-bas simple pour l'IMU
        if (_imuData.isValid) {
            _filteredRoll = IMU_FILTER_ALPHA * _filteredRoll + (1 - IMU_FILTER_ALPHA) * _imuData.roll;
            _filteredPitch = IMU_FILTER_ALPHA * _filteredPitch + (1 - IMU_FILTER_ALPHA) * _imuData.pitch;
            _filteredYaw = IMU_FILTER_ALPHA * _filteredYaw + (1 - IMU_FILTER_ALPHA) * _imuData.yaw;
            
            _imuData.roll = _filteredRoll;
            _imuData.pitch = _filteredPitch;
            _imuData.yaw = _filteredYaw;
        }
        
        // Filtre pour la tension des lignes
        if (_lineData.isTensionValid) {
            _filteredTension = TENSION_FILTER_ALPHA * _filteredTension + 
                              (1 - TENSION_FILTER_ALPHA) * _lineData.tension;
            _lineData.tension = _filteredTension;
        }
        
        // Filtre pour la longueur des lignes
        if (_lineData.isLengthValid) {
            _filteredLength = LENGTH_FILTER_ALPHA * _filteredLength + 
                             (1 - LENGTH_FILTER_ALPHA) * _lineData.length;
            _lineData.length = _filteredLength;
        }
        
        // Filtre pour les données de vent
        if (_windData.isValid) {
            _filteredWindSpeed = WIND_FILTER_ALPHA * _filteredWindSpeed + 
                                (1 - WIND_FILTER_ALPHA) * _windData.speed;
            
            // Filtrage directionnel spécial pour éviter les problèmes à 0/360 degrés
            float sinDirection = sin(_windData.direction * PI / 180.0);
            float cosDirection = cos(_windData.direction * PI / 180.0);
            
            static float filteredSinDirection = sinDirection;
            static float filteredCosDirection = cosDirection;
            
            filteredSinDirection = WIND_FILTER_ALPHA * filteredSinDirection + 
                                  (1 - WIND_FILTER_ALPHA) * sinDirection;
            filteredCosDirection = WIND_FILTER_ALPHA * filteredCosDirection + 
                                  (1 - WIND_FILTER_ALPHA) * cosDirection;
            
            _filteredWindDirection = atan2(filteredSinDirection, filteredCosDirection) * 180.0 / PI;
            if (_filteredWindDirection < 0) _filteredWindDirection += 360.0;
            
            _windData.speed = _filteredWindSpeed;
            _windData.direction = _filteredWindDirection;
        }
    }
};

#endif // SENSOR_MODULE_H
