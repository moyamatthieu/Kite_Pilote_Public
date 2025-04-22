/*
 * Module de gestion des servomoteurs et du moteur pas à pas pour le projet Kite Pilote
 * 
 * Ce module contrôle les actionneurs du système:
 * - Servo Direction: Contrôle la direction gauche/droite du kite
 * - Servo Trim: Contrôle l'angle d'incidence (puissance) du kite
 * - Stepper Winch: Gestion de l'enroulement/déroulement et récupération d'énergie avec moteur pas à pas
 * 
 * Créé le: 17/04/2025
 * Mise à jour le: 22/04/2025 - Remplacement du servo winch par un stepper motor
 */

#ifndef SERVO_MODULE_H
#define SERVO_MODULE_H

#include <Arduino.h>
#include <ESP32Servo.h>
#include "../utils/simple_stepper.h"
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
        _currentState(),  // Initialise avec les valeurs par défaut (voir ServoState)
        _stepperWinch(STEPPER_STEPS_PER_REV, STEPPER_WINCH_PIN1, STEPPER_WINCH_PIN2, 
                      STEPPER_WINCH_PIN3, STEPPER_WINCH_PIN4),
        _lastStepDirection(0),
        _lastStepTime(0)
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
        
        // Initialisation du moteur pas à pas pour le treuil/générateur
        try {
            // Configuration de la vitesse du moteur pas à pas
            _stepperWinch.setSpeed(STEPPER_MAX_SPEED / 2); // Vitesse initiale à mi-régime
            _winchInitialized = true;
            LOG_INFO("SERVO", "Stepper Winch initialisé sur les broches %d, %d, %d, %d",
                STEPPER_WINCH_PIN1, STEPPER_WINCH_PIN2, STEPPER_WINCH_PIN3, STEPPER_WINCH_PIN4);
        } catch (const std::exception& e) {
            LOG_ERROR("SERVO", "Échec d'initialisation du Stepper Winch: %s", e.what());
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
    
    // Contrôle du treuil/générateur avec moteur pas à pas (mode et puissance)
    bool setWinchMode(WinchMode mode) {
        if (!_winchInitialized) {
            LOG_ERROR("SERVO", "Stepper Winch non initialisé");
            return false;
        }
        
        _currentState.winchMode = mode;
        _lastStepDirection = 0; // Réinitialiser la direction à l'arrêt par défaut
        
        // Appliquer les réglages selon le mode
        switch (mode) {
            case WINCH_MODE_GENERATOR:
                // En mode générateur, utiliser la puissance actuelle
                return setWinchPower(_currentState.winchPower);
                
            case WINCH_MODE_REELING_IN:
                // Enroulement: vitesse maximale dans le sens horaire
                _stepperWinch.setSpeed(STEPPER_MAX_SPEED);
                _lastStepDirection = 1;  // Direction positive (enroulement)
                LOG_INFO("SERVO", "Treuil: Mode enroulement (stepper)");
                break;
                
            case WINCH_MODE_REELING_OUT:
                // Déroulement: vitesse maximale dans le sens anti-horaire
                _stepperWinch.setSpeed(STEPPER_MAX_SPEED);
                _lastStepDirection = -1;  // Direction négative (déroulement)
                LOG_INFO("SERVO", "Treuil: Mode déroulement (stepper)");
                break;
                
            case WINCH_MODE_BRAKE:
                // Frein: couper l'alimentation des bobines pour maintenir la position
                // La résistance magnétique naturelle du stepper maintient la position
                _lastStepDirection = 0;  // Arrêt (pas de mouvement)
                LOG_INFO("SERVO", "Treuil: Mode frein (stepper)");
                break;
                
            case WINCH_MODE_IDLE:
                // Repos: aucune résistance (moteur désactivé)
                _lastStepDirection = 0;  // Arrêt (pas de mouvement)
                
                // Approche plus compatible pour désactiver le moteur
                // Au lieu de modifier les modes des pins, on peut simplement
                // éviter d'envoyer des signaux au moteur en laissant _lastStepDirection à 0
                // La méthode update() ne fera pas bouger le moteur
                
                // Si nécessaire, on peut mettre les sorties à LOW pour désactiver les bobines
                digitalWrite(STEPPER_WINCH_PIN1, LOW);
                digitalWrite(STEPPER_WINCH_PIN2, LOW);
                digitalWrite(STEPPER_WINCH_PIN3, LOW);
                digitalWrite(STEPPER_WINCH_PIN4, LOW);
                
                LOG_INFO("SERVO", "Treuil: Mode repos (stepper)");
                break;
        }
        
        return true;
    }
    
    // Contrôle de la puissance du générateur avec moteur pas à pas (0-100%)
    bool setWinchPower(float power) {
        if (!_winchInitialized) {
            LOG_ERROR("SERVO", "Stepper Winch non initialisé");
            return false;
        }
        
        // Vérifier que nous sommes en mode générateur
        if (_currentState.winchMode != WINCH_MODE_GENERATOR) {
            LOG_WARNING("SERVO", "Impossible de régler la puissance en dehors du mode générateur");
            return false;
        }
        
        // Limiter la puissance entre 0 et 100%
        power = constrain(power, WINCH_MIN_POWER, WINCH_MAX_POWER);
        
        // Pour le mode générateur, la puissance détermine la résistance
        // Réinitialiser les pins du stepper pour assurer le contrôle
        pinMode(STEPPER_WINCH_PIN1, OUTPUT);
        pinMode(STEPPER_WINCH_PIN2, OUTPUT);
        pinMode(STEPPER_WINCH_PIN3, OUTPUT);
        pinMode(STEPPER_WINCH_PIN4, OUTPUT);
        
        // Convertir la puissance en vitesse du moteur pas à pas
        int stepperSpeed = map(power, WINCH_MIN_POWER, WINCH_MAX_POWER, 
                             STEPPER_MAX_SPEED * 0.1, STEPPER_MAX_SPEED); // Min 10% de vitesse max
        _stepperWinch.setSpeed(stepperSpeed);
        
        // En mode générateur, le stepper agit comme un frein/générateur
        // Plus la puissance est élevée, plus la résistance est forte
        _lastStepDirection = -1; // Direction négative pour la génération (résistance au déroulement)
        
        // Mettre à jour l'état actuel
        _currentState.winchPower = power;
        
        LOG_DEBUG("SERVO", "Puissance générateur réglée à %.1f%% (stepper speed: %d)", power, stepperSpeed);
        
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
        // Gestion du stepper motor pour le treuil/générateur
        if (_winchInitialized && _lastStepDirection != 0) {
            unsigned long currentTime = millis();
            // Ne pas exécuter trop fréquemment pour éviter de surcharger le processeur
            if (currentTime - _lastStepTime >= 20) { // 20ms = ~50 updates par seconde max
                // Nombre de pas à effectuer à chaque mise à jour
                const int stepsPerUpdate = 1; // Réduit pour assurer la fluidité
                
                // Déplacer le stepper selon la direction
                if (_lastStepDirection > 0) {
                    // Enroulement (sens horaire)
                    _stepperWinch.step(stepsPerUpdate);
                } else if (_lastStepDirection < 0) {
                    // Déroulement ou génération (sens anti-horaire)
                    _stepperWinch.step(-stepsPerUpdate);
                }
                
                _lastStepTime = currentTime;
            }
        }
    }

private:
    // Objets servomoteurs et moteur pas à pas
    Servo _servoDirection;    // Servo pour la direction gauche/droite
    Servo _servoTrim;         // Servo pour l'angle d'incidence (puissance)
    SimpleStepper _stepperWinch;    // Moteur pas à pas pour le treuil/générateur
    
    // États d'initialisation
    bool _directionInitialized;
    bool _trimInitialized;
    bool _winchInitialized;
    
    // État actuel des actionneurs
    ServoState _currentState;
    
    // Variables pour le contrôle du stepper
    int _lastStepDirection;   // Direction du dernier mouvement (-1=déroulement, 0=arrêt, 1=enroulement)
    unsigned long _lastStepTime; // Horodatage du dernier pas (utilisé pour timing)
    
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
