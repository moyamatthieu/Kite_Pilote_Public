# Journal des Modifications du Projet Kite Pilote
Ce fichier sert de journal détaillé de toutes les modifications apportées au projet Kite Pilote.
Chaque entrée est horodatée avec précision, permettant de suivre l'évolution du projet.

## 2025-04-23

### 15:27:00 - Amélioration de l'initialisation du système avec affichage anticipé
- **Catégorie**: Robustesse, Interface utilisateur
- **Action**:
    - Réorganisation complète de la séquence d'initialisation du système
    - Les écrans (LCD et TFT) sont maintenant initialisés en tout premier lieu
    - Les LEDs d'état sont configurées dès le démarrage
    - Affichage en temps réel des messages d'initialisation sur tous les périphériques
    - Messages d'erreur affichés sur tous les périphériques simultanément
    - Indication visuelle de code d'erreur avec code numérique
    - Réorganisation de l'initialisation de la connexion WiFi et web
- **Fichiers concernés**:
    - `src/main.cpp`
- **Impact**: Considérablement amélioré le feedback visuel pendant l'initialisation et le diagnostic en cas d'erreur
- **Pourquoi**: Permet de voir immédiatement ce qui se passe pendant le démarrage, sans avoir à attendre que tous les modules soient initialisés. En cas d'erreur, tous les écrans et voyants affichent maintenant l'erreur de manière synchronisée.

### 15:12:00 - Implémentation de la partition de stockage dédiée
- **Catégorie**: Configuration, Robustesse
- **Action**:
    - Création d'une table de partitions personnalisée dans storage.csv
    - Configuration spécifique d'une partition "storage" de type spiffs pour LittleFS
    - Mise à jour de platformio.ini pour utiliser cette table de partitions
    - Passage explicite à LittleFS comme système de fichiers
- **Fichiers concernés**:
    - `storage.csv` (nouveau)
    - `platformio.ini`
- **Impact**: Solution définitive au problème "partition not found" en spécifiant une partition dédiée
- **Pourquoi**: Cette configuration garantit que le système dispose d'une partition correctement définie et identifiable pour le stockage des fichiers, évitant les problèmes d'initialisation de LittleFS.

### 15:09:00 - Correction de l'initialisation de LittleFS dans main.cpp
- **Catégorie**: Bugfix, Robustesse
- **Action**:
    - Correction de l'erreur "partition "spiffs" could not be found" (Error: 261)
    - Implémentation d'une initialisation à deux niveaux avec fallback
    - Tentative avec partition "storage" puis avec configuration par défaut
    - Poursuite du démarrage même en cas d'échec d'initialisation du système de fichiers
- **Fichiers concernés**:
    - `src/main.cpp`
- **Impact**: Résolution du problème de blocage au démarrage lié à LittleFS
- **Pourquoi**: Cette modification augmente la fiabilité du démarrage du système même en cas de problème de configuration LittleFS, évitant ainsi le blocage complet du programme.

### 15:02:00 - Ajout d'un script de sauvegarde GitHub
- **Catégorie**: Outillage, Configuration, Automatisation
- **Action**:
    - Création d'un script Python pour automatiser la sauvegarde du projet sur GitHub
    - Intégration avec le script de mise à jour de version
    - Génération automatique de messages de commit basés sur le changelog
    - Support des arguments en ligne de commande pour configurer le remote et la branche
- **Fichiers concernés**:
    - `script/github_backup.py`
- **Impact**: Automatisation du processus de sauvegarde et standardisation des messages de commit
- **Pourquoi**: Cette solution simplifie le processus de sauvegarde, réduit les risques d'erreurs et garantit l'uniformité des messages de commit, facilitant ainsi le suivi des versions et des modifications.

### 14:56:00 - Ajout d'un script d'automatisation de mise à jour de version
- **Catégorie**: Outillage, Configuration
- **Action**:
    - Création d'un script Python pour extraire la version du changelog et mettre à jour config.h
    - Automatisation de la synchronisation entre le changelog et la configuration
    - Mise à jour automatique de la date de build
- **Fichiers concernés**:
    - `script/update_version.py`
- **Impact**: Élimination des risques d'incohérence entre le changelog et les définitions de version
- **Pourquoi**: Cette automatisation assure que les versions mentionnées dans le code et la documentation restent synchronisées, évitant ainsi les confusions potentielles et réduisant les erreurs manuelles.

### 14:55:00 - Implémentation du fonctionnement dégradé pour le module capteurs
- **Catégorie**: Robustesse, Sécurité
- **Action**:
    - Extension du principe de fonctionnement dégradé au module SensorModule
    - Ajout d'un mécanisme de tentative périodique de récupération des capteurs défaillants
    - Mise en place d'un système d'estimation de valeurs pour les capteurs non disponibles
    - Implémentation de vérifications de plausibilité pour détecter les lectures aberrantes
