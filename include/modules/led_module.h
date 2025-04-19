/*
 * Module de gestion des LEDs pour le projet Kite Pilote
 * 
 * Contrôle les LEDs avec différents motifs de clignotement pour indiquer l'état du système.
 * Module simple permettant plusieurs patterns (clignotement lent/rapide, SOS, etc.)
 * 
 * Créé le: 17/04/2025
 */

#ifndef LED_MODULE_H
#define LED_MODULE_H

#include <Arduino.h>
#include "../core/config.h"
#include "../utils/logger.h"

class LedModule {
public:
    // Constructeur
    LedModule(uint8_t pin, const char* name = "LED") {
        _pin = pin;
        _name = name;
        _pattern = LED_PATTERN_OFF;
        _lastUpdateTime = 0;
        _currentState = false;
        _patternStep = 0;
    }
    
    // Initialisation du module
    bool begin() {
        pinMode(_pin, OUTPUT);
        digitalWrite(_pin, LOW);
        LOG_INFO(_name, "Initialisé sur la broche %d", _pin);
        return true;
    }
    
    // Définir un motif de clignotement
    void setPattern(LedPattern pattern) {
        if (_pattern != pattern) {
            _pattern = pattern;
            _lastUpdateTime = 0; // Forcer une mise à jour immédiate
            _patternStep = 0;    // Réinitialiser la séquence
            LOG_DEBUG(_name, "Motif changé: %d", pattern);
        }
    }
    
    // Allumer la LED
    void turnOn() {
        setPattern(LED_PATTERN_ON);
    }
    
    // Éteindre la LED
    void turnOff() {
        setPattern(LED_PATTERN_OFF);
    }
    
    // Obtenir le motif actuel
    LedPattern getPattern() const {
        return _pattern;
    }
    
    // Mise à jour (à appeler dans la boucle principale)
    void update() {
        unsigned long currentTime = millis();
        
        // Gestion selon le motif choisi
        switch (_pattern) {
            case LED_PATTERN_OFF:
                // LED éteinte en permanence
                if (_currentState) {
                    _currentState = false;
                    digitalWrite(_pin, LOW);
                }
                break;
                
            case LED_PATTERN_ON:
                // LED allumée en permanence
                if (!_currentState) {
                    _currentState = true;
                    digitalWrite(_pin, HIGH);
                }
                break;
                
            case LED_PATTERN_SLOW_BLINK:
                // Clignotement lent (1Hz)
                if (currentTime - _lastUpdateTime >= 500) {
                    _currentState = !_currentState;
                    digitalWrite(_pin, _currentState ? HIGH : LOW);
                    _lastUpdateTime = currentTime;
                }
                break;
                
            case LED_PATTERN_FAST_BLINK:
                // Clignotement rapide (5Hz)
                if (currentTime - _lastUpdateTime >= 100) {
                    _currentState = !_currentState;
                    digitalWrite(_pin, _currentState ? HIGH : LOW);
                    _lastUpdateTime = currentTime;
                }
                break;
                
            case LED_PATTERN_ERROR:
                // Motif d'erreur (SOS)
                updateSOSPattern(currentTime);
                break;
                
            case LED_PATTERN_WIFI_CONNECTING:
                // Motif de connexion WiFi (double clignotement)
                updateWiFiConnectingPattern(currentTime);
                break;
                
            case LED_PATTERN_WIFI_CONNECTED:
                // Motif WiFi connecté (triple clignotement court puis pause)
                updateWiFiConnectedPattern(currentTime);
                break;
        }
    }

private:
    uint8_t _pin;             // Broche de la LED
    const char* _name;        // Nom pour les logs (ex: "LED_ROUGE")
    LedPattern _pattern;      // Motif actuel
    bool _currentState;       // État actuel (allumé/éteint)
    unsigned long _lastUpdateTime; // Dernière mise à jour
    uint8_t _patternStep;     // Étape actuelle dans les motifs complexes
    
    // Motif SOS en morse (... --- ...)
    void updateSOSPattern(unsigned long currentTime) {
        static const uint8_t sosPattern[] = {
            1, 0, 1, 0, 1, 0,   // ...
            0, 0,               // pause
            1, 1, 0, 1, 1, 0, 1, 1, 0,  // ---
            0, 0,               // pause
            1, 0, 1, 0, 1, 0,   // ...
            0, 0, 0, 0          // longue pause
        };
        static const uint8_t patternLength = sizeof(sosPattern) / sizeof(sosPattern[0]);
        
        // 200ms pour chaque étape
        if (currentTime - _lastUpdateTime >= 200) {
            _patternStep = (_patternStep + 1) % patternLength;
            _currentState = sosPattern[_patternStep] == 1;
            digitalWrite(_pin, _currentState ? HIGH : LOW);
            _lastUpdateTime = currentTime;
        }
    }
    
    // Motif de connexion WiFi (double clignotement)
    void updateWiFiConnectingPattern(unsigned long currentTime) {
        static const uint8_t wifiPattern[] = {
            1, 0, 1, 0, 0, 0, 0, 0
        };
        static const uint8_t patternLength = sizeof(wifiPattern) / sizeof(wifiPattern[0]);
        
        // 150ms pour chaque étape
        if (currentTime - _lastUpdateTime >= 150) {
            _patternStep = (_patternStep + 1) % patternLength;
            _currentState = wifiPattern[_patternStep] == 1;
            digitalWrite(_pin, _currentState ? HIGH : LOW);
            _lastUpdateTime = currentTime;
        }
    }
    
    // Motif WiFi connecté (triple clignotement puis pause)
    void updateWiFiConnectedPattern(unsigned long currentTime) {
        static const uint8_t wifiConnectedPattern[] = {
            1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0
        };
        static const uint8_t patternLength = sizeof(wifiConnectedPattern) / sizeof(wifiConnectedPattern[0]);
        
        // 100ms pour chaque étape
        if (currentTime - _lastUpdateTime >= 100) {
            _patternStep = (_patternStep + 1) % patternLength;
            _currentState = wifiConnectedPattern[_patternStep] == 1;
            digitalWrite(_pin, _currentState ? HIGH : LOW);
            _lastUpdateTime = currentTime;
        }
    }
};

#endif // LED_MODULE_H
