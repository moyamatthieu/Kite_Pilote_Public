/*
 * Module de gestion des servomoteurs pour le projet Kite Pilote
 * 
 * Ce module contrôle les trois servomoteurs du système:
 * - Servo Direction: Contrôle la direction gauche/droite du kite
 * - Servo Trim: Contrôle l'angle d'incidence (puissance) du kite
 * - Servo Treuil/Générateur: Gestion de l'enroulement/déroulement et génération d'énergie
 * 
 * Créé le: 17/04/2025
 */

#ifndef SERVO_MODULE_H
#define SERVO_MODULE_H

#include <Arduino.h>
#include <ESP32Servo.h>
#include "../core/config.h"
#include "../core/data_types.h"
#include "../utils/logger.h"

class ServoModule {
public:
    // Constructeur
    ServoModule() : 
        _directionInitialized(false),
        _trimInitialized(false),
        _winchInitialized(false),
        _currentState()  // Initialise avec les valeurs par défaut (voir ServoState)
    {
        // Reste de l'initialisation dans begin()
    }
    
    // Initialisation des servomoteurs
    bool begin() {
        bool success = true;
        
        LOG_INFO("SERVO", "Initialisation des servomoteurs...");
        
        // Initialisation du servo de direction
        _servoDirection.setPeriodHertz(50);  // Fréquence standard pour la plupart des servos
        if (_servoDirection.attach(SERVO_DIRECTION_PIN, SERVO_MIN_PULSE, SERVO_MAX_PULSE)) {
            _directionInitialized = true;
            LOG_INFO("SERVO", "Servo Direction initialisé sur la broche %d", SERVO_DIRECTION_PIN);
        } else {
            LOG_ERROR("SERVO", "Échec d'initialisation du servo Direction");
            success = false;
        }
        
        // Initialisation du servo de trim
        _servoTrim.setPeriodHertz(50);
        if (_servoTrim.attach(SERVO_TRIM_PIN, SERVO_MIN_PULSE, SERVO_MAX_PULSE)) {
            _trimInitialized = true;
            LOG_INFO("SERVO", "Servo Trim initialisé sur la broche %d", SERVO_TRIM_PIN);
        } else {
            LOG_ERROR("SERVO", "Échec d'initialisation du servo Trim");
            success = false;
        }
        
        // Initialisation du servo de treuil/générateur
        _servoWinch.setPeriodHertz(50);
        if (_servoWinch.attach(SERVO_WINCH_PIN, SERVO_MIN_PULSE, SERVO_MAX_PULSE)) {
            _winchInitialized = true;
            LOG_INFO("SERVO", "Servo Treuil initialisé sur la broche %d", SERVO_WINCH_PIN);
        } else {
            LOG_ERROR("SERVO", "Échec d'initialisation du servo Treuil");
            success = false;
        }
        
        // Si tout s'est bien passé, positionner les servos en position neutre
        if (success) {
            neutralPosition();
        }
        
        return success;
    }
    
    // Contrôle du servo de direction (angle entre -45° et +45°)
    bool setDirectionAngle(float angle) {
        if (!_directionInitialized) {
            LOG_ERROR("SERVO", "Servo Direction non initialisé");
            return false;
        }
        
        // Limiter l'angle aux valeurs min/max
        angle = constrain(angle, DIRECTION_MIN_ANGLE, DIRECTION_MAX_ANGLE);
        
        // Convertir l'angle en impulsion et positionner le servo
        _servoDirection.write(mapAngleToDegrees(angle, DIRECTION_MIN_ANGLE, DIRECTION_MAX_ANGLE));
        
        // Mettre à jour l'état actuel
        _currentState.directionAngle = angle;
        
        LOG_DEBUG("SERVO", "Direction réglée à %.1f degrés", angle);
        
        return true;
    }
    
    // Contrôle du servo de trim (angle entre -30° et +30°)
    bool setTrimAngle(float angle) {
        if (!_trimInitialized) {
            LOG_ERROR("SERVO", "Servo Trim non initialisé");
            return false;
        }
        
        // Limiter l'angle aux valeurs min/max
        angle = constrain(angle, TRIM_MIN_ANGLE, TRIM_MAX_ANGLE);
        
        // Convertir l'angle en impulsion et positionner le servo
        _servoTrim.write(mapAngleToDegrees(angle, TRIM_MIN_ANGLE, TRIM_MAX_ANGLE));
        
        // Mettre à jour l'état actuel
        _currentState.trimAngle = angle;
        
        LOG_DEBUG("SERVO", "Trim réglé à %.1f degrés", angle);
        
        return true;
    }
    