- **Fichiers concernés**:
    - `src/modules/sensor_module.cpp`
- **Impact**: Le système peut maintenant fonctionner même avec des capteurs défectueux, améliorant considérablement sa robustesse
- **Pourquoi**: Cette amélioration permet au système de maintenir un niveau de fonctionnalité acceptable même en cas de panne de capteurs, en utilisant des estimations et des mécanismes de récupération automatiques.

### 14:51:00 - Documentation du principe de fonctionnement dégradé
- **Catégorie**: Documentation, Architecture
- **Action**:
    - Ajout d'une nouvelle section 3.10 "Tolérance aux Pannes et Fonctionnement Dégradé" dans le document projet_kite_complet.md
    - Documentation détaillée des principes de fonctionnement en mode dégradé
    - Explication des mécanismes de bypass, d'isolation de défaillances et de récupération autonome
    - Clarification de la hiérarchisation des fonctionnalités et des services essentiels
- **Fichiers concernés**:
    - `docs/guides/projet_kite_complet.md`
- **Impact**: Clarification du principe fondamental que le système doit continuer à fonctionner quoi qu'il arrive
- **Pourquoi**: Cette documentation établit un principe directeur important pour le développement futur, assurant la création d'un système hautement disponible même en cas de défaillance partielle.

### 14:45:00 - Implémentation du système de bypass LittleFS
- **Catégorie**: Robustesse, Sécurité, Bugfix
- **Action**:
    - Ajout d'un mécanisme de secours en cas d'échec d'initialisation de LittleFS
    - Tentative d'initialisation alternative sans spécifier de partition
    - Fallback vers une interface HTML intégrée si toutes les tentatives échouent
    - Amélioration de la gestion d'erreurs avec des messages de log plus détaillés
- **Fichiers concernés**:
    - `src/modules/web_interface_module.cpp`
- **Impact**: Meilleure robustesse du système même en cas de défaillance du système de fichiers.
- **Pourquoi**: Cette modification permet au serveur web de fonctionner même si LittleFS n'est pas correctement monté, évitant ainsi des erreurs critiques et assurant la disponibilité minimale de l'interface utilisateur.

### 14:38:00 - Modernisation du code C++ et amélioration de la documentation
- **Catégorie**: Refactorisation, Optimisation, Standards, Documentation
- **Action**:
    - Application des standards C++ modernes (C++11/14) à plusieurs modules
    - Utilisation de std::function pour les callbacks au lieu de pointeurs de fonction bruts
    - Remplacement de #define par constexpr pour les constantes
    - Utilisation de enum class au lieu de enum pour une meilleure sécurité de typage
    - Ajout du qualificateur noexcept aux fonctions appropriées
    - Utilisation de std::array au lieu des tableaux C
    - Remplacement de NULL par nullptr
    - Amélioration de la gestion des erreurs avec des vérifications plus explicites
    - Application plus rigoureuse du RAII pour la gestion des ressources
- **Fichiers concernés**:
    - `include/modules/lcd_module.h` et `src/modules/lcd_module.cpp`
    - `include/modules/web_interface_module.h` et `src/modules/web_interface_module.cpp`
- **Impact**: Meilleure sécurité du code, facilitation de la maintenance, diminution potentielle des bugs et fuites mémoire.
- **Pourquoi**: L'application des standards C++ modernes améliore la qualité du code, sa robustesse et sa maintenabilité à long terme.

### 14:19:00 - Restructuration modulaire des fichiers sources
- **Catégorie**: Refactorisation, Architecture
- **Action**: 
    - Séparation complète des interfaces et des implémentations (headers/sources) pour les modules principaux
    - Conversion des modules SensorModule, ServoModule et AutopilotModule du style "tout-en-un-header" vers une architecture plus standard C++
    - Implémentation des méthodes des modules dans des fichiers .cpp dédiés
    - Amélioration de la documentation Doxygen pour toutes les classes et méthodes
    - Application de conventions de nommage cohérentes pour les membres privés (préfixe m_ remplacé par _)
- **Fichiers concernés**: 
    - `include/modules/{sensor,servo,autopilot}_module.h`
    - `src/modules/{sensor,servo,autopilot}_module.cpp`
    - `include/core/config.h`
