/*
 * Implémentation du module de gestion des capteurs pour le projet Kite Pilote
 * 
 * Gère la lecture et le traitement des données des différents capteurs:
 * - IMU (MPU6050) pour l'orientation
 * - Capteur de tension des lignes (cellule de charge)
 * - Capteur de longueur des lignes
 * - Anémomètre/girouette pour les données de vent
 * 
 * Version: 2.0.0.5
 * Créé le: 23/04/2025
 */

#include "modules/sensor_module.h"

// Constructeur
SensorModule::SensorModule() 
#ifndef SIMULATION_MODE
    : m_imu(nullptr), m_loadCell(nullptr), m_tensionScale(1.0f), m_tensionOffset(0.0f)
#endif
{
    resetAllData();
}

// Destructeur - libération des ressources
SensorModule::~SensorModule() {
#ifndef SIMULATION_MODE
    // Libération de la mémoire allouée pour les capteurs
    if (m_imu != nullptr) {
        delete m_imu;
        m_imu = nullptr;
    }
    
    if (m_loadCell != nullptr) {
        delete m_loadCell;
        m_loadCell = nullptr;
    }
#endif
}

// Initialisation de tous les capteurs
bool SensorModule::begin() {
    bool success = true;
    
    LOG_INFO("CAPTEUR", "Initialisation des capteurs...");
    
#ifdef SIMULATION_MODE
    // En mode simulation, on initialise des capteurs virtuels
    LOG_INFO("CAPTEUR", "Mode simulation activé, utilisation de capteurs virtuels");
    m_imuInitialized = true;
    m_tensionSensorInitialized = true;
    m_lengthSensorInitialized = true;
    m_windSensorInitialized = true;
#else
    // Initialisation des capteurs réels
    m_imuInitialized = initIMU();
    m_tensionSensorInitialized = initTensionSensor();
    m_lengthSensorInitialized = initLengthSensor();
    m_windSensorInitialized = initWindSensor();
#endif
    
    // Vérifier l'initialisation des capteurs essentiels
    if (!m_imuInitialized) {
        LOG_ERROR("CAPTEUR", "Échec de l'initialisation de l'IMU - Mode dégradé activé");
        // Même si l'IMU a échoué, nous continuons à fonctionner en mode dégradé
        // Nous réglons toutefois les données comme invalides pour que le reste du système sache
        // qu'il doit utiliser des valeurs de repli
        m_imuData.isValid = false;
    }
    
    if (!m_tensionSensorInitialized) {
        LOG_ERROR("CAPTEUR", "Échec de l'initialisation du capteur de tension - Mode dégradé activé");
        // Nous continuons à fonctionner même sans capteur de tension, mais marquons les données comme invalides
        m_lineData.isTensionValid = false;
    }
    
    // L'anémomètre est optionnel
    if (!m_windSensorInitialized) {
        LOG_WARNING("CAPTEUR", "Anémomètre non disponible - Utilisation de valeurs par défaut");
        m_windData.isValid = false;
    }
    
    // Même en cas d'échec des capteurs critiques, nous continuons à fonctionner
    // Le système doit rester opérationnel même avec des fonctionnalités réduites
    LOG_INFO("CAPTEUR", "Module capteurs initialisé (mode %s)", 
            (m_imuInitialized && m_tensionSensorInitialized) ? "normal" : "dégradé");
    
    // Nous retournons toujours true pour permettre au système de démarrer
    // même avec des capteurs défectueux
    return true;
}

// Mise à jour des données de tous les capteurs
void SensorModule::update() {
#ifndef SIMULATION_MODE
    // En mode réel, lire les données des capteurs physiques
    updateIMU();
    updateLineTension();
    updateLineLength();
    updateWindSensor();
#endif
    
    // Filtrage des données pour réduire le bruit
    filterData();
}

// Méthode privée pour réinitialiser toutes les données
void SensorModule::resetAllData() {
    // Réinitialiser les données des capteurs
    m_imuData = IMUData();
    m_lineData = LineData();
    m_windData = WindData();
    
    // Réinitialiser les états d'initialisation
    m_imuInitialized = false;
    m_tensionSensorInitialized = false;
    m_lengthSensorInitialized = false;
    m_windSensorInitialized = false;
    
    // Réinitialiser les valeurs filtrées
    m_filteredRoll = 0.0f;
    m_filteredPitch = 0.0f;
    m_filteredYaw = 0.0f;
    m_filteredTension = 0.0f;
    m_filteredLength = 0.0f;
    m_filteredWindSpeed = 0.0f;
    m_filteredWindDirection = 0.0f;
}

