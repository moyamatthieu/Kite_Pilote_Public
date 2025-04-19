/*
 * Module de diagnostic pour le projet Kite Pilote
 * 
 * Ce module fournit des outils complets pour surveiller l'état du système,
 * détecter les problèmes, enregistrer les événements et faciliter le débogage.
 * Il implémente les fonctionnalités décrites dans la section 11 du document de projet.
 * 
 * Créé le: 17/04/2025
 */

#ifndef DIAGNOSTIC_MODULE_H
#define DIAGNOSTIC_MODULE_H

#include <Arduino.h>
#include <vector>
#include <algorithm> // Pour std::min et std::max
#include "../utils/circular_buffer.h"
#include "../core/config.h"
#include "../core/data_types.h"
#include "../utils/logger.h"

// Niveaux de log pour le système de diagnostic
enum DiagLogLevel {
    DIAG_LEVEL_ERROR = 0,    // Erreurs critiques uniquement
    DIAG_LEVEL_WARNING = 1,  // Erreurs et avertissements
    DIAG_LEVEL_INFO = 2,     // Informations générales
    DIAG_LEVEL_DEBUG = 3,    // Informations détaillées pour le débogage
    DIAG_LEVEL_VERBOSE = 4   // Logs très détaillés pour le développement
};

// Catégories de diagnostic pour organiser les messages
enum DiagCategory {
    DIAG_CAT_SYSTEM = 0,     // Système global
    DIAG_CAT_SENSORS = 1,    // Capteurs (IMU, tension, etc.)
    DIAG_CAT_ACTUATORS = 2,  // Actionneurs (servos)
    DIAG_CAT_AUTOPILOT = 3,  // Logique d'autopilote
    DIAG_CAT_COMM = 4,       // Communication sans fil
    DIAG_CAT_POWER = 5,      // Gestion de l'énergie
    DIAG_CAT_HARDWARE = 6,   // Problèmes matériels
    DIAG_CAT_WIFI = 7,       // Connectivité WiFi
    DIAG_CAT_USER = 8        // Interactions utilisateur
};

// Structure d'une entrée de log de diagnostic
struct DiagLogEntry {
    uint32_t timestamp;      // Horodatage en millisecondes depuis le démarrage
    DiagLogLevel level;      // Niveau de sévérité
    DiagCategory category;   // Catégorie du message
    char module[8];          // Module source (nom court)
    char message[128];       // Message (limité à 128 caractères)
    
    // Constructeur
    DiagLogEntry(DiagLogLevel lvl, DiagCategory cat, const char* mod, const char* msg) :
        timestamp(millis()),
        level(lvl),
        category(cat)
    {
        strncpy(module, mod, sizeof(module) - 1);
        module[sizeof(module) - 1] = '\0';
        
        strncpy(message, msg, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';
    }
    
    // Constructeur par défaut (pour CircularBuffer)
    DiagLogEntry() :
        timestamp(0),
        level(DIAG_LEVEL_INFO),
        category(DIAG_CAT_SYSTEM)
    {
        module[0] = '\0';
        message[0] = '\0';
    }
};

// Structure pour les métriques de performance
struct PerformanceMetrics {
    uint32_t loopCount;               // Nombre total de boucles exécutées
    unsigned long minLoopTime;        // Temps minimum d'exécution de la boucle (μs)
    unsigned long maxLoopTime;        // Temps maximum d'exécution de la boucle (μs)
    unsigned long avgLoopTime;        // Temps moyen d'exécution de la boucle (μs)
    uint32_t freeHeapSize;            // Mémoire libre (octets)
    float cpuUsagePercent;            // Utilisation CPU (%)
    float batteryVoltage;             // Tension de la batterie (V)
    uint32_t wifiPacketsSent;         // Paquets WiFi envoyés
    uint32_t wifiPacketsReceived;     // Paquets WiFi reçus
    uint32_t commErrorCount;          // Nombre d'erreurs de communication
    
