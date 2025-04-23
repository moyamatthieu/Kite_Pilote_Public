/*
 * Module de gestion des capteurs pour le projet Kite Pilote
 * 
 * Ce module centralise la gestion de tous les capteurs du système:
 * - IMU (Inertial Measurement Unit) pour l'orientation du kite
 * - Capteur de tension des lignes
 * - Capteur de longueur des lignes
 * - Anémomètre/Girouette pour les données de vent
 * 
 * Version: 2.0.0.5
 * Créé le: 17/04/2025
 * Mis à jour le: 23/04/2025
 */

#ifndef SENSOR_MODULE_H
#define SENSOR_MODULE_H

#include <Arduino.h>
#include <Wire.h>
#include "../core/config.h"
#include "../core/data_types.h"
#include "../utils/logger.h"

#ifndef SIMULATION_MODE
#include <MPU6050_tockn.h> // Bibliothèque pour l'IMU
#include <HX711.h>        // Bibliothèque pour cellule de charge (tension des lignes)
#endif

/**
 * @class SensorModule
 * @brief Gère l'acquisition et le traitement des données des capteurs
 * 
 * Cette classe centralise toutes les opérations liées aux capteurs pour:
 * - Initialiser les différents capteurs
 * - Lire et filtrer les données
 * - Valider les mesures et détecter les anomalies
 * - Fournir une interface unifiée pour l'accès aux données
 */
class SensorModule {
public:
    /**
     * @brief Constructeur par défaut
     */
    SensorModule();
    
    /**
     * @brief Destructeur
     */
    ~SensorModule();
    
    /**
     * @brief Initialise tous les capteurs du système
     * @return true si l'initialisation s'est déroulée correctement
     */
    bool begin();
    
    /**
     * @brief Met à jour les données de tous les capteurs
     * Doit être appelée régulièrement dans la boucle principale
     */
    void update();
    
    /**
     * @brief Récupère les données actuelles de l'IMU
     * @return Structure IMUData contenant les mesures d'orientation
     */
    IMUData getIMUData() const;
    
    /**
     * @brief Récupère les données actuelles des lignes
     * @return Structure LineData contenant tension et longueur
     */
    LineData getLineData() const;
    
    /**
     * @brief Récupère les données actuelles du vent
     * @return Structure WindData contenant vitesse et direction
     */
    WindData getWindData() const;
    
    // Accesseurs pour les valeurs individuelles fréquemment utilisées
    /**
     * @brief Renvoie l'angle de roulis actuel en degrés
     * @return float Angle de roulis (-180 à +180)
     */
    float getRoll() const;
    
    /**
     * @brief Renvoie l'angle de tangage actuel en degrés
     * @return float Angle de tangage (-90 à +90)
     */
    float getPitch() const;
    
    /**
     * @brief Renvoie l'angle de lacet actuel en degrés
     * @return float Angle de lacet (0 à 359)
     */
    float getYaw() const;
    
    /**
     * @brief Renvoie la tension actuelle des lignes en Newtons
     * @return float Tension mesurée
     */
    float getLineTension() const;
    
    /**
     * @brief Renvoie la longueur actuelle des lignes en mètres
     * @return float Longueur mesurée
     */
    float getLineLength() const;
    
    /**
     * @brief Renvoie la vitesse actuelle du vent en m/s
     * @return float Vitesse du vent
     */
    float getWindSpeed() const;
    
    /**
     * @brief Renvoie la direction actuelle du vent en degrés
     * @return float Direction du vent (0 à 359, 0 = Nord)
     */
    float getWindDirection() const;
    
    /**
     * @brief Vérifie si tous les capteurs essentiels sont opérationnels
     * @return true si tous les capteurs essentiels sont valides
     */
    bool allSensorsValid() const;
    
    /**
     * @brief Effectue une calibration de l'IMU
     * @return true si la calibration a réussi
     */
    bool calibrateIMU();
    