// Accesseurs pour les structures de données
IMUData SensorModule::getIMUData() const {
    return m_imuData;
}

LineData SensorModule::getLineData() const {
    return m_lineData;
}

WindData SensorModule::getWindData() const {
    return m_windData;
}

// Accesseurs pour les valeurs individuelles
float SensorModule::getRoll() const {
    return m_imuData.roll;
}

float SensorModule::getPitch() const {
    return m_imuData.pitch;
}

float SensorModule::getYaw() const {
    return m_imuData.yaw;
}

float SensorModule::getLineTension() const {
    return m_lineData.tension;
}

float SensorModule::getLineLength() const {
    return m_lineData.length;
}

float SensorModule::getWindSpeed() const {
    return m_windData.speed;
}

float SensorModule::getWindDirection() const {
    return m_windData.direction;
}

// Vérification de la validité de tous les capteurs essentiels
bool SensorModule::allSensorsValid() const {
    return m_imuData.isValid && m_lineData.isTensionValid;
}

#ifdef SIMULATION_MODE
// Méthodes d'accès pour la simulation
void SensorModule::setIMUData(float roll, float pitch, float yaw) {
    m_imuData.roll = roll;
    m_imuData.pitch = pitch;
    m_imuData.yaw = yaw;
    m_imuData.isValid = true;
    m_imuData.updateTimestamp();
}

void SensorModule::setLineTension(float tension) {
    m_lineData.tension = tension;
    m_lineData.isTensionValid = true;
    m_lineData.updateTimestamp();
    m_lineData.updateMaxTension();
}

void SensorModule::setLineLength(float length) {
    m_lineData.length = length;
    m_lineData.isLengthValid = true;
    m_lineData.updateTimestamp();
}

void SensorModule::setWindData(float speed, float direction) {
    m_windData.speed = speed;
    m_windData.direction = direction;
    m_windData.isValid = true;
    m_windData.updateTimestamp();
}
#endif

// Initialisation de l'IMU (MPU6050)
bool SensorModule::initIMU() {
#ifndef SIMULATION_MODE
    LOG_INFO("CAPTEUR", "Initialisation de l'IMU...");
    
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    
    // Créer et initialiser l'instance de MPU6050
    m_imu = new MPU6050(Wire);
    
    if (m_imu == nullptr) {
        LOG_ERROR("CAPTEUR", "Échec d'allocation mémoire pour l'IMU");
        return false;
    }
    
    // Tenter de démarrer l'IMU
    m_imu->begin();
    
    // Temporisation pour laisser le temps à l'IMU de s'initialiser
    delay(100);
    
    // Calibration de base (peut être raffinée ultérieurement)
    LOG_INFO("CAPTEUR", "Calibration de l'IMU...");
    m_imu->calcGyroOffsets(true);
    
    LOG_INFO("CAPTEUR", "IMU initialisé avec succès");
    return true;
#else
    return true; // Toujours vrai en mode simulation
#endif
}

// Initialisation du capteur de tension des lignes
bool SensorModule::initTensionSensor() {
#ifndef SIMULATION_MODE
    LOG_INFO("CAPTEUR", "Initialisation du capteur de tension...");
    
    // Utilisation d'une cellule de charge avec amplificateur HX711
    const int TENSION_DOUT_PIN = 25; // Modifier selon le branchement réel
    const int TENSION_SCK_PIN = 26;  // Modifier selon le branchement réel
    
    m_loadCell = new HX711();
    
    if (m_loadCell == nullptr) {
        LOG_ERROR("CAPTEUR", "Échec d'allocation mémoire pour le capteur de tension");
        return false;
    }
    
    // Initialiser l'HX711
    m_loadCell->begin(TENSION_DOUT_PIN, TENSION_SCK_PIN);
    
    if (!m_loadCell->is_ready()) {
        LOG_ERROR("CAPTEUR", "HX711 non détecté");
        delete m_loadCell;
        m_loadCell = nullptr;
        return false;
    }
    
    // Paramètres de calibration par défaut (à calibrer avec calibrateTensionSensor)
    m_tensionScale = 22.0f; // Valeur typique pour convertir en Newtons (à ajuster)
    m_tensionOffset = m_loadCell->read_average(10);
    
    LOG_INFO("CAPTEUR", "Capteur de tension initialisé, offset: %.2f", m_tensionOffset);
    return true;
#else
    return true; // Toujours vrai en mode simulation
#endif
}