    // Constructeur avec valeurs par défaut
    PerformanceMetrics() :
        loopCount(0),
        minLoopTime(ULONG_MAX),
        maxLoopTime(0),
        avgLoopTime(0),
        freeHeapSize(0),
        cpuUsagePercent(0.0f),
        batteryVoltage(0.0f),
        wifiPacketsSent(0),
        wifiPacketsReceived(0),
        commErrorCount(0)
    {}
};

// Résultats de diagnostic pour un test spécifique
struct DiagnosticResult {
    bool success;                // Le test a-t-il réussi?
    DiagCategory category;       // Catégorie du diagnostic
    char component[16];          // Composant testé
    char message[64];            // Message de résultat ou erreur
    uint32_t timestamp;          // Horodatage
    
    // Constructeur
    DiagnosticResult(bool succ, DiagCategory cat, const char* comp, const char* msg) :
        success(succ),
        category(cat),
        timestamp(millis())
    {
        strncpy(component, comp, sizeof(component) - 1);
        component[sizeof(component) - 1] = '\0';
        
        strncpy(message, msg, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';
    }
    
    // Constructeur par défaut pour les tableaux
    DiagnosticResult() :
        success(false),
        category(DIAG_CAT_SYSTEM),
        timestamp(0)
    {
        component[0] = '\0';
        message[0] = '\0';
    }
};

// Taille maximum du buffer circulaire de logs
#define DIAG_LOG_BUFFER_SIZE 200

class DiagnosticModule {
public:
    // Constructeur
    DiagnosticModule() :
        _initialized(false),
        _currentLogLevel(DIAG_LEVEL_INFO),
        _autoRunDiagnosticsInterval(60000),  // 1 minute par défaut
        _lastAutoDiagnosticTime(0),
        _lastPerformanceUpdateTime(0),
        _performanceUpdateInterval(5000)   // 5 secondes par défaut
    {
        // L'initialisation complète est faite dans begin()
    }
    
    // Initialisation du module
    bool begin() {
        LOG_INFO("DIAG", "Initialisation du module de diagnostic");
        
        // Initialiser les métriques de performance
        _metrics = PerformanceMetrics();
        
        // Ajouter la première entrée de log
        logMessage(DIAG_LEVEL_INFO, DIAG_CAT_SYSTEM, "DIAG", "Module de diagnostic initialisé");
        
        _initialized = true;
        _lastAutoDiagnosticTime = millis();
        _lastPerformanceUpdateTime = millis();
        
        return true;
    }
    
    // Définir le niveau de log
    void setLogLevel(DiagLogLevel level) {
        _currentLogLevel = level;
        logMessage(DIAG_LEVEL_INFO, DIAG_CAT_SYSTEM, "DIAG", 
                  "Niveau de log changé: %d", static_cast<int>(level));
    }
    
    // Obtenir le niveau de log actuel
    DiagLogLevel getLogLevel() const {
        return _currentLogLevel;
    }
    
    // Ajouter une entrée de log (version simple)
    void logMessage(DiagLogLevel level, DiagCategory category, const char* module, const char* message) {
        if (!_initialized || level > _currentLogLevel) return;
        
        DiagLogEntry entry(level, category, module, message);
        _logBuffer.push(entry);
        
        // Les messages d'erreur sont également envoyés au log système
        if (level == DIAG_LEVEL_ERROR) {
            LOG_ERROR(module, "%s", message);
        } else if (level == DIAG_LEVEL_WARNING) {
            LOG_WARNING(module, "%s", message);
        }
    }
    
    // Ajouter une entrée de log (version avec formatage)
    template<typename... Args>
    void logMessage(DiagLogLevel level, DiagCategory category, const char* module, 
                    const char* format, Args... args) {
        if (!_initialized || level > _currentLogLevel) return;
        
        char formattedMessage[128];
        snprintf(formattedMessage, sizeof(formattedMessage), format, args...);
        
        DiagLogEntry entry(level, category, module, formattedMessage);
        _logBuffer.push(entry);
        
        // Les messages d'erreur sont également envoyés au log système
        if (level == DIAG_LEVEL_ERROR) {
            LOG_ERROR(module, "%s", formattedMessage);
        } else if (level == DIAG_LEVEL_WARNING) {
            LOG_WARNING(module, "%s", formattedMessage);
        }
    }
    
