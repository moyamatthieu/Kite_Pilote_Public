/*
 * Implémentation du module d'autopilote pour le projet Kite Pilote
 * 
 * Ce module implémente les algorithmes de contrôle automatique du kite pour
 * les différents modes de vol: vol en huit, vol circulaire, séquences de 
 * décollage et d'atterrissage, et optimisation de la génération d'énergie.
 * 
 * Version: 2.0.0.5
 * Créé le: 23/04/2025
 */

#include "modules/autopilot_module.h"

// Constructeur
AutopilotModule::AutopilotModule() {
    resetState();
}

// Destructeur
AutopilotModule::~AutopilotModule() {
    // Aucune ressource spécifique à libérer
}

// Initialisation du module
bool AutopilotModule::begin() {
    LOG_INFO("AUTOPILOT", "Initialisation de l'autopilote");
    _initialized = true;
    return true;
}

// Définir le mode d'autopilote
bool AutopilotModule::setMode(AutopilotMode mode) {
    // Vérification de l'initialisation
    if (!_initialized) {
        LOG_ERROR("AUTOPILOT", "Impossible de changer de mode - non initialisé");
        return false;
    }
    
    // Si le mode est déjà actif, ne rien faire
    if (_status.mode == mode) {
        return true;
    }
    
    // Transition vers le nouveau mode
    AutopilotMode oldMode = _status.mode;
    _status.mode = mode;
    
    // Mise à jour du message textuel et réinitialisation des données spécifiques au mode
    switch (mode) {
        case AUTOPILOT_OFF:
            _status.setStatusMessage("Désactivé");
            LOG_INFO("AUTOPILOT", "Mode: Désactivé");
            break;
            
        case AUTOPILOT_STANDBY:
            _status.setStatusMessage("Attente");
            LOG_INFO("AUTOPILOT", "Mode: Attente");
            break;
            
        case AUTOPILOT_LAUNCH:
            _status.setStatusMessage("Décollage");
            _status.completionPercent = 0.0f;
            _status.startNewSequence();
            LOG_INFO("AUTOPILOT", "Mode: Décollage");
            break;
            
        case AUTOPILOT_LAND:
            _status.setStatusMessage("Atterrissage");
            _status.completionPercent = 0.0f;
            _status.startNewSequence();
            LOG_INFO("AUTOPILOT", "Mode: Atterrissage");
            break;
            
        case AUTOPILOT_EIGHT_PATTERN:
            _status.setStatusMessage("Vol en 8");
            _patternStep = 0;
            LOG_INFO("AUTOPILOT", "Mode: Vol en 8");
            break;
            
        case AUTOPILOT_CIRCULAR:
            _status.setStatusMessage("Vol circulaire");
            _patternStep = 0;
            LOG_INFO("AUTOPILOT", "Mode: Vol circulaire");
            break;
            
        case AUTOPILOT_POWER_GENERATION:
            _status.setStatusMessage("Génération");
            _status.powerGenerated = 0.0f;
            LOG_INFO("AUTOPILOT", "Mode: Génération d'énergie");
            break;
    }
    
    _lastModeChangeTime = millis();
    return true;
}

// Arrêt d'urgence
void AutopilotModule::emergencyStop() {
    LOG_WARNING("AUTOPILOT", "ARRÊT D'URGENCE activé");
    setMode(AUTOPILOT_OFF);
}

// Mise à jour de l'autopilote
void AutopilotModule::update(const IMUData& imuData, const LineData& lineData, const WindData& windData) {
    // Ignorer si non initialisé ou désactivé
    if (!_initialized || _status.mode == AUTOPILOT_OFF) {
        return;
    }
    
    // Mettre à jour les données utilisées pour l'autopilote
    _lastImuData = imuData;
    _lastLineData = lineData;
    _lastWindData = windData;
    
    // Mettre à jour le contrôle selon le mode actuel
    switch (_status.mode) {
        case AUTOPILOT_STANDBY:
            updateStandbyMode();
            break;
            
        case AUTOPILOT_LAUNCH:
            updateLaunchMode();
            break;
            
        case AUTOPILOT_LAND:
            updateLandMode();
            break;
            
        case AUTOPILOT_EIGHT_PATTERN:
            updateEightPatternMode();
            break;
            
        case AUTOPILOT_CIRCULAR:
            updateCircularMode();
            break;
            
        case AUTOPILOT_POWER_GENERATION:
            updatePowerGenerationMode();
            break;
            
        default:
            // Rien à faire pour AUTOPILOT_OFF
            break;
    }
    
    // Incrémenter le compteur de mise à jour
    _updateCount++;
}

// Obtenir l'état actuel de l'autopilote
AutopilotStatus AutopilotModule::getStatus() const {
    return _status;
}

// Obtenir le mode actuel
AutopilotMode AutopilotModule::getMode() const {
    return _status.mode;
}

// Obtenir le message de statut
const char* AutopilotModule::getStatusMessage() const {
    return _status.statusMessage;
}