// Initialisation du capteur de longueur des lignes
bool SensorModule::initLengthSensor() {
#ifndef SIMULATION_MODE
    LOG_INFO("CAPTEUR", "Initialisation du capteur de longueur...");
    // Dans cette version, nous implémentons une estimation de la longueur
    // basée sur d'autres paramètres plutôt qu'un capteur physique dédié
    
    // Initialisation minimale sans capteur physique
    LOG_WARNING("CAPTEUR", "Capteur de longueur non implémenté, utilisation d'une estimation");
    
    // Valeur initiale par défaut
    m_lineData.length = 30.0f; // Longueur initiale estimée en mètres
    m_lineData.isLengthValid = true;
    
    return true;
#else
    return true; // Toujours vrai en mode simulation
#endif
}

// Initialisation des capteurs de vent
bool SensorModule::initWindSensor() {
#ifndef SIMULATION_MODE
    LOG_INFO("CAPTEUR", "Initialisation des capteurs de vent...");
    
    // Dans cette version, nous implémentons une version simple
    // basée sur des capteurs analogiques pour l'anémomètre et la girouette
    
    // Pins pour les capteurs de vent (à adapter)
    const int ANEMOMETER_PIN = 34; // ADC pour l'anémomètre
    const int VANE_PIN = 35;       // ADC pour la girouette
    
    // Configuration des pins en entrée analogique
    pinMode(ANEMOMETER_PIN, INPUT);
    pinMode(VANE_PIN, INPUT);
    
    // La calibration précise sera effectuée lors des lectures
    
    LOG_INFO("CAPTEUR", "Capteurs de vent initialisés");
    return true;
#else
    return true; // Toujours vrai en mode simulation
#endif
}

// Mise à jour des données de l'IMU
bool SensorModule::updateIMU() {
#ifndef SIMULATION_MODE
    if (!m_imuInitialized || m_imu == nullptr) {
        // Mode dégradé: si l'IMU n'est pas disponible, nous utilisons des valeurs de repli
        // qui peuvent être basées sur la dernière lecture valide ou des valeurs par défaut
        
        // Nous essayons périodiquement de récupérer l'IMU
        static unsigned long lastRetryTime = 0;
        const unsigned long RETRY_INTERVAL = 10000; // 10 secondes entre les tentatives
        
        unsigned long currentTime = millis();
        if (currentTime - lastRetryTime > RETRY_INTERVAL) {
            lastRetryTime = currentTime;
            
            LOG_INFO("CAPTEUR", "Tentative de récupération de l'IMU...");
            if (initIMU()) {
                LOG_INFO("CAPTEUR", "IMU récupéré avec succès!");
                m_imuInitialized = true;
                // On continuera avec les nouvelles lectures au prochain appel
            }
        }
        
        // En attendant, on signale que les données ne sont pas valides
        m_imuData.isValid = false;
        return false;
    }
    
    // Mettre à jour les données de l'IMU
    m_imu->update();
    
    // Vérification supplémentaire pour détecter les problèmes de communication I2C
    bool dataReasonable = true;
    
    // Récupérer les valeurs
    float newRoll = m_imu->getAngleX();
    float newPitch = m_imu->getAngleY();
    float newYaw = m_imu->getAngleZ();
    
    // Vérifier la plausibilité des nouvelles données
    // Si les valeurs changent de manière irréaliste, cela peut indiquer un problème de capteur
    const float MAX_CHANGE_RATE = 30.0f; // degrés par appel (ajuster selon la fréquence d'appel)
    
    if (m_imuData.isValid) {
        if (abs(newRoll - m_imuData.roll) > MAX_CHANGE_RATE ||
            abs(newPitch - m_imuData.pitch) > MAX_CHANGE_RATE) {
            LOG_WARNING("CAPTEUR", "Changement suspect dans les données IMU - filtrage renforcé");
            dataReasonable = false;
            
            // En cas de problème, on utilise un filtrage plus agressif pour limiter l'impact
            // des lectures aberrantes tout en permettant un retour progressif à la normale
            newRoll = m_imuData.roll * 0.9f + newRoll * 0.1f;
            newPitch = m_imuData.pitch * 0.9f + newPitch * 0.1f;
            newYaw = m_imuData.yaw * 0.9f + newYaw * 0.1f;
        }
    }
    
    m_imuData.roll = newRoll;
    m_imuData.pitch = newPitch;
    m_imuData.yaw = newYaw;
    
    // Normaliser la valeur de yaw pour qu'elle soit dans [0, 360)
    while (m_imuData.yaw < 0) m_imuData.yaw += 360.0f;
    while (m_imuData.yaw >= 360.0f) m_imuData.yaw -= 360.0f;
    
    // Récupérer les accélérations
    m_imuData.accX = m_imu->getAccX();
    m_imuData.accY = m_imu->getAccY();
    m_imuData.accZ = m_imu->getAccZ();
    
    // Mettre à jour l'horodatage et la validité
    m_imuData.updateTimestamp();
    m_imuData.isValid = dataReasonable; // Mise à jour de la validité basée sur notre vérification
    
    return dataReasonable;
#else
    return false; // Ne devrait pas être appelé en mode simulation
#endif
}

