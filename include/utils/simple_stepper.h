/*
 * Implémentation simplifiée d'un contrôleur de moteur pas à pas pour le projet Kite Pilote
 * 
 * Cette classe fournit les fonctionnalités de base pour contrôler un moteur pas à pas
 * à 4 fils (typiquement en configuration unipolaire ou bipolaire).
 * 
 * Créé le: 22/04/2025
 */

#ifndef SIMPLE_STEPPER_H
#define SIMPLE_STEPPER_H

#include <Arduino.h>

class SimpleStepper {
public:
    // Constructeur avec 4 broches pour un moteur pas à pas standard
    SimpleStepper(int stepsPerRevolution, uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4) :
        _stepsPerRevolution(stepsPerRevolution),
        _currentStep(0),
        _lastStepTime(0),
        _stepInterval(60L * 1000L / stepsPerRevolution / 60), // Vitesse par défaut: 60 RPM
        _pin1(pin1),
        _pin2(pin2),
        _pin3(pin3),
        _pin4(pin4)
    {
        // Configuration des broches de sortie
        pinMode(_pin1, OUTPUT);
        pinMode(_pin2, OUTPUT);
        pinMode(_pin3, OUTPUT);
        pinMode(_pin4, OUTPUT);
        
        // État initial des broches
        digitalWrite(_pin1, LOW);
        digitalWrite(_pin2, LOW);
        digitalWrite(_pin3, LOW);
        digitalWrite(_pin4, LOW);
    }
    
    // Définir la vitesse de rotation (RPM)
    void setSpeed(long rpm) {
        if (rpm <= 0) {
            rpm = 1; // Éviter la division par zéro
        }
        
        // Calcul de l'intervalle entre les pas en microsecondes
        _stepInterval = 60L * 1000L * 1000L / _stepsPerRevolution / rpm;
    }
    
    // Exécuter un nombre de pas (positif = sens horaire, négatif = sens anti-horaire)
    void step(int numberOfSteps) {
        int stepsLeft = abs(numberOfSteps);
        int direction = (numberOfSteps > 0) ? 1 : -1;
        
        // Exécuter les pas un par un
        while (stepsLeft > 0) {
            unsigned long now = micros();
            
            // Vérifier si c'est le moment d'effectuer le prochain pas
            if (now - _lastStepTime >= _stepInterval) {
                _lastStepTime = now;
                
                // Déplacer d'un pas dans la direction demandée
                _currentStep += direction;
                
                // Rester dans la plage de 0-3 (séquence à 4 pas)
                // Utilise une arithmétique modulo mais reste compatible avec les valeurs négatives
                _currentStep = (_currentStep + 4) % 4;
                
                // Activer les bobines appropriées pour la position actuelle
                stepMotor(_currentStep);
                
                stepsLeft--;
            }
        }
    }
    
    // Désactiver toutes les bobines (mode repos/idle)
    void releaseMotor() {
        digitalWrite(_pin1, LOW);
        digitalWrite(_pin2, LOW);
        digitalWrite(_pin3, LOW);
        digitalWrite(_pin4, LOW);
    }
    
private:
    // Paramètres du moteur
    int _stepsPerRevolution;
    int _currentStep;
    unsigned long _lastStepTime;
    unsigned long _stepInterval; // En microsecondes
    
    // Broches de contrôle
    uint8_t _pin1;
    uint8_t _pin2;
    uint8_t _pin3;
    uint8_t _pin4;
    
    // Activer la séquence appropriée pour la position actuelle (séquence à 4 pas)
    void stepMotor(int thisStep) {
        switch (thisStep) {
            case 0:  // 1010
                digitalWrite(_pin1, HIGH);
                digitalWrite(_pin2, LOW);
                digitalWrite(_pin3, HIGH);
                digitalWrite(_pin4, LOW);
                break;
            case 1:  // 0110
                digitalWrite(_pin1, LOW);
                digitalWrite(_pin2, HIGH);
                digitalWrite(_pin3, HIGH);
                digitalWrite(_pin4, LOW);
                break;
            case 2:  // 0101
                digitalWrite(_pin1, LOW);
                digitalWrite(_pin2, HIGH);
                digitalWrite(_pin3, LOW);
                digitalWrite(_pin4, HIGH);
                break;
            case 3:  // 1001
                digitalWrite(_pin1, HIGH);
                digitalWrite(_pin2, LOW);
                digitalWrite(_pin3, LOW);
                digitalWrite(_pin4, HIGH);
                break;
        }
    }
};

#endif // SIMPLE_STEPPER_H