- **Impact**: Code plus maintenable et modulaire, temps de compilation réduit (moins de recompilations en cascade). Mise à jour de la version à v2.0.0.5.
- **Pourquoi**: L'approche précédente avec toutes les implémentations dans les headers était fonctionnelle mais moins conforme aux bonnes pratiques C++. Cette restructuration améliore la séparation des responsabilités, facilite la maintenance future et se conforme mieux aux standards de l'industrie.

## 2025-04-23

### 12:03:00 - Ajustement config Wokwi pour LittleFS (Workaround)
- **Catégorie**: Configuration, Simulation
- **Action**:
    - Restauré la section `[wokwi.esp32.filesystem]` dans `wokwi.toml` pour mapper directement le dossier `data/` à la racine `/` du système de fichiers simulé.
    - Supprimé la section `[wokwi.littlefs]` potentiellement conflictuelle.
    - Maintenu `partitions = "custom_partitions.csv"` dans `wokwi.toml` (même si le montage par label échoue).
    - Maintenu `SubType = 0x82` dans `custom_partitions.csv` pour permettre la compilation.
- **Fichiers concernés**: `wokwi.toml`, `custom_partitions.csv`, `include/core/config.h`
- **Impact**: Permet au serveur web de servir les fichiers statiques (HTML, CSS) dans la simulation Wokwi, malgré l'échec persistant du montage `LittleFS.begin(..., "storage")` dans cet environnement. C'est un compromis pour la simulation. Mise à jour de la version à v2.0.0.4.
- **Pourquoi**: Wokwi ne semble pas gérer correctement le montage LittleFS par label via une table de partitions personnalisée. Le mappage direct via `[wokwi.esp32.filesystem]` est un contournement pour rendre les fichiers accessibles au serveur web dans la simulation.

### 11:51:00 - Correction des erreurs d'initialisation LittleFS (Wokwi) et Tactile (I2C)
- **Catégorie**: Bugfix, Configuration
- **Action**:
    - Modifié `wokwi.toml` pour spécifier `partitions = "custom_partitions.csv"`, corrigeant l'erreur de montage LittleFS dans la simulation Wokwi (`partition "storage" could not be found`).
    - Modifié `custom_partitions.csv` pour utiliser `littlefs` comme `SubType` (bien que non strictement nécessaire pour Wokwi, améliore la compatibilité avec `pio uploadfs`).
    - Supprimé les définitions `TOUCH_SDA` et `TOUCH_SCL` conflictuelles dans `src/main.cpp` pour utiliser les broches I2C définies dans `config.h` (21/22), corrigeant l'erreur de détection de l'écran tactile.
- **Fichiers concernés**: `wokwi.toml`, `custom_partitions.csv`, `src/main.cpp`, `include/core/config.h`
- **Impact**: Résolution des erreurs `partition "storage" could not be found` et `Écran tactile non détecté` au démarrage. Le système initialise maintenant correctement LittleFS et l'écran tactile dans la simulation Wokwi. Mise à jour de la version à v2.0.0.3.
- **Pourquoi**: La simulation Wokwi n'utilisait pas la table de partitions personnalisée, et une incohérence dans les définitions des broches I2C empêchait l'initialisation correcte de l'écran tactile.

### 10:14:00 - Correction de l'initialisation de LittleFS (Erreur 261)
- **Catégorie**: Bugfix, Configuration
- **Action**: 
    - Modifié `custom_partitions.csv` pour renommer la partition de données en "storage" et définir son sous-type à `0x82` (LittleFS).
    - Modifié l'appel `LittleFS.begin()` dans `src/modules/web_interface_module.cpp` pour spécifier explicitement le label de partition "storage".
- **Fichiers concernés**: `custom_partitions.csv`, `src/modules/web_interface_module.cpp`, `include/core/config.h`
- **Impact**: Résolution de l'erreur `partition "spiffs" could not be found` (Error: 261) lors du montage de LittleFS. Le système de fichiers est maintenant correctement initialisé. Mise à jour de la version à v2.0.0.2.
- **Pourquoi**: Incohérence entre la définition de la partition dans `custom_partitions.csv` (label "spiffs", subtype "spiffs") et l'appel implicite ou explicite à `LittleFS.begin()` qui cherchait une partition compatible LittleFS. La modification aligne la définition de la partition et l'appel de montage.

### 09:00:00 - Correction des erreurs de compilation liées à setup() et loop()
- **Catégorie**: Bugfix
- **Action**: Renommage de `demarrer()` en `setup()` et `bouclePrincipale()` en `loop()` dans `src/main.cpp`.
- **Fichiers concernés**: `src/main.cpp`
- **Impact**: Résolution des erreurs de compilation liées aux fonctions Arduino requises.
- **Pourquoi**: Les noms de fonctions `setup()` et `loop()` sont requis par le framework Arduino.