// Mise à jour des données de tension des lignes
bool SensorModule::updateLineTension() {
#ifndef SIMULATION_MODE
    if (!m_tensionSensorInitialized || m_loadCell == nullptr) {
        // Mode dégradé: tentative périodique de récupération du capteur
        static unsigned long lastRetryTime = 0;
        const unsigned long RETRY_INTERVAL = 15000; // 15 secondes entre les tentatives
        
        unsigned long currentTime = millis();
        if (currentTime - lastRetryTime > RETRY_INTERVAL) {
            lastRetryTime = currentTime;
            
            LOG_INFO("CAPTEUR", "Tentative de récupération du capteur de tension...");
            if (initTensionSensor()) {
                LOG_INFO("CAPTEUR", "Capteur de tension récupéré avec succès!");
                m_tensionSensorInitialized = true;
            }
        }
        
        // En l'absence de capteur, nous utilisons une valeur estimée basée sur d'autres paramètres
        // Par exemple, nous pourrions estimer la tension à partir de l'orientation du kite
        if (m_imuData.isValid) {
            // Estimation très basique basée sur l'angle de pitch du kite
            // Évidemment, cette estimation est très approximative et devrait être améliorée
            // avec un modèle plus sophistiqué dans une implémentation réelle
            float estimatedTension = 0.0f;
            if (m_imuData.pitch > 0) { // Si le kite pointe vers le haut
                estimatedTension = 100.0f + m_imuData.pitch * 5.0f; // Tension de base + facteur d'angle
            } else {
                estimatedTension = max(50.0f, 100.0f + m_imuData.pitch * 2.0f); // Tension minimale
            }
            
            // Limites raisonnables
            estimatedTension = constrain(estimatedTension, 0.0f, MAX_SAFE_TENSION);
            
            // Mise à jour avec indication que c'est une valeur estimée
            m_lineData.tension = estimatedTension;
            m_lineData.isTensionValid = false; // Marquer comme non valide car c'est une estimation
            m_lineData.updateTimestamp();
            
            LOG_DEBUG("CAPTEUR", "Tension estimée à %.2fN (basée sur IMU)", estimatedTension);
            return true;
        }
        
        return false;
    }
    
    // Vérifier si le capteur est prêt
    if (m_loadCell->is_ready()) {
        // Lire la valeur brute
        long rawValue = m_loadCell->read();
        
        // Appliquer la calibration
        float tension = (rawValue - m_tensionOffset) / m_tensionScale;
        
        // Limiter aux valeurs positives (la tension ne peut être négative)
        tension = max(0.0f, tension);
        
        // Vérifier la plausibilité - changement trop rapide pourrait indiquer une erreur de capteur
        static float lastValidTension = tension;
        const float MAX_CHANGE_RATE = 50.0f; // Newtons par appel (à ajuster)
        
        bool valueReasonable = true;
        if (m_lineData.isTensionValid && abs(tension - lastValidTension) > MAX_CHANGE_RATE) {
            LOG_WARNING("CAPTEUR", "Changement suspect de tension: %.2f -> %.2f N", lastValidTension, tension);
            valueReasonable = false;
            
            // Filtre anti-spike
            tension = lastValidTension * 0.8f + tension * 0.2f;
        }
        
        if (valueReasonable) {
            lastValidTension = tension;
        }
        
        // Mettre à jour les données
        m_lineData.tension = tension;
        m_lineData.isTensionValid = valueReasonable;
        m_lineData.updateTimestamp();
        m_lineData.updateMaxTension();
        
        return valueReasonable;
    } else {
        // Le capteur n'est pas prêt - cela pourrait indiquer un problème temporaire
        static int readFailCount = 0;
        readFailCount++;
        
        // Après plusieurs échecs consécutifs, nous considérons que le capteur est défaillant
        if (readFailCount > 10) {
            LOG_ERROR("CAPTEUR", "Capteur de tension non réactif après %d tentatives", readFailCount);
            m_tensionSensorInitialized = false; // Marquer comme non initialisé pour déclencher la procédure de récupération
            readFailCount = 0; // Réinitialiser pour le prochain cycle
        }
    }
    
    return false;
#else
    return false; // Ne devrait pas être appelé en mode simulation
#endif
}

