/*
 * Implémentation du module de gestion des servomoteurs pour le projet Kite Pilote
 * 
 * Gère les actionneurs mécaniques du système:
 * - Servomoteurs de direction et de trim
 * - Moteur pas à pas pour le treuil/générateur
 * 
 * Version: 2.0.0.5
 * Créé le: 23/04/2025
 */

#include "modules/servo_module.h"

// Constructeur
ServoModule::ServoModule() : 
    m_directionInitialized(false),
    m_trimInitialized(false),
    m_winchInitialized(false),
    m_currentState(),  // Initialise avec les valeurs par défaut (voir ServoState)
    m_stepperWinch(STEPPER_STEPS_PER_REV, STEPPER_WINCH_PIN1, STEPPER_WINCH_PIN2, 
                  STEPPER_WINCH_PIN3, STEPPER_WINCH_PIN4),
    m_lastStepDirection(0),
    m_lastStepTime(0)
{
    // Reste de l'initialisation dans begin()
}

// Destructeur
ServoModule::~ServoModule() {
    // Relâcher les ressources si nécessaire
    if (m_directionInitialized) {
        m_servoDirection.detach();
    }
    
    if (m_trimInitialized) {
        m_servoTrim.detach();
    }
    
    // Pour le moteur pas à pas, désactiver les sorties
    if (m_winchInitialized) {
        digitalWrite(STEPPER_WINCH_PIN1, LOW);
        digitalWrite(STEPPER_WINCH_PIN2, LOW);
        digitalWrite(STEPPER_WINCH_PIN3, LOW);
        digitalWrite(STEPPER_WINCH_PIN4, LOW);
    }
}

