/*
 * Système de journalisation (Logger) pour le projet Kite Pilote
 * 
 * Module avancé pour l'enregistrement et l'affichage des messages de débogage.
 * Fournit différents niveaux de détail (ERROR, WARNING, INFO, DEBUG, VERBOSE)
 * et une capacité de stockage en mémoire avec buffer circulaire.
 * 
 * Permet le diagnostic efficace du système et facilite le débogage.
 * 
 * Créé le: 17/04/2025
 * Mis à jour le: 19/04/2025
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include "../core/config.h"

// Structure pour stocker une entrée de log
struct LogEntry {
    uint32_t timestamp;  // Horodatage (ms depuis le démarrage)
    uint8_t level;       // Niveau de log (ERROR, WARNING, etc.)
    char module[16];     // Module source (nom court, max 15 caractères)
    char message[128];   // Message (tronqué si plus long)
    
    LogEntry() : timestamp(0), level(0) {
        module[0] = '\0';
        message[0] = '\0';
    }
};

// Niveaux de journalisation
enum LogLevel {
    LL_NONE = 0,    // Aucun message (désactivé)
    LL_ERROR = 1,   // Erreurs critiques seulement
    LL_WARNING = 2, // Erreurs et avertissements
    LL_INFO = 3,    // Informations générales (par défaut)
    LL_DEBUG = 4,   // Messages détaillés pour débogage
    LL_VERBOSE = 5  // Messages très détaillés (développement)
};

// Classe Logger améliorée avec buffer circulaire
class Logger {
public:
    // Taille du buffer circulaire (nombre d'entrées de log)
    static const uint16_t BUFFER_SIZE = 
        #ifdef LOG_BUFFER_SIZE
            LOG_BUFFER_SIZE
        #else
            100  // Valeur par défaut si non définie
        #endif
    ;
    
    // Initialisation du logger avec options avancées
    static void begin(LogLevel level = static_cast<LogLevel>(LL_INFO), bool printToSerial = true) {
        _logLevel() = level;
        _bufferIndex() = 0;
        _bufferFull() = false;
        memset(_logBuffer(), 0, sizeof(LogEntry) * BUFFER_SIZE);
        
        _printToSerial() = printToSerial;
        
        if (_printToSerial()) {
            Serial.println(F("=== Système de journalisation initialisé ==="));
            Serial.print(F("Niveau de log: "));
            Serial.println(logLevelToString(level));
            Serial.print(F("Taille du buffer: "));
            Serial.print(BUFFER_SIZE);
            Serial.println(F(" entrées"));
        }
    }
    
    // Définir le niveau de journalisation
    static void setLogLevel(LogLevel level) {
        _logLevel() = level;
        info("LOGGER", "Niveau de journalisation défini à %s", logLevelToString(level));
    }
    
    // Obtenir le niveau actuel de journalisation
    static LogLevel getLogLevel() {
        return _logLevel();
    }
    
    // Fonctions de journalisation pour différents niveaux
    static void error(const char* module, const char* message) {
        if (_logLevel() >= static_cast<LogLevel>(LL_ERROR)) {
            log(static_cast<LogLevel>(LL_ERROR), module, message);
        }
    }
    
    static void warning(const char* module, const char* message) {
        if (_logLevel() >= static_cast<LogLevel>(LL_WARNING)) {
            log(static_cast<LogLevel>(LL_WARNING), module, message);
        }
    }
    
    static void info(const char* module, const char* message) {
        if (_logLevel() >= static_cast<LogLevel>(LL_INFO)) {
            log(static_cast<LogLevel>(LL_INFO), module, message);
        }
    }
    
    static void debug(const char* module, const char* message) {
        if (_logLevel() >= static_cast<LogLevel>(LL_DEBUG)) {
            log(static_cast<LogLevel>(LL_DEBUG), module, message);
        }
    }
    
    static void verbose(const char* module, const char* message) {
        if (_logLevel() >= static_cast<LogLevel>(LL_VERBOSE)) {
            log(static_cast<LogLevel>(LL_VERBOSE), module, message);
        }
    }
    
    // Fonctions avec formatage (comme printf)
    template<typename... Args>
    static void error(const char* module, const char* format, Args... args) {
        if (_logLevel() >= static_cast<LogLevel>(LL_ERROR)) {
            formatAndLog(static_cast<LogLevel>(LL_ERROR), module, format, args...);
        }
    }
    
    template<typename... Args>
    static void warning(const char* module, const char* format, Args... args) {
        if (_logLevel() >= static_cast<LogLevel>(LL_WARNING)) {
            formatAndLog(static_cast<LogLevel>(LL_WARNING), module, format, args...);
        }
    }
    
    template<typename... Args>
    static void info(const char* module, const char* format, Args... args) {
        if (_logLevel() >= static_cast<LogLevel>(LL_INFO)) {
            formatAndLog(static_cast<LogLevel>(LL_INFO), module, format, args...);
        }
    }
    
    template<typename... Args>
    static void debug(const char* module, const char* format, Args... args) {
        if (_logLevel() >= static_cast<LogLevel>(LL_DEBUG)) {
            formatAndLog(static_cast<LogLevel>(LL_DEBUG), module, format, args...);
        }
    }
    
    template<typename... Args>
    static void verbose(const char* module, const char* format, Args... args) {
        if (_logLevel() >= static_cast<LogLevel>(LL_VERBOSE)) {
            formatAndLog(static_cast<LogLevel>(LL_VERBOSE), module, format, args...);
        }
    }
    
    // Obtenir le nombre total d'entrées de log dans le buffer
    static uint16_t getLogCount() {
        return _bufferFull() ? BUFFER_SIZE : _bufferIndex();
    }
    
    // Obtenir une entrée de log spécifique par index
    static const LogEntry* getLogEntry(uint16_t index) {
        if (index >= getLogCount()) {
            return nullptr;
        }
        
        uint16_t actualIndex;
        if (_bufferFull()) {
            // Buffer circulaire rempli, calculer l'index réel
            actualIndex = (_bufferIndex() + index) % BUFFER_SIZE;
        } else {
            // Buffer pas encore rempli
            actualIndex = index;
        }
        
        return &_logBuffer()[actualIndex];
    }
    
    // Exporter les logs vers la console série
    static void exportLogs(uint16_t count = 0, LogLevel minLevel = static_cast<LogLevel>(LL_ERROR)) {
        uint16_t totalLogs = getLogCount();
        uint16_t logsToExport = (count == 0 || count > totalLogs) ? totalLogs : count;
        
        Serial.println(F("=== Exportation des logs ==="));
        Serial.print(F("Total: "));
        Serial.print(logsToExport);
        Serial.println(F(" entrées"));
        Serial.println(F("Timestamp [Niveau] Module: Message"));
        Serial.println(F("------------------------------------------"));
        
        for (uint16_t i = 0; i < logsToExport; i++) {
            const LogEntry* entry = getLogEntry(i);
            if (entry && entry->level >= minLevel) {
                printLogEntry(*entry);
            }
        }
        
        Serial.println(F("------------------------------------------"));
        Serial.println(F("Fin des logs"));
    }
    
    // Vider le buffer de logs
    static void clearLogs() {
        _bufferIndex() = 0;
        _bufferFull() = false;
        memset(_logBuffer(), 0, sizeof(LogEntry) * BUFFER_SIZE);
        info("LOGGER", "Buffer de logs vidé");
    }
    
    // Vérifier si un log existe avec un motif spécifique
    static bool findLogContaining(const char* pattern, LogLevel minLevel = static_cast<LogLevel>(LL_ERROR)) {
        uint16_t totalLogs = getLogCount();
        
        for (uint16_t i = 0; i < totalLogs; i++) {
            const LogEntry* entry = getLogEntry(i);
            if (entry && entry->level >= minLevel && strstr(entry->message, pattern) != nullptr) {
                return true;
            }
        }
        
        return false;
    }
    
    // Formater un horodatage en format lisible
    static void formatTimestamp(uint32_t timestamp, char* buffer, size_t bufferSize) {
        uint32_t seconds = timestamp / 1000;
        uint16_t millis = timestamp % 1000;
        uint8_t minutes = (seconds / 60) % 60;
        uint8_t hours = (seconds / 3600) % 24;
        seconds %= 60;
        
        snprintf(buffer, bufferSize, "%02d:%02d:%02d.%03d", hours, minutes, seconds, millis);
    }
    
    // Activer/désactiver l'affichage sur le port série
    static void setPrintToSerial(bool enabled) {
        _printToSerial() = enabled;
    }
    
private:
    // Utilisation d'une approche "inline singleton" pour éviter les définitions multiples
    static LogLevel& _getLogLevel() {
        static LogLevel level = LL_INFO;
        return level;
    }
    
    static LogEntry* _getLogBuffer() {
        static LogEntry buffer[BUFFER_SIZE];
        return buffer;
    }
    
    static uint16_t& _getBufferIndex() {
        static uint16_t index = 0;
        return index;
    }
    
    static bool& _getBufferFull() {
        static bool full = false;
        return full;
    }
    
    static bool& _getPrintToSerial() {
        static bool print = true;
        return print;
    }
    
    // Accesseurs pour les variables statiques
    static LogLevel& _logLevel() { return _getLogLevel(); }
    static LogEntry* _logBuffer() { return _getLogBuffer(); }
    static uint16_t& _bufferIndex() { return _getBufferIndex(); }
    static bool& _bufferFull() { return _getBufferFull(); }
    static bool& _printToSerial() { return _getPrintToSerial(); }
    
    // Fonction interne d'affichage et stockage
    static void log(LogLevel level, const char* module, const char* message) {
        uint32_t timestamp = millis();
        
        // Stocker dans le buffer circulaire
        LogEntry& entry = _logBuffer()[_bufferIndex()];
        entry.timestamp = timestamp;
        entry.level = level;
        
        // Copier et tronquer le module si nécessaire
        strncpy(entry.module, module, sizeof(entry.module) - 1);
        entry.module[sizeof(entry.module) - 1] = '\0';
        
        // Copier et tronquer le message si nécessaire
        strncpy(entry.message, message, sizeof(entry.message) - 1);
        entry.message[sizeof(entry.message) - 1] = '\0';
        
        // Avancer dans le buffer circulaire
        _bufferIndex() = (_bufferIndex() + 1) % BUFFER_SIZE;
        if (_bufferIndex() == 0) {
            _bufferFull() = true;
        }
        
        // Afficher sur la console série pour ERROR et WARNING ou si on est en mode debugging
        if (_printToSerial() && (level <= LL_WARNING || _logLevel() >= LL_DEBUG)) {
            printLogEntry(entry);
        }
    }
    
    // Affiche une entrée de log sur la console série
    static void printLogEntry(const LogEntry& entry) {
        Serial.print(entry.timestamp);
        Serial.print(F(" ["));
        Serial.print(logLevelToString(static_cast<LogLevel>(entry.level)));
        Serial.print(F("] "));
        Serial.print(entry.module);
        Serial.print(F(": "));
        Serial.println(entry.message);
    }
    
    // Formatage avancé avec paramètres variables
    template<typename... Args>
    static void formatAndLog(LogLevel level, const char* module, const char* format, Args... args) {
        char buffer[128]; // Tampon pour le message formaté
        snprintf(buffer, sizeof(buffer), format, args...);
        log(level, module, buffer);
    }
    
    // Convertir un niveau de log en chaîne
    static const char* logLevelToString(LogLevel level) {
        switch (level) {
            case LL_NONE: return "NONE";
            case LL_ERROR: return "ERROR";
            case LL_WARNING: return "WARNING";
            case LL_INFO: return "INFO";
            case LL_DEBUG: return "DEBUG";
            case LL_VERBOSE: return "VERBOSE";
            default: return "UNKNOWN";
        }
    }
};

// Les variables statiques sont maintenant initialisées dans leurs fonctions d'accès respectives
// Aucune initialisation supplémentaire n'est nécessaire ici

// Macros pour faciliter l'utilisation
#define LOG_ERROR(module, ...) Logger::error(module, __VA_ARGS__)
#define LOG_WARNING(module, ...) Logger::warning(module, __VA_ARGS__)
#define LOG_INFO(module, ...) Logger::info(module, __VA_ARGS__)
#define LOG_DEBUG(module, ...) Logger::debug(module, __VA_ARGS__)
#define LOG_VERBOSE(module, ...) Logger::verbose(module, __VA_ARGS__)

#endif // LOGGER_H