// Mise à jour des données de longueur des lignes
bool SensorModule::updateLineLength() {
#ifndef SIMULATION_MODE
    if (!m_lengthSensorInitialized) {
        return false;
    }
    
    // Dans cette version, nous estimons la longueur plutôt que de la mesurer directement
    // L'estimation pourrait être basée sur le temps de déroulement, l'angle, etc.
    
    // Pour cette version simple, nous gardons la longueur constante sauf si elle est mise à jour ailleurs
    
    // Mise à jour de l'horodatage uniquement
    if (m_lineData.isLengthValid) {
        m_lineData.updateTimestamp();
        return true;
    }
    
    return false;
#else
    return false; // Ne devrait pas être appelé en mode simulation
#endif
}

// Mise à jour des données du vent
bool SensorModule::updateWindSensor() {
#ifndef SIMULATION_MODE
    if (!m_windSensorInitialized) {
        return false;
    }
    
    // Pins pour les capteurs de vent (les mêmes que dans initWindSensor)
    const int ANEMOMETER_PIN = 34;
    const int VANE_PIN = 35;
    
    // Lire les valeurs analogiques
    int anemometerRaw = analogRead(ANEMOMETER_PIN);
    int vaneRaw = analogRead(VANE_PIN);
    
    // Convertir en unités physiques
    // Ces facteurs doivent être ajustés selon la calibration spécifique des capteurs
    const float ANEMOMETER_FACTOR = 0.1f; // Facteur de conversion en m/s
    float windSpeed = anemometerRaw * ANEMOMETER_FACTOR;
    
    // Conversion de la direction (0-4095 => 0-359 degrés)
    float windDirection = (vaneRaw * 359.0f) / 4095.0f;
    
    // Mettre à jour les données
    m_windData.speed = windSpeed;
    m_windData.direction = windDirection;
    
    // Si la vitesse dépasse le maximum précédent, mettre à jour
    if (windSpeed > m_windData.gustSpeed) {
        m_windData.gustSpeed = windSpeed;
    }
    
    // Mettre à jour l'horodatage et la validité
    m_windData.updateTimestamp();
    m_windData.isValid = true;
    
    return true;
#else
    return false; // Ne devrait pas être appelé en mode simulation
#endif
}

// Calibration de l'IMU
bool SensorModule::calibrateIMU() {
#ifndef SIMULATION_MODE
    if (!m_imuInitialized || m_imu == nullptr) {
        LOG_ERROR("CAPTEUR", "Impossible de calibrer l'IMU - non initialisé");
        return false;
    }
    
    LOG_INFO("CAPTEUR", "Calibration de l'IMU en cours...");
    LOG_INFO("CAPTEUR", "Maintenir l'IMU immobile pendant la calibration");
    
    // Calibration des offsets du gyroscope (vrai = afficher les résultats sur série)
    m_imu->calcGyroOffsets(true);
    
    LOG_INFO("CAPTEUR", "Calibration de l'IMU terminée");
    return true;
#else
    LOG_INFO("CAPTEUR", "Calibration de l'IMU simulée");
    return true;
#endif
}

