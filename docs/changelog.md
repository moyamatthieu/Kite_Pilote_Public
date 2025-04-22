# Journal des Modifications du Projet Kite Pilote

Ce fichier sert de journal détaillé de toutes les modifications apportées au projet Kite Pilote.
Chaque entrée est horodatée avec précision, permettant de suivre l'évolution du projet.

## 2025-04-22

### 21:37:00 - Refactorisation de la fonction mettreAJourAffichage()

- **Catégorie**: Refactorisation
- **Action**: Division de la fonction `mettreAJourAffichage()` dans `src/main.cpp` en fonctions plus petites pour afficher les informations sur l'écran LCD.
- **Fichiers concernés**: `src/main.cpp`
- **Impact**: Amélioration de la lisibilité et de la maintenabilité du code.
- **Pourquoi**: La fonction `mettreAJourAffichage()` était trop longue et difficile à comprendre. La division en fonctions plus petites permet de mieux organiser le code et de faciliter la maintenance.

## 2025-04-22

### 21:37:00 - Refactorisation de la fonction envoyerHeartbeat()

- **Catégorie**: Refactorisation
- **Action**: Division de la fonction `envoyerHeartbeat()` dans `src/main.cpp` en fonctions plus petites pour mettre à jour l'état du système, lire la température du CPU et afficher le heartbeat.
- **Fichiers concernés**: `src/main.cpp`
- **Impact**: Amélioration de la lisibilité et de la maintenabilité du code.
- **Pourquoi**: La fonction `envoyerHeartbeat()` était trop longue et difficile à comprendre. La division en fonctions plus petites permet de mieux organiser le code et de faciliter la maintenance.

## 2025-04-22

### 21:36:00 - Amélioration de la gestion des erreurs et refactorisation de verifierCapteurs()

- **Catégorie**: Refactorisation, Amélioration
- **Action**: Division de la fonction `verifierCapteurs()` dans `src/main.cpp` en fonctions plus petites pour vérifier l'état de l'IMU, de la tension et des servos. Ajout d'une fonction `gererErreurCapteur()` pour gérer les erreurs de capteurs.
- **Fichiers concernés**: `src/main.cpp`
- **Impact**: Amélioration de la lisibilité et de la maintenabilité du code. Meilleure gestion des erreurs de capteurs.
- **Pourquoi**: La fonction `verifierCapteurs()` était trop longue et difficile à comprendre. La division en fonctions plus petites permet de mieux organiser le code et de faciliter la maintenance.

## 2025-04-22

### 21:32:00 - Refactorisation de la fonction initialiserSysteme()

- **Catégorie**: Refactorisation
- **Action**: Division de la fonction `initialiserSysteme()` dans `src/main.cpp` en fonctions plus petites pour chaque module (par exemple, `initialiserLEDs()`, `initialiserLCD()`, `initialiserCapteurs()`, etc.).
- **Fichiers concernés**: `src/main.cpp`
- **Impact**: Amélioration de la lisibilité et de la maintenabilité du code.
- **Pourquoi**: La fonction `initialiserSysteme()` était trop longue et difficile à comprendre. La division en fonctions plus petites permet de mieux organiser le code et de faciliter la maintenance.

## 2025-04-22

### 21:27:00 - Résolution du conflit de port 8180

- **Catégorie**: Bugfix
- **Action**: Identification et arrêt du processus (PID 726, "node") utilisant le port 8180, afin de permettre à Wokwi de configurer le port forwarding.
- **Impact**: Résolution du problème de port forwarding, permettant à Wokwi d'utiliser le port 8180.
- **Pourquoi**: Un autre processus utilisait déjà le port 8180, empêchant Wokwi de l'utiliser.

## 2025-04-22

### 21:24:00 - Exécution du script data_to_littlefs.py

- **Catégorie**: Configuration
- **Action**: Exécution du script `data_to_littlefs.py` pour automatiser la configuration de LittleFS dans `wokwi.toml`.
- **Fichiers concernés**: `wokwi.toml`, `data_to_littlefs.py`
- **Impact**: Mise à jour automatique de la section `[wokwi.littlefs]` dans `wokwi.toml` en fonction des fichiers présents dans le dossier `data`.
- **Pourquoi**: Automatisation de la configuration de LittleFS pour Wokwi, facilitant la gestion des fichiers inclus dans la simulation.

## 2025-04-22

### 21:22:00 - Correction de la syntaxe du fichier wokwi.toml

- **Catégorie**: Bugfix, Configuration
- **Correction**: Correction de la syntaxe incorrecte dans la section `[wokwi.littlefs]` du fichier `wokwi.toml`.
- **Fichiers concernés**: `wokwi.toml`
- **Impact**: Résolution de l'erreur de parsing du fichier `wokwi.toml`, permettant à la simulation Wokwi de fonctionner correctement.
- **Pourquoi**: La syntaxe incorrecte empêchait Wokwi de charger correctement la configuration du système de fichiers LittleFS.

## 2025-04-22

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