// Initialisation des servomoteurs
bool ServoModule::begin() {
    bool success = true;
    
    LOG_INFO("SERVO", "Initialisation des servomoteurs...");
    
    // Initialisation du servo de direction
    m_servoDirection.setPeriodHertz(50);  // Fréquence standard pour la plupart des servos
    if (m_servoDirection.attach(SERVO_DIRECTION_PIN, SERVO_MIN_PULSE, SERVO_MAX_PULSE)) {
        m_directionInitialized = true;
        LOG_INFO("SERVO", "Servo Direction initialisé sur la broche %d", SERVO_DIRECTION_PIN);
    } else {
        LOG_ERROR("SERVO", "Échec d'initialisation du servo Direction");
        success = false;
    }
    
    // Initialisation du servo de trim
    m_servoTrim.setPeriodHertz(50);
    if (m_servoTrim.attach(SERVO_TRIM_PIN, SERVO_MIN_PULSE, SERVO_MAX_PULSE)) {
        m_trimInitialized = true;
        LOG_INFO("SERVO", "Servo Trim initialisé sur la broche %d", SERVO_TRIM_PIN);
    } else {
        LOG_ERROR("SERVO", "Échec d'initialisation du servo Trim");
        success = false;
    }
    
    // Initialisation du moteur pas à pas pour le treuil/générateur
    try {
        // Configuration de la vitesse du moteur pas à pas
        m_stepperWinch.setSpeed(STEPPER_MAX_SPEED / 2); // Vitesse initiale à mi-régime
        m_winchInitialized = true;
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
bool ServoModule::setDirectionAngle(float angle) {
    if (!m_directionInitialized) {
        LOG_ERROR("SERVO", "Servo Direction non initialisé");
        return false;
    }
    
    // Limiter l'angle aux valeurs min/max
    angle = constrain(angle, DIRECTION_MIN_ANGLE, DIRECTION_MAX_ANGLE);
    
    // Convertir l'angle en impulsion et positionner le servo
    m_servoDirection.write(mapAngleToDegrees(angle, DIRECTION_MIN_ANGLE, DIRECTION_MAX_ANGLE));
    
    // Mettre à jour l'état actuel
    m_currentState.directionAngle = angle;
    
    LOG_DEBUG("SERVO", "Direction réglée à %.1f degrés", angle);
    
    return true;
}

// Contrôle du servo de trim (angle entre -30° et +30°)
bool ServoModule::setTrimAngle(float angle) {
    if (!m_trimInitialized) {
        LOG_ERROR("SERVO", "Servo Trim non initialisé");
        return false;
    }
    
    // Limiter l'angle aux valeurs min/max
    angle = constrain(angle, TRIM_MIN_ANGLE, TRIM_MAX_ANGLE);
    
    // Convertir l'angle en impulsion et positionner le servo
    m_servoTrim.write(mapAngleToDegrees(angle, TRIM_MIN_ANGLE, TRIM_MAX_ANGLE));
    
    // Mettre à jour l'état actuel
    m_currentState.trimAngle = angle;
    
    LOG_DEBUG("SERVO", "Trim réglé à %.1f degrés", angle);
    
    return true;
}

// Contrôle du treuil/générateur avec moteur pas à pas (mode et puissance)
bool ServoModule::setWinchMode(WinchMode mode) {
    if (!m_winchInitialized) {
        LOG_ERROR("SERVO", "Stepper Winch non initialisé");
        return false;
    }
    
    m_currentState.winchMode = mode;
    m_lastStepDirection = 0; // Réinitialiser la direction à l'arrêt par défaut
    
    // Appliquer les réglages selon le mode
    switch (mode) {
        case WINCH_MODE_GENERATOR:
            // En mode générateur, utiliser la puissance actuelle
            return setWinchPower(m_currentState.winchPower);
            
        case WINCH_MODE_REELING_IN:
            // Enroulement: vitesse maximale dans le sens horaire
            m_stepperWinch.setSpeed(STEPPER_MAX_SPEED);
            m_lastStepDirection = 1;  // Direction positive (enroulement)
            LOG_INFO("SERVO", "Treuil: Mode enroulement (stepper)");
            break;
            
        case WINCH_MODE_REELING_OUT:
            // Déroulement: vitesse maximale dans le sens anti-horaire
            m_stepperWinch.setSpeed(STEPPER_MAX_SPEED);
            m_lastStepDirection = -1;  // Direction négative (déroulement)
            LOG_INFO("SERVO", "Treuil: Mode déroulement (stepper)");
            break;
            
        case WINCH_MODE_BRAKE:
            // Frein: couper l'alimentation des bobines pour maintenir la position
            // La résistance magnétique naturelle du stepper maintient la position
            m_lastStepDirection = 0;  // Arrêt (pas de mouvement)
            LOG_INFO("SERVO", "Treuil: Mode frein (stepper)");
            break;
            
        case WINCH_MODE_IDLE:
            // Repos: aucune résistance (moteur désactivé)
            m_lastStepDirection = 0;  // Arrêt (pas de mouvement)
            
            // Désactiver les bobines pour éviter la consommation d'énergie
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
bool ServoModule::setWinchPower(float power) {
    if (!m_winchInitialized) {
        LOG_ERROR("SERVO", "Stepper Winch non initialisé");
        return false;
    }
    
    // Vérifier que nous sommes en mode générateur
    if (m_currentState.winchMode != WINCH_MODE_GENERATOR) {
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
    m_stepperWinch.setSpeed(stepperSpeed);
    
    // En mode générateur, le stepper agit comme un frein/générateur
    // Plus la puissance est élevée, plus la résistance est forte
    m_lastStepDirection = -1; // Direction négative pour la génération (résistance au déroulement)
    
    // Mettre à jour l'état actuel
    m_currentState.winchPower = power;
    
    LOG_DEBUG("SERVO", "Puissance générateur réglée à %.1f%% (stepper speed: %d)", power, stepperSpeed);
    
    return true;
}

// Mettre tous les servos en position neutre
void ServoModule::neutralPosition() {
    LOG_INFO("SERVO", "Positionnement des servos en position neutre");
    
    // Direction au centre (0°)
    setDirectionAngle(DIRECTION_CENTER);
    
    // Trim au centre (0°)
    setTrimAngle(TRIM_CENTER);
    
    // Treuil en mode repos
    setWinchMode(WINCH_MODE_IDLE);
}

// Arrêt d'urgence
void ServoModule::emergencyStop() {
    LOG_WARNING("SERVO", "ARRÊT D'URGENCE des servomoteurs");
    
    // Direction au centre
    if (m_directionInitialized) {
        setDirectionAngle(DIRECTION_CENTER);
    }
    
    // Trim au minimum de puissance
    if (m_trimInitialized) {
        setTrimAngle(TRIM_MIN_ANGLE);
    }
    
    // Treuil en mode frein
    if (m_winchInitialized) {
        setWinchMode(WINCH_MODE_BRAKE);
    }
}

// Obtenir l'état actuel des servomoteurs
ServoState ServoModule::getState() const {
    return m_currentState;
}

// Getters individuels pour chaque paramètre
float ServoModule::getDirectionAngle() const {
    return m_currentState.directionAngle;
}

float ServoModule::getTrimAngle() const {
    return m_currentState.trimAngle;
}

WinchMode ServoModule::getWinchMode() const {
    return m_currentState.winchMode;
}

float ServoModule::getWinchPower() const {
    return m_currentState.winchPower;
}

// Vérifier si les servos sont correctement initialisés
bool ServoModule::isInitialized() const {
    return m_directionInitialized && m_trimInitialized && m_winchInitialized;
}

// Mise à jour périodique (pour les actions qui nécessitent un suivi temps réel)
void ServoModule::update() {
    // Gestion du stepper motor pour le treuil/générateur
    if (m_winchInitialized && m_lastStepDirection != 0) {
        unsigned long currentTime = millis();
        // Ne pas exécuter trop fréquemment pour éviter de surcharger le processeur
        if (currentTime - m_lastStepTime >= 20) { // 20ms = ~50 updates par seconde max
            // Nombre de pas à effectuer à chaque mise à jour
            const int stepsPerUpdate = 1; // Réduit pour assurer la fluidité
            
            // Déplacer le stepper selon la direction
            if (m_lastStepDirection > 0) {
                // Enroulement (sens horaire)
                m_stepperWinch.step(stepsPerUpdate);
            } else if (m_lastStepDirection < 0) {
                // Déroulement ou génération (sens anti-horaire)
                m_stepperWinch.step(-stepsPerUpdate);
            }
            
            m_lastStepTime = currentTime;
        }
    }
}

// Méthodes privées

// Convertir l'angle du kite en angle du servo (0-180°)
int ServoModule::mapAngleToDegrees(float angle, float minAngle, float maxAngle) {
    return map(angle * 10, minAngle * 10, maxAngle * 10, 0, 180) / 10;
}

// Mapper une valeur float (méthode utilitaire)
float ServoModule::mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