// Calibration du capteur de tension
bool SensorModule::calibrateTensionSensor(float knownWeight) {
#ifndef SIMULATION_MODE
    if (!m_tensionSensorInitialized || m_loadCell == nullptr) {
        LOG_ERROR("CAPTEUR", "Impossible de calibrer le capteur de tension - non initialisé");
        return false;
    }
    
    LOG_INFO("CAPTEUR", "Calibration du capteur de tension...");
    
    // Calibration du zéro (sans charge)
    LOG_INFO("CAPTEUR", "Retirer toute charge et appuyer sur Entrée");
    
    // En pratique, il faudrait attendre une confirmation de l'utilisateur ici
    // Pour cette implémentation, nous calibrons directement
    
    // Lire la valeur à vide
    m_tensionOffset = m_loadCell->read_average(10);
    LOG_INFO("CAPTEUR", "Offset à vide: %.2f", m_tensionOffset);
    
    // Si un poids connu est fourni, calibrer le facteur d'échelle
    if (knownWeight > 0.0f) {
        LOG_INFO("CAPTEUR", "Placer un poids connu de %.2f N et appuyer sur Entrée", knownWeight);
        
        // En pratique, attendre la confirmation de l'utilisateur
        
        // Lire la valeur avec le poids connu
        long valueWithWeight = m_loadCell->read_average(10);
        
        // Calculer le facteur d'échelle
        m_tensionScale = (valueWithWeight - m_tensionOffset) / knownWeight;
        
        LOG_INFO("CAPTEUR", "Facteur d'échelle: %.2f", m_tensionScale);
    }
    
    LOG_INFO("CAPTEUR", "Calibration du capteur de tension terminée");
    return true;
#else
    LOG_INFO("CAPTEUR", "Calibration du capteur de tension simulée");
    return true;
#endif
}

// Filtrage des données pour réduire le bruit
void SensorModule::filterData() {
    // Filtre passe-bas simple pour l'IMU
    if (m_imuData.isValid) {
        m_filteredRoll = IMU_FILTER_ALPHA * m_filteredRoll + (1.0f - IMU_FILTER_ALPHA) * m_imuData.roll;
        m_filteredPitch = IMU_FILTER_ALPHA * m_filteredPitch + (1.0f - IMU_FILTER_ALPHA) * m_imuData.pitch;
        m_filteredYaw = IMU_FILTER_ALPHA * m_filteredYaw + (1.0f - IMU_FILTER_ALPHA) * m_imuData.yaw;
        
        m_imuData.roll = m_filteredRoll;
        m_imuData.pitch = m_filteredPitch;
        m_imuData.yaw = m_filteredYaw;
    }
    
    // Filtre pour la tension des lignes
    if (m_lineData.isTensionValid) {
        m_filteredTension = TENSION_FILTER_ALPHA * m_filteredTension + 
                           (1.0f - TENSION_FILTER_ALPHA) * m_lineData.tension;
        m_lineData.tension = m_filteredTension;
    }
    
    // Filtre pour la longueur des lignes
    if (m_lineData.isLengthValid) {
        m_filteredLength = LENGTH_FILTER_ALPHA * m_filteredLength + 
                          (1.0f - LENGTH_FILTER_ALPHA) * m_lineData.length;
        m_lineData.length = m_filteredLength;
    }
    
    // Filtre pour les données de vent
    if (m_windData.isValid) {
        m_filteredWindSpeed = WIND_FILTER_ALPHA * m_filteredWindSpeed + 
                             (1.0f - WIND_FILTER_ALPHA) * m_windData.speed;
        
        // Filtrage directionnel spécial pour éviter les problèmes à 0/360 degrés
        float sinDirection = sin(m_windData.direction * PI / 180.0f);
        float cosDirection = cos(m_windData.direction * PI / 180.0f);
        
        static float filteredSinDirection = sinDirection;
        static float filteredCosDirection = cosDirection;
        
        filteredSinDirection = WIND_FILTER_ALPHA * filteredSinDirection + 
                              (1.0f - WIND_FILTER_ALPHA) * sinDirection;
        filteredCosDirection = WIND_FILTER_ALPHA * filteredCosDirection + 
                              (1.0f - WIND_FILTER_ALPHA) * cosDirection;
        
        m_filteredWindDirection = atan2(filteredSinDirection, filteredCosDirection) * 180.0f / PI;
        if (m_filteredWindDirection < 0.0f) {
            m_filteredWindDirection += 360.0f;
        }
        
        m_windData.speed = m_filteredWindSpeed;
        m_windData.direction = m_filteredWindDirection;
    }
}
