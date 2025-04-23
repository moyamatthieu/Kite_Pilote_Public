/*
 * Module d'autopilote pour le projet Kite Pilote
 * 
 * Ce module implémente les différents modes de contrôle automatique du kite:
 * - Mode vol en huit (figure 8) pour maximiser la génération d'énergie
 * - Mode vol circulaire stabilisé
 * - Séquences de décollage et d'atterrissage
 * 
 * Version: 2.0.0.5
 * Créé le: 17/04/2025
 * Mis à jour le: 23/04/2025
 */

#ifndef AUTOPILOT_MODULE_H
#define AUTOPILOT_MODULE_H

#include <Arduino.h>
#include "../core/config.h"
#include "../core/data_types.h"
#include "../utils/logger.h"

/**
 * @class AutopilotModule
 * @brief Gère les algorithmes de contrôle automatique du kite
 * 
 * Cette classe implémente les différents modes de vol et séquences automatiques:
 * - Vol en 8 pour maximiser la génération d'énergie
 * - Vol circulaire pour une génération plus stable
 * - Séquences de décollage et d'atterrissage
 * - Mode d'optimisation de puissance
 */
class AutopilotModule {
public:
    /**
     * @brief Constructeur par défaut
     */
    AutopilotModule();
    
    /**
     * @brief Destructeur
     */
    ~AutopilotModule();
    
    /**
     * @brief Initialise le module d'autopilote
     * @return true si l'initialisation est réussie
     */
    bool begin();
    
    /**
     * @brief Définit le mode d'autopilote
     * @param mode Nouveau mode à activer
     * @return true si le changement de mode est réussi
     */
    bool setMode(AutopilotMode mode);
    
    /**
     * @brief Effectue un arrêt d'urgence
     * Met le système en mode sécuritaire immédiatement
     */
    void emergencyStop();
    
    /**
     * @brief Met à jour l'autopilote avec les données capteurs actuelles
     * @param imuData Données d'orientation du kite
     * @param lineData Données de tension et longueur des lignes
     * @param windData Données du vent
     */
    void update(const IMUData& imuData, const LineData& lineData, const WindData& windData);
    
    /**
     * @brief Obtient l'état actuel de l'autopilote
     * @return La structure d'état complète
     */
    AutopilotStatus getStatus() const;
    
    // Accesseurs pour les informations d'état courantes
    /**
     * @brief Obtient le mode actuel de l'autopilote
     * @return Le mode en cours
     */
    AutopilotMode getMode() const;
    
    /**
     * @brief Obtient le message de statut actuel
     * @return Message de statut textuel
     */
    const char* getStatusMessage() const;
    
    /**
     * @brief Obtient le pourcentage de complétion de la séquence en cours
     * @return Pourcentage (0-100%)
     */
    float getCompletionPercentage() const;
    
    /**
     * @brief Obtient la puissance générée actuellement
     * @return Puissance en watts
     */
    float getPowerGenerated() const;
    
    /**
     * @brief Obtient l'énergie totale générée
     * @return Énergie en watt-heures
     */
    float getTotalEnergy() const;
    
    // Accesseurs pour les valeurs cibles utilisées par les servomoteurs
    /**
     * @brief Obtient l'angle de direction cible
     * @return Angle en degrés
     */
    float getTargetDirectionAngle() const;
    
    /**
     * @brief Obtient l'angle de trim cible
     * @return Angle en degrés
     */
    float getTargetTrimAngle() const;
    
    /**
     * @brief Obtient le mode cible du treuil
     * @return Mode du treuil
     */
    WinchMode getTargetWinchMode() const;
    
    /**
     * @brief Obtient la puissance cible du treuil
     * @return Puissance en pourcentage (0-100%)
     */
    float getTargetWinchPower() const;

private:
    // État d'initialisation
    bool _initialized;
    
    // Données d'état et de contrôle
    AutopilotStatus _status;
    
    // Dernières données capteurs reçues
    IMUData _lastImuData;
    LineData _lastLineData;
    WindData _lastWindData;
    
    // Valeurs de contrôle calculées
    float _targetDirectionAngle;
    float _targetTrimAngle;
    WinchMode _targetWinchMode;
    float _targetWinchPower;
    
    // Variables pour suivre les motifs de vol
    uint32_t _updateCount;
    uint16_t _patternStep;
    unsigned long _lastModeChangeTime;
    
    // Paramètres configurables pour les différents modes
    const float EIGHT_PATTERN_AMPLITUDE = 35.0f;    // Amplitude du mouvement en 8 (degrés)
    const float CIRCULAR_PATTERN_RADIUS = 30.0f;    // Rayon du mouvement circulaire (degrés)
    const float LAUNCH_SEQUENCE_DURATION = 30000;   // Durée de la séquence de décollage (ms)
    const float LAND_SEQUENCE_DURATION = 30000;     // Durée de la séquence d'atterrissage (ms)
    
    /**
     * @brief Réinitialise l'état interne du module
     */
    void resetState();
    
    /**
     * @brief Fonction utilitaire pour la conversion linéaire de valeurs
     * @param x Valeur à convertir
     * @param in_min Minimum de la plage d'entrée
     * @param in_max Maximum de la plage d'entrée
     * @param out_min Minimum de la plage de sortie
     * @param out_max Maximum de la plage de sortie
     * @return Valeur convertie
     */
    float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
    
    // Implémentations des différents modes de vol
    
    /**
     * @brief Met à jour le mode veille
     * Maintient le kite en position stable et neutre
     */
    void updateStandbyMode();
    
    /**
     * @brief Met à jour le mode décollage
     * Séquence de décollage en trois phases
     */
    void updateLaunchMode();
    
    /**
     * @brief Met à jour le mode atterrissage
     * Séquence d'atterrissage contrôlée
     */
    void updateLandMode();
    
    /**
     * @brief Met à jour le mode vol en 8
     * Optimisé pour la génération d'énergie
     */
    void updateEightPatternMode();
    
    /**
     * @brief Met à jour le mode vol circulaire
     * Vol plus stable avec génération d'énergie constante
     */
    void updateCircularMode();
    
    /**
     * @brief Met à jour le mode génération de puissance
     * Combinaison optimale des techniques pour maximiser l'énergie
     */
    void updatePowerGenerationMode();
};

#endif // AUTOPILOT_MODULE_H