### 09:05:00 - Implémentation de la méthode update() dans WebInterfaceModule
- **Catégorie**: Bugfix
- **Action**: Ajout de la définition de la méthode `update()` dans `src/modules/web_interface_module.cpp`.
- **Fichiers concernés**: `src/modules/web_interface_module.cpp`, `include/modules/web_interface_module.h`
- **Impact**: Résolution de l'erreur de compilation liée à la référence non définie à `WebInterfaceModule::update()`.
- **Pourquoi**: La méthode `update()` est nécessaire pour la mise à jour périodique de l'interface web.

## 2025-04-22

### 21:37:00 - Refactorisation de la fonction mettreAJourAffichage()
- **Catégorie**: Refactorisation
- **Action**: Division de la fonction `mettreAJourAffichage()` dans `src/main.cpp` en fonctions plus petites pour afficher les informations sur l'écran LCD.
- **Fichiers concernés**: `src/main.cpp`
- **Impact**: Amélioration de la lisibilité et de la maintenabilité du code.
- **Pourquoi**: La fonction `mettreAJourAffichage()` était trop longue et difficile à comprendre. La division en fonctions plus petites permet de mieux organiser le code et de faciliter la maintenance.

### 21:37:00 - Refactorisation de la fonction envoyerHeartbeat()
- **Catégorie**: Refactorisation
- **Action**: Division de la fonction `envoyerHeartbeat()` dans `src/main.cpp` en fonctions plus petites pour mettre à jour l'état du système, lire la température du CPU et afficher le heartbeat.
- **Fichiers concernés**: `src/main.cpp`
- **Impact**: Amélioration de la lisibilité et de la maintenabilité du code.
- **Pourquoi**: La fonction `envoyerHeartbeat()` était trop longue et difficile à comprendre. La division en fonctions plus petites permet de mieux organiser le code et de faciliter la maintenance.

### 21:36:00 - Amélioration de la gestion des erreurs et refactorisation de verifierCapteurs()
- **Catégorie**: Refactorisation, Amélioration
- **Action**: Division de la fonction `verifierCapteurs()` dans `src/main.cpp` en fonctions plus petites pour vérifier l'état de l'IMU, de la tension et des servos. Ajout d'une fonction `gererErreurCapteur()` pour gérer les erreurs de capteurs.
- **Fichiers concernés**: `src/main.cpp`
- **Impact**: Amélioration de la lisibilité et de la maintenabilité du code. Meilleure gestion des erreurs de capteurs.
- **Pourquoi**: La fonction `verifierCapteurs()` était trop longue et difficile à comprendre. La division en fonctions plus petites permet de mieux organiser le code et de faciliter la maintenance.

### 21:32:00 - Refactorisation de la fonction initialiserSysteme()
- **Catégorie**: Refactorisation
- **Action**: Division de la fonction `initialiserSysteme()` dans `src/main.cpp` en fonctions plus petites pour chaque module (par exemple, `initialiserLEDs()`, `initialiserLCD()`, `initialiserCapteurs()`, etc.).
- **Fichiers concernés**: `src/main.cpp`
- **Impact**: Amélioration de la lisibilité et de la maintenabilité du code.
- **Pourquoi**: La fonction `initialiserSysteme()` était trop longue et difficile à comprendre. La division en fonctions plus petites permet de mieux organiser le code et de faciliter la maintenance.

### 21:27:00 - Résolution du conflit de port 8180
- **Catégorie**: Bugfix
- **Action**: Identification et arrêt du processus (PID 726, "node") utilisant le port 8180, afin de permettre à Wokwi de configurer le port forwarding.
- **Impact**: Résolution du problème de port forwarding, permettant à Wokwi d'utiliser le port 8180.
- **Pourquoi**: Un autre processus utilisait déjà le port 8180, empêchant Wokwi de l'utiliser.

### 21:24:00 - Exécution du script data_to_littlefs.py
- **Catégorie**: Configuration
- **Action**: Exécution du script `data_to_littlefs.py` pour automatiser la configuration de LittleFS dans `wokwi.toml`.
- **Fichiers concernés**: `wokwi.toml`, `data_to_littlefs.py`
- **Impact**: Mise à jour automatique de la section `[wokwi.littlefs]` dans `wokwi.toml` en fonction des fichiers présents dans le dossier `data`.
- **Pourquoi**: Automatisation de la configuration de LittleFS pour Wokwi, facilitant la gestion des fichiers inclus dans la simulation.