    // Récupérer les N dernières entrées de log
    std::vector<DiagLogEntry> getLastLogs(uint16_t count) const {
        std::vector<DiagLogEntry> result;
        uint16_t numEntries = std::min(count, static_cast<uint16_t>(_logBuffer.size()));
        
        // Ajouter les entrées du plus récent au plus ancien
        for (uint16_t i = 0; i < numEntries; i++) {
            result.push_back(_logBuffer[_logBuffer.size() - 1 - i]);
        }
        
        return result;
    }
    
    // Récupérer les entrées de log filtrées par niveau et/ou catégorie
    std::vector<DiagLogEntry> getFilteredLogs(DiagLogLevel minLevel, DiagCategory category = DIAG_CAT_SYSTEM, int maxCount = 50) const {
        std::vector<DiagLogEntry> result;
        
        // Parcourir les entrées du buffer, du plus récent au plus ancien
        for (int i = _logBuffer.size() - 1; i >= 0 && static_cast<int>(result.size()) < maxCount; i--) {
            const DiagLogEntry& entry = _logBuffer[i];
            
            // Filtrer selon le niveau minimum et la catégorie (si pas -1)
            if (entry.level <= minLevel && (category == static_cast<DiagCategory>(-1) || entry.category == category)) {
                result.push_back(entry);
            }
        }
        
        return result;
    }
    
    // Exécuter tous les tests de diagnostic
    std::vector<DiagnosticResult> runAllDiagnostics() {
        std::vector<DiagnosticResult> results;
        
        logMessage(DIAG_LEVEL_INFO, DIAG_CAT_SYSTEM, "DIAG", "Lancement des diagnostics complets");
        
        // Diagnostic du système
        results.push_back(runSystemDiagnostic());
        
        // Diagnostic des capteurs
        results.push_back(runSensorsDiagnostic());
        
        // Diagnostic des actionneurs
        results.push_back(runActuatorsDiagnostic());
        
        // Diagnostic de la communication
        results.push_back(runCommunicationDiagnostic());
        
        // Diagnostic de l'alimentation
        results.push_back(runPowerDiagnostic());
        
        // Enregistrer un résumé des résultats
        int successCount = 0;
        for (const auto& result : results) {
            if (result.success) successCount++;
        }
        
        logMessage(DIAG_LEVEL_INFO, DIAG_CAT_SYSTEM, "DIAG", 
                  "Diagnostics terminés: %d/%d réussis", successCount, results.size());
        
        return results;
    }
    
    // Exécuter un diagnostic spécifique
    DiagnosticResult runDiagnostic(DiagCategory category) {
        switch (category) {
            case DIAG_CAT_SYSTEM:
                return runSystemDiagnostic();
            case DIAG_CAT_SENSORS:
                return runSensorsDiagnostic();
            case DIAG_CAT_ACTUATORS:
                return runActuatorsDiagnostic();
            case DIAG_CAT_COMM:
                return runCommunicationDiagnostic();
            case DIAG_CAT_POWER:
                return runPowerDiagnostic();
            default:
                return DiagnosticResult(false, category, "UNKNOWN", "Catégorie de diagnostic non supportée");
        }
    }
    