// Obtenir le pourcentage de complétion
float AutopilotModule::getCompletionPercentage() const {
    return _status.completionPercent;
}

// Obtenir la puissance générée
float AutopilotModule::getPowerGenerated() const {
    return _status.powerGenerated;
}

// Obtenir l'énergie totale
float AutopilotModule::getTotalEnergy() const {
    return _status.totalEnergy;
}

// Obtenir l'angle de direction cible
float AutopilotModule::getTargetDirectionAngle() const {
    return _targetDirectionAngle;
}

// Obtenir l'angle de trim cible
float AutopilotModule::getTargetTrimAngle() const {
    return _targetTrimAngle;
}

// Obtenir le mode cible du treuil
WinchMode AutopilotModule::getTargetWinchMode() const {
    return _targetWinchMode;
}

// Obtenir la puissance cible du treuil
float AutopilotModule::getTargetWinchPower() const {
    return _targetWinchPower;
}

// Réinitialisation de l'état interne
void AutopilotModule::resetState() {
    _status = AutopilotStatus();
    _targetDirectionAngle = 0.0f;
    _targetTrimAngle = 0.0f;
    _targetWinchMode = WINCH_MODE_IDLE;
    _targetWinchPower = 0.0f;
    _updateCount = 0;
    _patternStep = 0;
    _lastModeChangeTime = millis();
    _initialized = false;
}

// Implémentations des différents modes

void AutopilotModule::updateStandbyMode() {
    // En mode attente, maintenir le kite stable
    _targetDirectionAngle = 0.0f;
    _targetTrimAngle = 0.0f;
    _targetWinchMode = WINCH_MODE_IDLE;
    _targetWinchPower = 0.0f;
}

void AutopilotModule::updateLaunchMode() {
    // Séquence de décollage - progression linéaire
    unsigned long elapsedTime = millis() - _lastModeChangeTime;
    float progress = min(1.0f, (float)elapsedTime / LAUNCH_SEQUENCE_DURATION);
    _status.completionPercent = progress * 100.0f;
    
    // Une séquence simple pour illustrer, à adapter selon les besoins réels
    if (progress < 0.3f) {
        // Phase 1: Préparation (30% du temps)
        _targetDirectionAngle = 0.0f;
        _targetTrimAngle = -10.0f;
        _targetWinchMode = WINCH_MODE_IDLE;
    } else if (progress < 0.7f) {
        // Phase 2: Décollage (40% du temps)
        _targetDirectionAngle = 0.0f;
        _targetTrimAngle = mapFloat(progress, 0.3f, 0.7f, -10.0f, 20.0f);
        _targetWinchMode = WINCH_MODE_REELING_OUT;
    } else {
        // Phase 3: Stabilisation (30% du temps)
        _targetDirectionAngle = 0.0f;
        _targetTrimAngle = 10.0f;
        _targetWinchMode = WINCH_MODE_BRAKE;
    }
    
    // Si la séquence est terminée, passer en mode vol en huit
    if (progress >= 1.0f) {
        LOG_INFO("AUTOPILOT", "Séquence de décollage terminée");
        setMode(AUTOPILOT_EIGHT_PATTERN);
    }
}

void AutopilotModule::updateLandMode() {
    // Séquence d'atterrissage - progression linéaire
    unsigned long elapsedTime = millis() - _lastModeChangeTime;
    float progress = min(1.0f, (float)elapsedTime / LAND_SEQUENCE_DURATION);
    _status.completionPercent = progress * 100.0f;
    
    // Séquence d'atterrissage simplifiée
    if (progress < 0.3f) {
        // Phase 1: Réduction de puissance (30% du temps)
        _targetDirectionAngle = 0.0f;
        _targetTrimAngle = mapFloat(progress, 0.0f, 0.3f, 10.0f, -15.0f);
        _targetWinchMode = WINCH_MODE_BRAKE;
    } else if (progress < 0.8f) {
        // Phase 2: Rembobinage contrôlé (50% du temps)
        _targetDirectionAngle = 0.0f;
        _targetTrimAngle = -15.0f;
        _targetWinchMode = WINCH_MODE_REELING_IN;
    } else {
        // Phase 3: Arrêt final (20% du temps)
        _targetDirectionAngle = 0.0f;
        _targetTrimAngle = -20.0f;
        _targetWinchMode = WINCH_MODE_BRAKE;
    }
    
    // Si la séquence est terminée, passer en mode désactivé
    if (progress >= 1.0f) {
        LOG_INFO("AUTOPILOT", "Séquence d'atterrissage terminée");
        setMode(AUTOPILOT_OFF);
    }
}

