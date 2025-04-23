/*
 * Module de gestion des servomoteurs et du moteur pas à pas pour le projet Kite Pilote
 * 
 * Ce module contrôle les actionneurs du système:
 * - Servo Direction: Contrôle la direction gauche/droite du kite
 * - Servo Trim: Contrôle l'angle d'incidence (puissance) du kite
 * - Stepper Winch: Gestion de l'enroulement/déroulement et récupération d'énergie avec moteur pas à pas
 * 
 * Version: 2.0.0.5
 * Créé le: 17/04/2025
 * Mis à jour le: 23/04/2025
 */

#ifndef SERVO_MODULE_H
#define SERVO_MODULE_H

#include <Arduino.h>
#include <ESP32Servo.h>
#include "../utils/simple_stepper.h"
#include "../core/config.h"
#include "../core/data_types.h"
#include "../utils/logger.h"

/**
 * @class ServoModule
 * @brief Gère les servomoteurs et le moteur pas à pas du système
 * 
 * Cette classe contrôle tous les actionneurs mécaniques du système:
 * - Servo de direction: Oriente le kite latéralement
 * - Servo de trim: Ajuste l'angle d'incidence pour contrôler la puissance
 * - Moteur pas à pas (stepper): Gère le treuil pour l'enroulement/déroulement et la génération d'énergie
 */
class ServoModule {
public:
    /**
     * @brief Constructeur par défaut
     */
    ServoModule();
    
    /**
     * @brief Destructeur
     */
    ~ServoModule();
    
    /**
     * @brief Initialise tous les actionneurs
     * @return true si l'initialisation s'est déroulée correctement
     */
    bool begin();
    
    /**
     * @brief Règle l'angle du servo de direction
     * @param angle Angle de direction en degrés (entre DIRECTION_MIN_ANGLE et DIRECTION_MAX_ANGLE)
     * @return true si l'opération a réussi
     */
    bool setDirectionAngle(float angle);
    
    /**
     * @brief Règle l'angle du servo de trim (puissance)
     * @param angle Angle de trim en degrés (entre TRIM_MIN_ANGLE et TRIM_MAX_ANGLE)
     * @return true si l'opération a réussi
     */
    bool setTrimAngle(float angle);
    
    /**
     * @brief Règle le mode de fonctionnement du treuil
     * @param mode Mode d'opération (GENERATOR, REELING_IN, REELING_OUT, BRAKE, IDLE)
     * @return true si l'opération a réussi
     */
    bool setWinchMode(WinchMode mode);
    
    /**
     * @brief Règle la puissance/résistance du treuil (en mode générateur)
     * @param power Puissance en pourcentage (0-100%)
     * @return true si l'opération a réussi
     */
    bool setWinchPower(float power);
    
    /**
     * @brief Positionne tous les actionneurs en position neutre/sécuritaire
     */
    void neutralPosition();
    
    /**
     * @brief Effectue un arrêt d'urgence
     * Met le système dans un état sécuritaire immédiatement
     */
    void emergencyStop();
    
    /**
     * @brief Obtient l'état actuel de tous les actionneurs
     * @return Structure ServoState contenant les positions/états actuels
     */
    ServoState getState() const;
    
    /**
     * @brief Obtient l'angle actuel du servo de direction
     * @return Angle en degrés
     */
    float getDirectionAngle() const;
    
    /**
     * @brief Obtient l'angle actuel du servo de trim
     * @return Angle en degrés
     */
    float getTrimAngle() const;
    
    /**
     * @brief Obtient le mode actuel du treuil
     * @return Mode d'opération
     */
    WinchMode getWinchMode() const;
    
    /**
     * @brief Obtient la puissance actuelle du treuil
     * @return Puissance en pourcentage (0-100%)
     */
    float getWinchPower() const;
    
    /**
     * @brief Vérifie si tous les actionneurs sont correctement initialisés
     * @return true si tous les actionneurs sont initialisés
     */
    bool isInitialized() const;
    
    /**
     * @brief Met à jour l'état des actionneurs (pour les mouvements continus)
     * À appeler régulièrement dans la boucle principale
     */
    void update();

private:
    // Objets servomoteurs et moteur pas à pas
    Servo m_servoDirection;    // Servo pour la direction gauche/droite
    Servo m_servoTrim;         // Servo pour l'angle d'incidence (puissance)
    SimpleStepper m_stepperWinch;    // Moteur pas à pas pour le treuil/générateur
    
    // États d'initialisation
    bool m_directionInitialized;
    bool m_trimInitialized;
    bool m_winchInitialized;
    
    // État actuel des actionneurs
    ServoState m_currentState;
    
    // Variables pour le contrôle du stepper
    int m_lastStepDirection;   // Direction du dernier mouvement (-1=déroulement, 0=arrêt, 1=enroulement)
    unsigned long m_lastStepTime; // Horodatage du dernier pas (utilisé pour timing)
    
    /**
     * @brief Convertit l'angle d'entrée en angle pour le servo (0-180)
     * @param angle Angle d'entrée (dans la plage spécifiée)
     * @param minAngle Angle minimum de la plage d'entrée
     * @param maxAngle Angle maximum de la plage d'entrée
     * @return Angle converti pour le servo (0-180)
     */
    int mapAngleToDegrees(float angle, float minAngle, float maxAngle);
    
    /**
     * @brief Fonction utilitaire pour mapper une valeur float
     * @param x Valeur à mapper
     * @param in_min Minimum de la plage d'entrée
     * @param in_max Maximum de la plage d'entrée
     * @param out_min Minimum de la plage de sortie
     * @param out_max Maximum de la plage de sortie
     * @return Valeur mappée
     */
    static float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
};

#endif // SERVO_MODULE_H