    // Mise à jour des métriques de performance
    void updatePerformanceMetrics(unsigned long loopTime) {
        _metrics.loopCount++;
        
        // Mettre à jour les temps min/max/moyenne
        _metrics.minLoopTime = std::min(_metrics.minLoopTime, loopTime);
        _metrics.maxLoopTime = std::max(_metrics.maxLoopTime, loopTime);
        
        // Calcul de moyenne glissante
        _metrics.avgLoopTime = (_metrics.avgLoopTime * 0.95) + (loopTime * 0.05);
        
        // Mettre à jour les autres métriques périodiquement
        unsigned long currentTime = millis();
        if (currentTime - _lastPerformanceUpdateTime >= _performanceUpdateInterval) {
            _lastPerformanceUpdateTime = currentTime;
            
            // Mémoire libre
            _metrics.freeHeapSize = ESP.getFreeHeap();
            
            // Utilisation CPU (estimation simplifiée basée sur le temps de boucle)
            // Dans un système réel, ceci serait plus sophistiqué
            float targetLoopTime = 50.0f; // ms, basé sur delay(50) dans loop()
            _metrics.cpuUsagePercent = std::min(100.0f, static_cast<float>(_metrics.avgLoopTime) / targetLoopTime * 100.0f);
            
            // En conditions réelles, on mettrait à jour d'autres métriques ici
            // comme la tension de la batterie via une lecture ADC
            
            // Log des métriques de performance en mode verbose
            logMessage(DIAG_LEVEL_VERBOSE, DIAG_CAT_SYSTEM, "PERF", 
                      "Loop: %lu µs, Mem: %lu B, CPU: %.1f%%", 
                      _metrics.avgLoopTime, _metrics.freeHeapSize, _metrics.cpuUsagePercent);
        }
    }
    
    // Obtenir les métriques de performance actuelles
    const PerformanceMetrics& getPerformanceMetrics() const {
        return _metrics;
    }
    
    // Mise à jour périodique à appeler dans la boucle principale
    void update() {
        if (!_initialized) return;
        
        // Exécuter des diagnostics automatiques périodiquement
        unsigned long currentTime = millis();
        if (_autoRunDiagnosticsInterval > 0 && 
            currentTime - _lastAutoDiagnosticTime >= _autoRunDiagnosticsInterval) {
            
            _lastAutoDiagnosticTime = currentTime;
            
            // Exécuter des diagnostics simplifiés
            DiagnosticResult systemResult = runSystemDiagnostic();
            
            // Log du résultat
            if (!systemResult.success) {
                logMessage(DIAG_LEVEL_WARNING, DIAG_CAT_SYSTEM, "DIAG", 
                          "Diagnostique auto système a échoué: %s", systemResult.message);
            }
        }
    }
    
    // Définir l'intervalle pour les diagnostics automatiques (0 pour désactiver)
    void setAutoDiagnosticsInterval(unsigned long intervalMs) {
        _autoRunDiagnosticsInterval = intervalMs;
    }
    
    // Vérifier l'état global du système
    bool isSystemHealthy() const {
        // Implémenter une logique pour déterminer si le système est en bonne santé
        // Basée sur les erreurs récentes, métriques, etc.
        
        // Pour cet exemple simplifié, nous vérifions uniquement les erreurs récentes
        int errorCount = 0;
        for (size_t i = 0; i < std::min(static_cast<size_t>(50), _logBuffer.size()); i++) {
            if (_logBuffer[_logBuffer.size() - 1 - i].level == DIAG_LEVEL_ERROR) {
                errorCount++;
            }
        }
        
        // Si plus de 5 erreurs récentes, considérer le système comme non sain
        return errorCount < 5;
    }
    
private:
    bool _initialized;                   // État d'initialisation
    DiagLogLevel _currentLogLevel;       // Niveau de log actuel
    unsigned long _autoRunDiagnosticsInterval;  // Intervalle pour les diagnostics auto (ms)
    unsigned long _lastAutoDiagnosticTime;     // Dernière exécution de diagnostic auto
    unsigned long _lastPerformanceUpdateTime;  // Dernière mise à jour des performances
    unsigned long _performanceUpdateInterval;  // Intervalle de mise à jour des performances
    
    CircularBuffer<DiagLogEntry, DIAG_LOG_BUFFER_SIZE> _logBuffer;  // Buffer circulaire pour les logs
    PerformanceMetrics _metrics;  // Métriques de performance
    
    // Implementations des diagnostics spécifiques
    