    // Contrôle du treuil/générateur (mode et puissance)
    bool setWinchMode(WinchMode mode) {
        if (!_winchInitialized) {
            LOG_ERROR("SERVO", "Servo Treuil non initialisé");
            return false;
        }
        
        _currentState.winchMode = mode;
        
        // Appliquer les réglages selon le mode
        switch (mode) {
            case WINCH_MODE_GENERATOR:
                // En mode générateur, utiliser la puissance actuelle
                return setWinchPower(_currentState.winchPower);
                
            case WINCH_MODE_REELING_IN:
                // Enroulement: vitesse maximale dans un sens
                _servoWinch.write(180);  // Utiliser 180° pour vitesse maximale
                LOG_INFO("SERVO", "Treuil: Mode enroulement");
                break;
                
            case WINCH_MODE_REELING_OUT:
                // Déroulement: vitesse maximale dans l'autre sens
                _servoWinch.write(0);   // Utiliser 0° pour vitesse maximale inverse
                LOG_INFO("SERVO", "Treuil: Mode déroulement");
                break;
                
            case WINCH_MODE_BRAKE:
                // Frein: maintenir la position actuelle
                _servoWinch.write(90);  // Position centrale = arrêt
                LOG_INFO("SERVO", "Treuil: Mode frein");
                break;
                
            case WINCH_MODE_IDLE:
                // Repos: aucune résistance
                _servoWinch.write(90);  // Position centrale = arrêt
                LOG_INFO("SERVO", "Treuil: Mode repos");
                break;
        }
        
        return true;
    }
    
    // Contrôle de la puissance du générateur (0-100%)
    bool setWinchPower(float power) {
        if (!_winchInitialized) {
            LOG_ERROR("SERVO", "Servo Treuil non initialisé");
            return false;
        }
        
        // Vérifier que nous sommes en mode générateur
        if (_currentState.winchMode != WINCH_MODE_GENERATOR) {
            LOG_WARNING("SERVO", "Impossible de régler la puissance en dehors du mode générateur");
            return false;
        }
        
        // Limiter la puissance entre 0 et 100%
        power = constrain(power, WINCH_MIN_POWER, WINCH_MAX_POWER);
        
        // Convertir la puissance en angle du servo (0% = 90°, 100% = 180°)
        int servoAngle = map(power, WINCH_MIN_POWER, WINCH_MAX_POWER, 90, 180);
        _servoWinch.write(servoAngle);
        
        // Mettre à jour l'état actuel
        _currentState.winchPower = power;
        
        LOG_DEBUG("SERVO", "Puissance générateur réglée à %.1f%%", power);
        
        return true;
    }
    
    // Mettre tous les servos en position neutre
    void neutralPosition() {
        LOG_INFO("SERVO", "Positionnement des servos en position neutre");
        
        // Direction au centre (0°)
        setDirectionAngle(DIRECTION_CENTER);
        
        // Trim au centre (0°)
        setTrimAngle(TRIM_CENTER);
        
        // Treuil en mode repos
        setWinchMode(WINCH_MODE_IDLE);
    }
    
    // Arrêt d'urgence
    void emergencyStop() {
        LOG_WARNING("SERVO", "ARRÊT D'URGENCE des servomoteurs");
        
        // Direction au centre
        if (_directionInitialized) {
            setDirectionAngle(DIRECTION_CENTER);
        }
        
        // Trim au minimum de puissance
        if (_trimInitialized) {
            setTrimAngle(TRIM_MIN_ANGLE);
        }
        
        // Treuil en mode frein
        if (_winchInitialized) {
            setWinchMode(WINCH_MODE_BRAKE);
        }
    }
    
    // Obtenir l'état actuel des servomoteurs
    ServoState getState() const {
        return _currentState;
    }
    
    // Getters individuels pour chaque paramètre
    float getDirectionAngle() const { return _currentState.directionAngle; }
    float getTrimAngle() const { return _currentState.trimAngle; }
    WinchMode getWinchMode() const { return _currentState.winchMode; }
    float getWinchPower() const { return _currentState.winchPower; }
    
    // Vérifier si les servos sont correctement initialisés
    bool isInitialized() const {
        return _directionInitialized && _trimInitialized && _winchInitialized;
    }
    
    // Mise à jour périodique (pour les actions qui nécessitent un suivi temps réel)
    void update() {
        // Rien à faire pour le moment
        // Pourrait contenir des animations ou des séquences automatiques
    }

private:
    // Objets servomoteurs
    Servo _servoDirection;    // Servo pour la direction gauche/droite
    Servo _servoTrim;         // Servo pour l'angle d'incidence (puissance)
    Servo _servoWinch;        // Servo pour le treuil/générateur
    
    // États d'initialisation
    bool _directionInitialized;
    bool _trimInitialized;
    bool _winchInitialized;
    
    // État actuel des servomoteurs
    ServoState _currentState;
    
    // Convertir l'angle du kite (-45° à +45°) en angle du servo (0° à 180°)
    int mapAngleToDegrees(float angle, float minAngle, float maxAngle) {
        return map(angle * 10, minAngle * 10, maxAngle * 10, 0, 180) / 10;
    }
    
    // Mapper une valeur float (méthode utilitaire)
    static float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }
};

#endif // SERVO_MODULE_H