void AutopilotModule::updateEightPatternMode() {
    // Génération d'un motif en forme de 8 pour maximiser la génération d'énergie
    // Utilise une fonction sinusoïdale avec variation de phase
    
    // Déterminer l'angle dans le cycle (0-360 degrés)
    float cyclePosition = (_updateCount % 360) * 1.0f;
    
    // Calculer l'angle de direction avec une fonction en 8
    // La formule utilise sin(2x) pour créer un 8 horizontal
    _targetDirectionAngle = EIGHT_PATTERN_AMPLITUDE * sin(2 * cyclePosition * PI / 180.0f);
    
    // Angle de trim optimal (pourrait être adapté selon la force du vent)
    _targetTrimAngle = 15.0f;
    
    // Mode générateur avec puissance adaptée à la position dans le cycle
    _targetWinchMode = WINCH_MODE_GENERATOR;
    
    // Plus de puissance quand le kite traverse le centre du 8 (vitesse maximale)
    float centerFactor = abs(cos(2 * cyclePosition * PI / 180.0f));
    _targetWinchPower = 50.0f + 30.0f * centerFactor;
    
    // Mise à jour de la puissance générée (simulation améliorée)
    float windFactor = _lastWindData.isValid ? (_lastWindData.speed / 10.0f) : 1.0f;
    float tensionFactor = _lastLineData.isTensionValid ? (_lastLineData.tension / 500.0f) : 1.0f;
    float instantPower = _targetWinchPower * windFactor * tensionFactor;
    _status.powerGenerated = 0.9f * _status.powerGenerated + 0.1f * instantPower;
    
    // Accumuler l'énergie totale (Wh)
    _status.addEnergy(_status.powerGenerated * (1.0f / 3600.0f / 20.0f));  // 1/20e de seconde en heures
}

void AutopilotModule::updateCircularMode() {
    // Génération d'un motif circulaire
    // Utile pour une génération d'énergie plus stable mais moins efficace
    
    // Déterminer l'angle dans le cycle (0-360 degrés)
    float cyclePosition = (_updateCount % 360) * 1.0f;
    
    // Calculer l'angle de direction pour un cercle
    _targetDirectionAngle = CIRCULAR_PATTERN_RADIUS * sin(cyclePosition * PI / 180.0f);
    
    // Angle de trim constant pour ce mode
    _targetTrimAngle = 10.0f;
    
    // Mode générateur avec puissance constante
    _targetWinchMode = WINCH_MODE_GENERATOR;
    _targetWinchPower = 60.0f;
    
    // Mise à jour de la puissance générée (simulation simple)
    float windFactor = _lastWindData.isValid ? (_lastWindData.speed / 10.0f) : 1.0f;
    float instantPower = _targetWinchPower * windFactor;
    _status.powerGenerated = 0.8f * _status.powerGenerated + 0.2f * instantPower;
    
    // Accumuler l'énergie totale (Wh)
    _status.addEnergy(_status.powerGenerated * (1.0f / 3600.0f / 20.0f));  // 1/20e de seconde en heures
}

void AutopilotModule::updatePowerGenerationMode() {
    // Mode optimisé pour la génération de puissance maximale
    // Utilise une combinaison des meilleurs aspects des autres modes
    
    // Alterner entre figure en 8 rapide et phases de forte traction
    uint32_t cycleTime = (_updateCount / 100) % 3;  // Cycle de 3 phases
    
    if (cycleTime == 0) {
        // Phase 1: Vol en 8 rapide
        float cyclePosition = (_updateCount % 180) * 2.0f;  // Double vitesse
        _targetDirectionAngle = EIGHT_PATTERN_AMPLITUDE * sin(2 * cyclePosition * PI / 180.0f);
        _targetTrimAngle = 20.0f;  // Angle plus agressif
        _targetWinchPower = 70.0f;
    } else if (cycleTime == 1) {
        // Phase 2: Traction maximale au centre
        _targetDirectionAngle = 0.0f;
        _targetTrimAngle = 25.0f;
        _targetWinchPower = 90.0f;
    } else {
        // Phase 3: Transition de retour
        _targetDirectionAngle = 0.0f;
        _targetTrimAngle = 10.0f;
        _targetWinchPower = 40.0f;
    }
    
    // Toujours en mode générateur
    _targetWinchMode = WINCH_MODE_GENERATOR;
    
    // Mise à jour de la puissance générée (simulation plus réaliste)
    float windFactor = _lastWindData.isValid ? (_lastWindData.speed / 10.0f) : 1.0f;
    float tensionFactor = _lastLineData.isTensionValid ? (_lastLineData.tension / 500.0f) : 1.0f;
    float efficiencyFactor = 0.8f; // Rendement du système
    float instantPower = _targetWinchPower * windFactor * tensionFactor * efficiencyFactor;
    _status.powerGenerated = 0.9f * _status.powerGenerated + 0.1f * instantPower;
    
    // Accumuler l'énergie totale (Wh)
    _status.addEnergy(_status.powerGenerated * (UPDATE_INTERVAL_MS / 1000.0f / 3600.0f));
}

// Fonction utilitaire pour mapper une valeur float
float AutopilotModule::mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