    /**
     * @brief Effectue une calibration du capteur de tension
     * @param knownWeight Poids connu en Newtons pour la calibration
     * @return true si la calibration a réussi
     */
    bool calibrateTensionSensor(float knownWeight = 0.0f);
    
#ifdef SIMULATION_MODE
    // Méthodes spécifiques au mode simulation
    /**
     * @brief Définit manuellement les données de l'IMU (simulation)
     * @param roll Angle de roulis en degrés
     * @param pitch Angle de tangage en degrés
     * @param yaw Angle de lacet en degrés
     */
    void setIMUData(float roll, float pitch, float yaw);
    
    /**
     * @brief Définit manuellement la tension des lignes (simulation)
     * @param tension Tension en Newtons
     */
    void setLineTension(float tension);
    
    /**
     * @brief Définit manuellement la longueur des lignes (simulation)
     * @param length Longueur en mètres
     */
    void setLineLength(float length);
    
    /**
     * @brief Définit manuellement les données du vent (simulation)
     * @param speed Vitesse du vent en m/s
     * @param direction Direction du vent en degrés
     */
    void setWindData(float speed, float direction);
#endif

private:
    // Données des capteurs
    IMUData m_imuData;
    LineData m_lineData;
    WindData m_windData;
    
    // États d'initialisation des capteurs
    bool m_imuInitialized;
    bool m_tensionSensorInitialized;
    bool m_lengthSensorInitialized;
    bool m_windSensorInitialized;
    
    // Constantes pour le filtrage
    static constexpr float IMU_FILTER_ALPHA = 0.8f;  // Coefficient du filtre passe-bas pour l'IMU
    static constexpr float TENSION_FILTER_ALPHA = 0.7f; // Coefficient pour la tension
    static constexpr float LENGTH_FILTER_ALPHA = 0.9f;  // Coefficient pour la longueur
    static constexpr float WIND_FILTER_ALPHA = 0.7f;    // Coefficient pour le vent
    
    // Valeurs filtrées précédentes (pour le filtre passe-bas)
    float m_filteredRoll;
    float m_filteredPitch;
    float m_filteredYaw;
    float m_filteredTension;
    float m_filteredLength;
    float m_filteredWindSpeed;
    float m_filteredWindDirection;
    
#ifndef SIMULATION_MODE
    // Instances des capteurs réels
    MPU6050* m_imu;        // IMU pour orientation
    HX711* m_loadCell;     // Cellule de charge pour tension
    
    // Paramètres de calibration
    float m_tensionScale;  // Facteur d'échelle pour convertir les lectures en Newtons
    float m_tensionOffset; // Offset pour le zéro
#endif

    /**
     * @brief Réinitialise toutes les données des capteurs
     */
    void resetAllData();
    
    /**
     * @brief Initialise l'IMU (MPU6050)
     * @return true si l'initialisation a réussi
     */
    bool initIMU();
    
    /**
     * @brief Initialise le capteur de tension des lignes
     * @return true si l'initialisation a réussi
     */
    bool initTensionSensor();
    
    /**
     * @brief Initialise le capteur de longueur des lignes
     * @return true si l'initialisation a réussi
     */
    bool initLengthSensor();
    
    /**
     * @brief Initialise les capteurs de vent (anémomètre/girouette)
     * @return true si l'initialisation a réussi
     */
    bool initWindSensor();
    
    /**
     * @brief Met à jour les données de l'IMU
     * @return true si la mise à jour a réussi
     */
    bool updateIMU();
    
    /**
     * @brief Met à jour les données de tension des lignes
     * @return true si la mise à jour a réussi
     */
    bool updateLineTension();
    
    /**
     * @brief Met à jour les données de longueur des lignes
     * @return true si la mise à jour a réussi
     */
    bool updateLineLength();
    
    /**
     * @brief Met à jour les données du vent
     * @return true si la mise à jour a réussi
     */
    bool updateWindSensor();
    
    /**
     * @brief Applique un filtrage aux données brutes
     * Utilise des filtres passe-bas pour réduire le bruit
     */
    void filterData();
};

#endif // SENSOR_MODULE_H
