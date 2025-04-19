/*
 * Module de simulation pour le projet Kite Pilote
 * 
 * Ce module permet de simuler les capteurs et l'environnement pour tester 
 * le système sans matériel réel, notamment dans l'environnement Wokwi.
 * 
 * Créé le: 17/04/2025
 */

#ifndef SIMULATION_MODULE_H
#define SIMULATION_MODULE_H

#include <Arduino.h>
#include "../core/config.h"
#include "../core/data_types.h"
#include "../utils/logger.h"
#include "../modules/sensor_module.h"
#include "../modules/autopilot_module.h"
#include "../modules/servo_module.h"

class SimulationModule {
public:
    // Constructeur
    SimulationModule() : _initialized(false), _lastButtonDebounceTime(0) {
        // L'initialisation complète se fait dans begin()
    }
    
    // Initialisation
    bool begin() {
        LOG_INFO("SIMULATION", "Initialisation du mode simulation");
        
        // Vérifier que le mode simulation est activé
        #ifndef SIMULATION_MODE
            LOG_WARNING("SIMULATION", "Mode simulation non activé dans config.h");
            return false;
        #endif
        
        // Configuration des pins pour les potentiomètres de simulation
        pinMode(SIM_ROLL_PIN, INPUT);
        pinMode(SIM_PITCH_PIN, INPUT);
        pinMode(SIM_YAW_PIN, INPUT);
        pinMode(SIM_TENSION_PIN, INPUT);
        pinMode(SIM_WIND_SPEED_PIN, INPUT);
        pinMode(SIM_WIND_DIRECTION_PIN, INPUT);
        
        // Configuration des pins pour les boutons
        pinMode(SIM_MODE_BUTTON_PIN, INPUT_PULLUP);
        pinMode(SIM_EMERGENCY_BUTTON_PIN, INPUT_PULLUP);
        
        // Initialisation des états des boutons
        _lastModeButtonState = digitalRead(SIM_MODE_BUTTON_PIN);
        _lastEmergencyButtonState = digitalRead(SIM_EMERGENCY_BUTTON_PIN);
        
        _initialized = true;
        LOG_INFO("SIMULATION", "Mode simulation initialisé avec succès");
        
        return true;
    }
    
    // Mise à jour des capteurs avec les données de simulation
    void updateSensors(SensorModule& sensorModule) {
        if (!_initialized) return;
        
        // Lecture des potentiomètres et conversion en valeurs physiqueS
        int rollRaw = analogRead(SIM_ROLL_PIN);
        int pitchRaw = analogRead(SIM_PITCH_PIN);
        int yawRaw = analogRead(SIM_YAW_PIN);
        int tensionRaw = analogRead(SIM_TENSION_PIN);
        int windSpeedRaw = analogRead(SIM_WIND_SPEED_PIN);
        int windDirectionRaw = analogRead(SIM_WIND_DIRECTION_PIN);
        
        // Conversion en valeurs exploitables
        float roll = map(rollRaw, 0, 4095, -45, 45);
        float pitch = map(pitchRaw, 0, 4095, -30, 60);
        float yaw = map(yawRaw, 0, 4095, 0, 359);
        float tension = map(tensionRaw, 0, 4095, 0, 100);
        float windSpeed = map(windSpeedRaw, 0, 4095, 0, 20);
        float windDirection = map(windDirectionRaw, 0, 4095, 0, 359);
        
        // Mise à jour des données du module capteur
        sensorModule.setIMUData(roll, pitch, yaw);
        sensorModule.setLineTension(tension);
        sensorModule.setLineLength(50.0f); // Longueur fixe pour la simulation
        sensorModule.setWindData(windSpeed, windDirection);
        
        LOG_DEBUG("SIMULATION", "Capteurs mis à jour - Roll:%.1f Pitch:%.1f Tension:%.1f Wind:%.1f@%.1f°", 
                 roll, pitch, tension, windSpeed, windDirection);
    }
    
    // Gestion des boutons de simulation
    void handleButtons(AutopilotModule& autopilotModule, ServoModule& servoModule) {
        if (!_initialized) return;
        
        // Lecture de l'état actuel des boutons
        bool modeButtonState = digitalRead(SIM_MODE_BUTTON_PIN);
        bool emergencyButtonState = digitalRead(SIM_EMERGENCY_BUTTON_PIN);
        
        // Gestion du bouton de mode avec debounce
        if (modeButtonState != _lastModeButtonState) {
            _lastButtonDebounceTime = millis();
        }
        
        if ((millis() - _lastButtonDebounceTime) > DEBOUNCE_DELAY) {
            // Si l'état du bouton a changé et est stable depuis plus que le délai de debounce
            if (modeButtonState == LOW && _lastModeButtonState == HIGH) {  // Bouton pressé (LOW car INPUT_PULLUP)
                // Changer le mode de l'autopilote
                AutopilotMode currentMode = autopilotModule.getMode();
                AutopilotMode nextMode;
                
                // Cycle à travers les modes
                switch (currentMode) {
                    case AUTOPILOT_OFF:
                        nextMode = AUTOPILOT_STANDBY;
                        break;
                    case AUTOPILOT_STANDBY:
                        nextMode = AUTOPILOT_LAUNCH;
                        break;
                    case AUTOPILOT_LAUNCH:
                        nextMode = AUTOPILOT_EIGHT_PATTERN;
                        break;
                    case AUTOPILOT_EIGHT_PATTERN:
                        nextMode = AUTOPILOT_CIRCULAR;
                        break;
                    case AUTOPILOT_CIRCULAR:
                        nextMode = AUTOPILOT_POWER_GENERATION;
                        break;
                    case AUTOPILOT_POWER_GENERATION:
                        nextMode = AUTOPILOT_LAND;
                        break;
                    case AUTOPILOT_LAND:
                        nextMode = AUTOPILOT_OFF;
                        break;
                    default:
                        nextMode = AUTOPILOT_OFF;
                        break;
                }
                
                // Définir le nouveau mode
                autopilotModule.setMode(nextMode);
                
                // Afficher le changement de mode
                LOG_INFO("SIMULATION", "Mode autopilote changé en: %s", 
                         autopilotModule.getStatusMessage());
            }
        }
        
        // Gestion du bouton d'urgence avec debounce
        if (emergencyButtonState != _lastEmergencyButtonState) {
            _lastButtonDebounceTime = millis();
        }
        
        if ((millis() - _lastButtonDebounceTime) > DEBOUNCE_DELAY) {
            // Si l'état du bouton a changé et est stable depuis plus que le délai de debounce
            if (emergencyButtonState == LOW && _lastEmergencyButtonState == HIGH) {  // Bouton pressé (LOW car INPUT_PULLUP)
                // Déclencher l'arrêt d'urgence
                LOG_WARNING("SIMULATION", "ARRÊT D'URGENCE DÉCLENCHÉ");
                autopilotModule.emergencyStop();
                servoModule.emergencyStop();
            }
        }
        
        // Mettre à jour les états précédents des boutons
        _lastModeButtonState = modeButtonState;
        _lastEmergencyButtonState = emergencyButtonState;
    }
    
private:
    bool _initialized;                        // État d'initialisation
    bool _lastModeButtonState;                // Dernier état du bouton mode
    bool _lastEmergencyButtonState;           // Dernier état du bouton d'urgence
    unsigned long _lastButtonDebounceTime;    // Dernière mise à jour de l'état des boutons
    
    // Constantes
    static const unsigned long DEBOUNCE_DELAY = 50;  // Délai de debounce en ms
    
    // Fonction utilitaire: map pour les floats
    float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
        return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }
};

#endif // SIMULATION_MODULE_H