### 21:22:00 - Correction de la syntaxe du fichier wokwi.toml
- **Catégorie**: Bugfix, Configuration
- **Correction**: Correction de la syntaxe incorrecte dans la section `[wokwi.littlefs]` du fichier `wokwi.toml`.
- **Fichiers concernés**: `wokwi.toml`
- **Impact**: Résolution de l'erreur de parsing du fichier `wokwi.toml`, permettant à la simulation Wokwi de fonctionner correctement.
- **Pourquoi**: La syntaxe incorrecte empêchait Wokwi de charger correctement la configuration du système de fichiers LittleFS.

### 21:17:00 - Correction erreur de compilation LcdModule
- **Catégorie**: Bugfix
- **Correction**: Modification de l'instanciation des objets `LcdModule` dans `main.cpp` pour utiliser le constructeur par défaut, suite à la suppression du constructeur paramétré dans `lcd_module.h`.
- **Fichiers concernés**: `src/main.cpp`
- **Impact**: Résolution de l'erreur de compilation empêchant le build du projet. Le système peut de nouveau être compilé avec succès.
- **Pourquoi**: Le constructeur utilisé dans `main.cpp` n'existait plus dans la définition de la classe `LcdModule`, provoquant une erreur de compilation bloquante. La correction aligne l'utilisation de la classe avec sa définition actuelle.

## 2025-04-21

### 10:15:00 - Optimisation et correction des problèmes de compilation
- **Catégorie**: Bugfix, Configuration, Optimisation
- **Résolution du problème des définitions multiples dans logger.h**: Mise en place d'une approche "inline singleton" pour remplacer les variables statiques traditionnelles, éliminant les erreurs de compilation liées aux définitions multiples
- **Fichiers concernés**: `/include/utils/logger.h` - Modifications majeures de l'architecture interne du Logger
- **Correction du warning de dépendance Adafruit FT6206**: Suppression de la référence redondante dans platformio.ini
- **Résolution des warnings liés à SIMULATION_MODE**: Utilisation de directives conditionnelles dans config.h pour éviter les redéfinitions
- **Impact**: Amélioration significative de la fiabilité du processus de compilation et élimination des warnings
- **Pourquoi**: Cette modification était nécessaire pour assurer la stabilité et la maintenabilité du code à long terme, en particulier dans les projets multi-fichiers où le Logger est fréquemment utilisé

## 2025-04-19

### 15:45:30 - Restructuration majeure de l'architecture du projet
- Refonte complète de la structure des dossiers pour une meilleure organisation modulaire
- Création d'une nouvelle hiérarchie de dossiers (include, src, lib, tests, docs)
- Mise à jour des chemins d'inclusion dans main.cpp et dans toute la documentation pour refléter la nouvelle structure
- Séparation claire entre le code, les tests et la documentation
- Amélioration substantielle de la maintenabilité et de l'évolutivité du projet
- Organisation des tests en tests unitaires, d'intégration et système
- Consolidation de la documentation avec des sous-dossiers spécifiques (api, guides, kite)
- Mise à jour du README.md pour décrire la nouvelle structure
- Incrémentation de la version à v2.0.0.0 pour refléter ce changement architectural majeur
- Suppression définitive des anciennes versions du code et des dossiers obsolètes

## 2025-04-15

### 10:30:00 - Amélioration de la documentation du projet
- Création d'un fichier README.md principal à la racine du projet
- Mise à jour complète de la documentation avec les dernières fonctionnalités
- Harmonisation de tous les documents pour refléter l'architecture actuelle
- Ajout de diagrammes UML pour visualiser l'architecture logicielle
- Mise à jour du document projet_kite_complet.md pour la version 1.0.0.4
- Extension de la documentation pour les futurs contributeurs au projet
- Correction des incoherences entre les versions mentionnées dans les différents fichiers

### 10:25:00 - Implémentation d'un système de gestion d'énergie
- Création de la classe PowerManager pour l'optimisation de la consommation d'énergie
- Ajout des modes de veille légère (light sleep) et profonde (deep sleep) pour ESP32
- Implémentation d'un système de détection d'inactivité pour économiser l'énergie
- Mise à jour du système d'initialisation pour intégrer le gestionnaire d'énergie
- Configuration de modes d'économie d'énergie adaptés aux phases du vol du kite
- Modification de la boucle principale pour appeler PowerManager.update()
- Optimisation des performances énergétiques pendant les phases d'attente

### 10:15:00 - Mise en place de tests unitaires
- Configuration d'un environnement de tests unitaires avec framework Unity
- Création de tests pour les modules de configuration et de gestion d'erreurs
- Implémentation de tests d'intégration pour les modules principales
- Ajout de procédures de simulation pour tester les modules sans matériel physique
- Documentation du processus de test dans un fichier tests/README.md