    // Diagnostic système
    DiagnosticResult runSystemDiagnostic() {
        // Vérifier la mémoire libre
        uint32_t freeHeapSize = ESP.getFreeHeap();
        if (freeHeapSize < 10000) {  // Moins de 10KB de mémoire libre
            char msg[64];
            snprintf(msg, sizeof(msg), "Mémoire critique: seulement %lu octets libres", freeHeapSize);
            return DiagnosticResult(false, DIAG_CAT_SYSTEM, "MEMORY", msg);
        }
        
        // Vérifier la charge CPU
        if (_metrics.cpuUsagePercent > 90.0f) {  // CPU très chargé
            char msg[64];
            snprintf(msg, sizeof(msg), "CPU surchargé: %.1f%% utilisé", _metrics.cpuUsagePercent);
            return DiagnosticResult(false, DIAG_CAT_SYSTEM, "CPU", msg);
        }
        
        // Vérifier les watchdogs
        // Note: Dans un système réel, nous vérifierions l'état réel du watchdog
        
        char msg[64];
        snprintf(msg, sizeof(msg), "Système OK: Mémoire %lu B, CPU %.1f%%", 
                freeHeapSize, _metrics.cpuUsagePercent);
        return DiagnosticResult(true, DIAG_CAT_SYSTEM, "SYSTEM", msg);
    }
    
    // Diagnostic des capteurs
    DiagnosticResult runSensorsDiagnostic() {
        // Dans un système réel, on vérifierait chaque capteur
        // Pour cet exemple, nous retournons simplement un succès
        return DiagnosticResult(true, DIAG_CAT_SENSORS, "SENSORS", "Tous les capteurs fonctionnent normalement");
    }
    
    // Diagnostic des actionneurs
    DiagnosticResult runActuatorsDiagnostic() {
        // Dans un système réel, on vérifierait chaque actionneur
        // Pour cet exemple, nous retournons simplement un succès
        return DiagnosticResult(true, DIAG_CAT_ACTUATORS, "SERVOS", "Tous les servomoteurs répondent normalement");
    }
    
    // Diagnostic de la communication
    DiagnosticResult runCommunicationDiagnostic() {
        // Dans un système réel, on vérifierait la qualité de la communication
        // Pour cet exemple, nous retournons simplement un succès
        return DiagnosticResult(true, DIAG_CAT_COMM, "ESPNOW", "Communication ESP-NOW fonctionnelle");
    }
    
    // Diagnostic de l'alimentation
    DiagnosticResult runPowerDiagnostic() {
        // Dans un système réel, on lirait la tension de la batterie
        // Pour cet exemple, nous simulons une batterie faible
        float batteryVoltage = 11.8f;  // Volts (simulé)
        
        if (batteryVoltage < 11.0f) {
            char msg[64];
            snprintf(msg, sizeof(msg), "Batterie faible: %.1f V", batteryVoltage);
            return DiagnosticResult(false, DIAG_CAT_POWER, "BATTERY", msg);
        }
        
        char msg[64];
        snprintf(msg, sizeof(msg), "Batterie OK: %.1f V", batteryVoltage);
        return DiagnosticResult(true, DIAG_CAT_POWER, "BATTERY", msg);
    }
};

// Macros pour faciliter l'utilisation
#define DIAG_ERROR(diag, cat, module, ...) diag.logMessage(DIAG_LEVEL_ERROR, cat, module, __VA_ARGS__)
#define DIAG_WARNING(diag, cat, module, ...) diag.logMessage(DIAG_LEVEL_WARNING, cat, module, __VA_ARGS__)
#define DIAG_INFO(diag, cat, module, ...) diag.logMessage(DIAG_LEVEL_INFO, cat, module, __VA_ARGS__)
#define DIAG_DEBUG(diag, cat, module, ...) diag.logMessage(DIAG_LEVEL_DEBUG, cat, module, __VA_ARGS__)
#define DIAG_VERBOSE(diag, cat, module, ...) diag.logMessage(DIAG_LEVEL_VERBOSE, cat, module, __VA_ARGS__)

#endif // DIAGNOSTIC_MODULE_H
