# Projet Autopilote de Kite Générateur d'Électricité

*Version: 1.0.0.4 — Dernière mise à jour: 15 Avril 2025, 10:30:00*

## Guide de Navigation Rapide
- **[Vue d'ensemble](#introduction)** - Présentation du projet et schéma global
- **[Matériel](#2-composants-matériels-clés)** - ESP32, servomoteurs, capteurs, interfaces
- **[Logiciel](#3-fonctionnalités-logicielles-clés)** - Autopilote, contrôle, interfaces utilisateur
- **[Architecture](#10-architecture-des-fichiers-du-projet)** - Structure des fichiers et organisation du code
- **[Débogage](#11-systèmes-de-journalisation-diagnostic-et-terminal-distant)** - Logs, diagnostics et terminal

## Table des matières détaillée
- [Introduction](#introduction)
- [1. Objectif Global du Projet](#1-objectif-global-du-projet)
- [2. Composants Matériels Clés](#2-composants-matériels-clés)
  - [2.1 Contrôleur Principal](#21-contrôleur-principal)
  - [2.2 Actionneurs](#22-actionneurs)
    - [2.2.1 Servo 1 : Direction](#221-servo-1--direction)
    - [2.2.2 Servo 2 : Trim](#222-servo-2--trim)
    - [2.2.3 Servo 3 : Générateur/Treuil](#223-servo-3--générateur-treuil)
  - [2.3 Capteurs](#23-capteurs)
    - [2.3.1 IMU](#231-imu-unité-de-mesure-inertielle)
    - [2.3.2 Capteur de Tension de Ligne](#232-capteur-de-tension-de-ligne)
    - [2.3.3 Capteur de Longueur de Ligne](#233-capteur-de-longueur-de-ligne)
    - [2.3.4 Anémomètre/Girouette](#234-anémomètregirouette-optionnel-mais-recommandé)
  - [2.4 Interfaces Utilisateur](#24-interfaces-utilisateur)
  - [2.5 Alimentation](#25-alimentation)
  - [2.6 Architecture des ESP32](#26-architecture-des-esp32)
- [3. Fonctionnalités Logicielles Clés](#3-fonctionnalités-logicielles-clés)
  - [3.1 Autopilote](#31-autopilote)
  - [3.2 Gestion de la Génération d'Énergie](#32-gestion-de-la-génération-dénergie)
  - [3.3 Gestion de l'Enroulement/Déroulement](#33-gestion-de-lenroulementdéroulement)
  - [3.4 Modes Opérationnels](#34-modes-opérationnels)
  - [3.5 Interface Web](#35-interface-web)
  - [3.6 Affichage LCD](#36-affichage-lcd)
  - [3.7 Système de Diagnostic Avancé](#37-système-de-diagnostic-avancé)
  - [3.8 Mode Simulation](#38-mode-simulation)
  - [3.9 Sécurité Intégrée](#39-sécurité-intégrée)
- [4. Points Problématiques du Projet](#4-points-problématiques-du-projet)
- [5. Idées Additionnelles et Améliorations](#5-idées-additionnelles-et-améliorations)
- [6. Attentes vis-à-vis de l'IA pour le Développement](#6-attentes-vis-à-vis-de-lia-pour-le-développement)
- [7. Défis Anticipés et Solutions](#7-défis-anticipés-et-solutions)
- [8. Conclusion et Prochaines Étapes](#8-conclusion-et-prochaines-étapes)
- [9. Index des Composants Clés](#9-index-des-composants-clés)
- [10. Architecture des Fichiers du Projet](#10-architecture-des-fichiers-du-projet)
  - [10.1 Structure Globale](#101-structure-globale)
  - [10.2 Architecture Actuelle](#102-architecture-actuelle)
  - [10.3 Flux de Données et Interactions](#103-flux-de-données-et-interactions)
- [11. Systèmes de Journalisation, Diagnostic et Terminal Distant](#11-systèmes-de-journalisation-diagnostic-et-terminal-distant)
  - [11.1 Système de Journalisation (Logging)](#111-système-de-journalisation-logging)
  - [11.2 Système de Diagnostic](#112-système-de-diagnostic)
  - [11.3 Monitoring des Performances](#113-monitoring-des-performances)
  - [11.4 Terminal Distant](#114-terminal-distant)
  - [11.5 Intégration dans le Système Global](#115-intégration-dans-le-système-global)
  - [11.6 Avantages pour la Maintenance et le Débogage](#116-avantages-pour-la-maintenance-et-le-débogage)
- [12. Historique des Modifications](#12-historique-des-modifications)

## Introduction
Ce document présente une description détaillée du projet d'autopilote pour kite générateur d'électricité. Le système utilise un cerf-volant de traction (kite) contrôlé de manière automatique pour produire de l'électricité. Ce document sert de guide pour le développement matériel et logiciel, en mettant l'accent sur la modularité et la maintenabilité du code.

```
      ^  ^  ^  WIND DIRECTION  ^  ^  ^
      |  |  |                  |  |  |
       
      /=============\  
       /|             |\  
      / |    KITE     | \  
       /__|_____________|__\  
       /|    with      |\  
      / |  IMU+ESP32   | \  
       /  |             |  \  
      /   |             |   \  
       /    |             |    \  
      /     |             |     \  
     /      |             |      \  
    /       |             |       \  
   LEFT  /        |             |        \  RIGHT  
   LINE /         |             |         \ LINE  
     /          |             |          \  
    /           |             |           \  
     /            |             |            \  
    /             |             |             \  
   /              |             |              \  
  v               |             |               v  

  ┌──────────────────────────────────────────────────────────────────────────────┐
  │                                                                              │
  │  ┌─────────────────────── GROUND STATION ────────────────────────────────┐   │
  │  │                                                                      │   │
  │  │  ┌─── SENSORS ────┐    ┌─── CONTROL ────┐    ┌─── INTERFACE ──────┐  │   │
  │  │  │                │    │                │    │                   │  │   │
  │  │  │ ⚡ Line Tension │    │ ⚙️ ESP32        │    │ 📱 Web UI         │  │   │
  │  │  │ 📏 Line Length │    │   Controller   │    │ 🖥️ LCD Display     │  │   │
  │  │  │ 🌬️ Wind Sensor │    │   - Autopilot  │    │                   │  │   │
  │  │  │                │    │   - Power Gen  │    │                   │  │   │
  │  │  └────────────────┘    └────────────────┘    └───────────────────┘  │   │
  │  │                                                                      │   │
  │  │  ┌─── ACTUATORS ────────────────────────────────────────────────┐    │   │
  │  │  │                                                             │    │   │
  │  │  │ ↔️  Direction (Servo 1)                                     │    │   │
  │  │  │ ⬆⬇ Trim/Power (Servo 2)                                    │    │   │
  │  │  │ ⚙️  Generator/Winch (Servo 3)                               │    │   │
  │  │  │                                                             │    │   │
  │  │  └─────────────────────────────────────────────────────────────┘    │   │
  │  │                                                                      │   │
  │  │  ┌─── POWER SYSTEM ─────────────────────────────────────────────┐    │   │
  │  │  │                                                             │    │   │
  │  │  │ 🔋 Battery                                                  │    │   │
  │  │  │ ⚡ Generator                                                │    │   │
  │  │  │ 🔌 Power Management                                         │    │   │
  │  │  │                                                             │    │   │
  │  │  └─────────────────────────────────────────────────────────────┘    │   │
  │  │                                                                      │   │
  │  └──────────────────────────────────────────────────────────────────────┘   │
  │                                                                              │
  └──────────────────────────────────────────────────────────────────────────────┘
```

Ce schéma illustre les composants principaux du système, incluant le kite avec ses capteurs embarqués, les lignes de traction, et la station au sol équipée de capteurs, actionneurs, et interfaces utilisateur. Chaque élément est organisé pour refléter son rôle dans le fonctionnement global du projet.

## 1. Objectif Global du Projet

Développer un système de pilotage automatique basé sur ESP32 pour un kite (cerf-volant de traction) dans le but de générer de l'électricité. Le système doit accomplir plusieurs tâches clés:

- Gérer le vol autonome du kite avec des trajectoires optimisées (notamment en "huit") pour maximiser la puissance
- Contrôler précisément la tension des lignes pour produire efficacement de l'énergie via un servo-générateur
- Assurer la sécurité du système dans toutes les conditions de fonctionnement
- Offrir des interfaces de contrôle et de monitoring (locale via LCD et distante via interface Web)

## 2. Composants Matériels Clés

### 2.1 Contrôleur Principal
- **Carte ESP32**
  - **Modèle recommandé** : ESP32-WROOM-32
  - **Caractéristiques clés** :
    - Processeur dual-core jusqu'à 240MHz
    - Mémoire SRAM de 520KB
    - Connectivité WiFi et Bluetooth intégrée
  - **Avantages** :
    - Puissance de calcul suffisante pour l'algorithme d'autopilotage
    - Capacités sans fil intégrées pour la communication avec les capteurs et interfaces
  - **Alternatives** :
    - ESP32-S3 : Plus moderne, avec USB natif et meilleures performances.

### 2.2 Actionneurs
#### 2.2.1 Servo 1 : Direction
- **Fonction** : Contrôle la direction gauche/droite du kite en ajustant la longueur relative des lignes directionnelles.
- **Spécifications recommandées** :
  - Servo haute vitesse
  - Couple modéré (~10-15kg/cm)
- **Rôle dans le système** : Permet de maintenir ou modifier la trajectoire du kite.

#### 2.2.2 Servo 2 : Trim
- **Fonction** : Ajuste l'angle d'incidence du kite pour modifier sa puissance ou traction.
- **Action** :
  - "Border" : Augmente la puissance.
  - "Choquer" : Réduit la puissance.
- **Spécifications recommandées** :
  - Servo précis
  - Couple modéré à élevé selon la taille du kite.
- **Rôle dans le système** : Optimise la performance aérodynamique du kite.

#### 2.2.3 Servo 3 : Générateur/Treuil
- **Fonctions principales** :
  - **Génération d'énergie** :
    - Agit comme un frein/générateur en résistant activement à la traction des lignes.
    - Résistance contrôlable pour optimiser la génération d'énergie.
    - Couplé à un générateur électrique (moteur DC, alternateur, etc.).
  - **Enroulement des lignes** :
    - Enroule les lignes pour ramener le kite en position de faible traction.
    - Utilisé pour la phase de récupération ou pour poser le kite.
    - Nécessite une puissance suffisante pour rembobiner contre une légère résistance.
- **Spécifications recommandées** :
  - Servo robuste avec couple élevé.
  - Capacité à fonctionner dans des cycles répétés de traction et relâchement.

### 2.3 Capteurs
#### 2.3.1 IMU (Unité de Mesure Inertielle)
- **Emplacement** : Montée directement sur le kite.
- **Fonction** : Fournit l'orientation 3D complète (Roll, Pitch, Yaw).
- **Types recommandés** :
  - MPU-9250 : Gyroscope, accéléromètre et magnétomètre.
  - BNO055 : Fusion de capteurs intégrée pour des données prêtes à l'emploi.
- **Communication** :
  - Options sans fil pour relier l'IMU du kite à l'ESP32 au sol :
    - LoRa : Longue portée, faible consommation.
    - ESP-NOW : Protocole léger entre ESP32.
  - **Solution recommandée** : Second ESP32 miniature sur le kite communiquant via ESP-NOW.

#### 2.3.2 Capteur de Tension de Ligne
- **Fonction** : Mesure la force de traction exercée par le kite.
- **Options** :
  - Cellule de charge (load cell) avec amplificateur HX711.
  - Capteur à jauge de contrainte.
- **Importance** : Donnée critique pour l'autopilote et l'optimisation de la génération d'énergie.

#### 2.3.3 Capteur de Longueur de Ligne
- **Fonction** : Mesure la distance entre le kite et la station au sol.
- **Implémentation** : Encodeur rotatif monté sur le treuil/Servo 3.
- **Type recommandé** : Encodeur quadrature pour précision et détection du sens.

#### 2.3.4 Anémomètre/Girouette (Optionnel mais recommandé)
- **Fonction** : Mesure la vitesse et la direction du vent au sol.
- **Importance** : Critique pour l'adaptation de l'autopilote et la sécurité du système.
- **Type recommandé** : Anémomètre à ultrasons sans pièces mobiles.

### 2.4 Interfaces Utilisateur
#### 2.4.1 Interface Locale
- **Type** : Écran LCD 20x4 caractères avec connexion I2C.
- **Alternative** : Écran OLED graphique SSD1306 pour une meilleure visualisation.
- **Rôle** : Fournir des informations essentielles sur le système directement sur la station au sol.

#### 2.4.2 Interface Distante
- **Implémentation** : Interface Web responsive accessible via WiFi.
- **Mode de fonctionnement** :
  - ESP32 configuré comme point d'accès (AP).
  - Ou connecté à un réseau local pour un accès distant.
- **Rôle** : Permettre le contrôle et le monitoring à distance.

### 2.5 Alimentation
- **Besoins** :
  - Source fiable pour l'ESP32 et les servos (particulièrement le Servo 3).
- **Options** :
  - Batterie LiPo avec convertisseurs DC-DC appropriés.
  - Circuit de stockage et gestion de l'énergie générée.
  - Protection contre les surcharges et les décharges excessives.
- **Recommandations spécifiques** :
  - Batterie de voiture 12V 120Ah pour le prototypage.
  - Convertisseur DC-DC buck pour alimenter l'ESP32 et les composants électroniques.

### 2.6 Architecture des ESP32
#### 2.6.1 ESP32 pour le Kite
- **Rôle** : Gérer les données du capteur IMU monté sur le kite.
- **Caractéristiques** :
  - Collecte des données d'orientation (Roll, Pitch, Yaw) via l'IMU.
  - Communication sans fil avec l'ESP32 de la station au sol via ESP-NOW ou LoRa.
  - Alimentation légère et faible consommation pour un fonctionnement autonome.

#### 2.6.2 ESP32 pour la Station au Sol
- **Rôle** : Centraliser les données des capteurs et contrôler les actionneurs.
- **Caractéristiques** :
  - Réception des données de l'ESP32 du kite.
  - Gestion des actionneurs (servos, générateur, etc.).
  - Interface utilisateur locale (LCD) et distante (Web UI).
  - Traitement des données pour l'autopilote et la génération d'énergie.


## 3. Fonctionnalités Logicielles Clés

### 3.1 Autopilote
- **Algorithme de contrôle principal**
  - Maintien du kite dans la fenêtre de vent optimale
  - Stabilisation active basée sur les données de l'IMU
  - Utilisation d'un contrôleur PID multi-paramètres
  
- **Trajectoires optimisées**
  - Vol en "figure de huit" pour maximiser le différentiel de vitesse et la puissance
  - Trajectoires adaptatives selon la vitesse du vent
  - Paramètres ajustables: taille du huit, agressivité des virages, altitude

- **Traitement des données capteurs**
  - Fusion des données de l'IMU, tension et longueur de ligne
  - Filtrage du bruit (filtre de Kalman recommandé)
  - Prédiction de trajectoire à court terme

- **Séquences automatisées**
  - Logique de décollage assisté/automatique avec détection des conditions favorables
  - Procédures d'atterrissage sécurisées avec contrôle de tension

### 3.2 Gestion de la Génération d'Énergie
- **Contrôle optimisé du Servo 3**
  - Application d'une résistance variable (couple de freinage) pendant la phase de traction
  - Profils de résistance programmables selon les conditions
  
- **Algorithmes d'optimisation**
  - Adaptation dynamique de la résistance en fonction des données capteurs
  - Modèles prédictifs pour anticiper les pics de traction
  - Objectif configurable: maximisation de la puissance ou durée de génération

- **Surveillance et rendement**
  - Mesure en temps réel de la puissance générée (tension × courant)
  - Calcul du rendement instantané et cumulé
  - Historique des performances pour analyse

### 3.3 Gestion de l'Enroulement/Déroulement
- **Contrôle du treuil**
  - Algorithmes pour enrouler les lignes pendant la phase de faible traction
  - Synchronisation avec le trim pour minimiser la résistance
  - Contrôle de vitesse variable selon la position du kite
  
- **Optimisation du cycle**
  - Minimisation de l'énergie consommée pendant la phase de récupération
  - Détection du moment optimal pour passer de génération à récupération

### 3.4 Modes Opérationnels
- **Standby/Idle**
  - Système en attente, monitoring des capteurs
  - Vérifications périodiques du matériel

- **Manual Control**
  - Interface intuitive via Web UI
  - Contrôle direct et précis des servos avec retour visuel

- **Auto-Launch**
  - Séquence de décollage progressive et sécurisée
  - Détection des conditions favorables (force et direction du vent)
  - Étapes visuellement représentées sur l'interface

- **Auto-Fly / Power Generation**
  - Vol en huit ou autre trajectoire optimisée avec monitoring continu
  - Adaptation aux conditions changeantes
  - Indicateurs de performance en temps réel

- **Auto-Reel In / Recovery**
  - Phase d'enroulement contrôlée avec rétroaction des capteurs
  - Optimisation énergétique pendant cette phase

- **Auto-Park / Land**
  - Séquence d'atterrissage sécurisée avec contrôle de tension
  - Options d'atterrissage d'urgence en conditions défavorables

- **Emergency Stop**
  - Arrêt sécurisé immédiat avec mise en drapeau du kite
  - Arrêt de la génération et sécurisation des lignes
  - Procédure pouvant être déclenchée automatiquement ou manuellement

### 3.5 Interface Web
- **Visualisation en temps réel**
  - Dashboard interactif avec:
    - Orientation 3D du kite visualisée graphiquement
    - Données des capteurs (IMU, tension, longueur, vent)
    - État du système et mode actuel
    - Puissance générée instantanée et cumulative
    - Position estimée dans la fenêtre de vent

- **Contrôles**
  - Changement de mode opérationnel
  - Démarrage/arrêt des séquences automatiques
  - Interface de contrôle manuel avec retour visuel
  - Ajustements des paramètres d'autopilote (agressivité, taille du huit, etc.)
  - Paramètres de génération de puissance (résistance cible)

- **Fonctionnalités avancées**
  - Accès aux diagnostics et logs avec filtrage
  - Configuration complète du système
  - Contrôle du mode simulation avec visualisation des résultats
  - Graphiques historiques des performances et conditions

### 3.6 Affichage LCD
- **Informations essentielles**
  - Mode opérationnel actuel avec indicateur visuel
  - État du système et alertes
  - Tension des lignes et puissance instantanée
  - Messages d'erreur critiques

- **Interface de navigation**
  - Menu accessible via boutons physiques
  - Configuration basique sans nécessiter l'interface Web

### 3.7 Système de Diagnostic Avancé
- **Auto-vérification**
  - Séquence complète de tests au démarrage
  - Vérification des capteurs, actionneurs et communications

- **Surveillance continue**
  - Monitoring de la plausibilité des données capteurs
  - Détection de pannes de communication ou matérielles
  - Validation de la cohérence entre capteurs redondants

- **Détection d'anomalies**
  - Identification des conditions anormales (tension excessive, angles dangereux)
  - Algorithmes prédictifs pour anticipation des situations à risque
  - Réponse graduée selon la gravité

- **Logging et analyse**
  - Enregistrement détaillé des événements et erreurs
  - Niveaux de verbosité configurables
  - Export des données pour analyse externe
  - Visualisation graphique des logs dans l'interface Web

### 3.8 Mode Simulation
- **Environnement de test**
  - Simulation complète du comportement du kite sans matériel réel
  - Modélisation physique simplifiée mais réaliste

- **Entrées simulées**
  - Génération de données IMU virtuelles
  - Simulation de conditions de vent variables
  - Possibilité d'injecter des perturbations pour tester la robustesse

- **Visualisation**
  - Représentation graphique 2D/3D de la trajectoire simulée
  - Comparaison avec les trajectoires théoriques optimales

### 3.9 Sécurité Intégrée
- **Limites opérationnelles**
  - Paramètres configurables: vitesse max du vent, tension max des lignes
  - Arrêt automatique en cas de dépassement des seuils

- **Procédures d'urgence**
  - Hiérarchie des réponses selon le type et la gravité de la situation
  - Options multiples: pose d'urgence, mise en position neutre, etc.

- **Détection préventive**
  - Identification des situations à risque avant qu'elles deviennent critiques
  - Alertes précoces basées sur la tendance des paramètres

- **Geofencing**
  - Définition d'une zone de vol sécurisée
  - Intervention automatique si le kite approche des limites

## 4. Points Problématiques du Projet

### 4.1 Communication sans fil avec l'IMU
- **Problème** : L'IMU montée sur le kite doit communiquer avec l'ESP32 au sol sans connexion filaire
- **Défis** : 
  - Portée potentiellement importante (50-200m selon la longueur des lignes)
  - Fiabilité de la communication en environnement extérieur
  - Latence critique pour le contrôle en temps réel
  - Consommation d'énergie pour l'électronique embarquée sur le kite
- **Solutions potentielles** : LoRa, ESP-NOW, ou un second ESP32 sur le kite

### 4.2 Alimentation et gestion de l'énergie
- **Problème** : Les servomoteurs (particulièrement le Servo 3 générateur/treuil) nécessitent une puissance importante
- **Défis** :
  - Source d'alimentation robuste pour le système au sol
  - Alimentation de l'électronique embarquée sur le kite
  - Gestion de l'énergie générée (stockage, conversion, utilisation)
  - Équilibre entre génération et consommation
- **Solutions pour le prototypage** :
  - Alimentation en 12V pour le servo/treuil (puissance suffisante pour gérer des charges importantes)
  - Utilisation d'une batterie de voiture 12V 120Ah comme source d'énergie principale et système de stockage
  - Convertisseur DC-DC buck pour alimenter l'ESP32 et les composants électroniques à partir du 12V
  - Circuit de protection pour éviter la surcharge ou la décharge excessive de la batterie

### 4.3 Résistance mécanique et fabrication
- **Problème** : Le système doit résister à des forces de traction importantes
- **Défis** :
  - Conception d'un treuil/générateur suffisamment robuste
  - Système de freinage contrôlable pour la génération d'électricité
  - Fixation sécurisée des composants au kite sans compromettre ses performances aérodynamiques

### 4.4 Algorithme d'autopilotage complexe
- **Problème** : Développer un autopilote capable de maintenir une trajectoire optimale en conditions variables
- **Défis** :
  - Modélisation du comportement du kite
  - Algorithmes de contrôle adaptatifs aux conditions de vent changeantes
  - Gestion des trajectoires optimales pour la génération d'énergie (figure en "8")
  - Logique de décollage et d'atterrissage automatisés

### 4.5 Sécurité du système
- **Problème** : Assurer la sécurité de l'opération dans toutes les conditions
- **Défis** :
  - Détection fiable des situations dangereuses
  - Procédures d'urgence robustes
  - Protection contre les défaillances matérielles et logicielles
  - Gestion des conditions météorologiques extrêmes

### 4.6 Interface utilisateur et monitoring
- **Problème** : Créer des interfaces (locale et distante) intuitives et informatives
- **Défis** :
  - Limites d'affichage de l'écran LCD local
  - Performance du serveur web sur l'ESP32
  - Visualisation en temps réel des données complexes
  - Contrôle à distance sécurisé et réactif

### 4.7 Tests et validation
- **Problème** : Difficultés pour tester le système de façon progressive et sécuritaire
- **Défis** :
  - Besoin d'un mode simulation réaliste
  - Tests progressifs des composants individuels
  - Validation du système complet dans des conditions réelles
  - Instrumentation pour mesurer les performances et la génération d'énergie

### 4.8 Optimisation de la génération d'énergie
- **Problème** : Maximiser le rendement énergétique net du système
- **Défis** :
  - Équilibre entre la résistance appliquée et la vitesse du kite
  - Algorithmes d'optimisation de la puissance générée
  - Minimisation de la consommation des composants électroniques
  - Stratégie adaptative selon les conditions de vent

### 4.9 Modularité et évolutivité du code
- **Problème** : Développer une architecture logicielle modulaire et maintenable
- **Défis** :
  - Gestion des multiples fonctionnalités sans surcharger l'ESP32
  - Séparation claire des responsabilités entre modules
  - Documentation adéquate pour faciliter les modifications futures
  - Tests unitaires pour les composants logiciels critiques

## 5. Idées Additionnelles et Améliorations

### 5.1 Optimisation Énergétique
- **Bilan énergétique**
  - Calcul précis du rendement net (génération vs consommation)
  - Adaptation de la stratégie selon l'objectif (puissance max ou efficience)

- **Réduction de la consommation**
  - Modes d'économie d'énergie pour l'électronique
  - Optimisation du cycle de vol pour minimiser l'énergie d'enroulement

### 5.2 Apprentissage et Adaptation
- **Algorithmes adaptatifs**
  - Auto-calibration des paramètres de contrôle
  - Apprentissage des conditions locales de vent
  - Amélioration continue des trajectoires basée sur les performances

- **Machine Learning**
  - Potentiel d'intégration d'algorithmes simples pour l'optimisation
  - Identification des patterns météo favorables

### 5.3 Stockage d'Énergie
- **Système de batterie intégré**
  - Dimensionnement adapté à la production estimée
  - Circuits de charge/décharge intelligents

- **Utilisation de l'énergie stockée**
  - Alimentation autonome du système
  - Possibilité d'export vers d'autres équipements

### 5.4 Intégration Météo
- **API météo**
  - Connexion aux services de prévisions météo (si accès Internet)
  - Planification préventive basée sur les prévisions

- **Analyse des tendances**
  - Corrélation entre conditions météo et performance
  - Prédiction des meilleures heures de fonctionnement

### 5.5 Interface Web Avancée
- **Visualisation enrichie**
  - Graphiques temps réel avec Plotly.js ou Chart.js
  - Historique détaillé des performances et conditions

- **Application mobile**
  - Version responsive optimisée pour smartphone
  - Notifications push pour alertes importantes

- **Accès distant**
  - Option de contrôle via Internet (avec sécurité adaptée)
  - Partage des données avec une communauté d'utilisateurs

## 6. Attentes vis-à-vis de l'IA pour le Développement

### 6.1 Architecture Logicielle & Structure du Code
- **Conception robuste** ✓
  - ✅ Architecture modulaire implémentée le 14/04/2025
  - Implémentation efficace d'une machine à états
  - Utilisation optimale des capacités multi-cœurs de l'ESP32
  - Gestion des tâches avec FreeRTOS pour la parallélisation

- **Organisation modulaire**
  - **Structure actuelle** (après refactorisation du 14/04/2025):
    - `include/`
      - `config.h`: Configuration centrale du système
      - `led_manager.h`: Interface du gestionnaire LED
      - `lcd_manager.h`: Interface du gestionnaire LCD
      - `error_manager.h`: Interface du gestionnaire d'erreurs
      - `system_init.h`: Interface des fonctions d'initialisation
    - `src/`
      - `main.cpp`: Point d'entrée minimal
      - `led_manager.cpp`: Implémentation du gestionnaire LED
      - `lcd_manager.cpp`: Implémentation du gestionnaire LCD
      - `error_manager.cpp`: Implémentation du gestionnaire d'erreurs
      - `system_init.cpp`: Implémentation des fonctions d'initialisation

  - **Modules à développer** (prochaines étapes):
    - `autopilot.*`: Logique de contrôle du vol
    - `power_generation.*`: Gestion de la génération d'énergie
    - `sensors.*`: Interfaces avec tous les capteurs
    - `actuators.*`: Contrôle des servomoteurs
    - `web_interface.*`: Interface utilisateur Web
- `lcd_interface.*`: Interface LCD locale
    - `safety.*`: Systèmes de sécurité et détection d'erreurs
    - `config.*`: Gestion des paramètres configurables
    - `state_machine.*`: Machine à états principale
    - `sim.*`: Mode simulation
    
  > **Note**: Une refactorisation majeure a été effectuée le 14/04/2025 pour transformer l'architecture monolithique en une architecture modulaire. Pour plus de détails, consultez le fichier `kite-pilote_refactorisation.md`.

### 6.2 Exemples de Code et Implémentation
- **Exemples commentés**
  - Code Arduino C++ pour ESP32 couvrant les fonctionnalités clés
  - Implémentations de référence pour les algorithmes critiques

- **Bibliothèques recommandées**
  - Pour capteurs I2C (MPU9250, BNO055, etc.)
  - ESPAsyncWebServer pour l'interface Web
  - ArduinoJSON pour la sérialisation des données
  - PID_v1 pour les contrôleurs PID
  - ESP-NOW ou LoRa pour la communication sans fil
  - FastLED ou NeoPixel pour signalisation LED

### 6.3 Algorithmes et Contrôle
- **Autopilote**
  - Modèles mathématiques pour le contrôle optimal du kite
  - Implémentations de PID multi-dimensionnel
  - Stratégies de trajectoires efficaces

- **Optimisation**
  - Algorithmes pour maximiser le rendement énergétique
  - Stratégies de décision pour le cycle génération/récupération

### 6.4 Assistance au Débogage
- **Outils de diagnostic**
  - Systèmes de logging efficaces
  - Visualisation des données pour débogage
  - Tests unitaires pour les modules critiques

## 7. Défis Anticipés et Solutions

### 7.1 Communication entre l'IMU et l'ESP32
- **Défi**: Établir une communication fiable et temps réel entre l'IMU sur le kite et le contrôleur au sol.
- **Solutions possibles**:
  - Utilisation de LoRa pour sa longue portée (>1km) et sa faible consommation
  - Implémentation d'un second ESP32 miniature sur le kite avec ESP-NOW (portée ~200-300m en ligne directe)
  - Protocole de communication robuste avec vérification d'erreurs et prédiction

### 7.2 Résistance aux Conditions Environnementales
- **Défi**: Protection des composants électroniques contre l'humidité, la poussière et les vibrations.
- **Solutions**:
  - Boîtiers étanches avec classification IP65 ou supérieure
  - Connecteurs marins de qualité pour les connexions exposées
  - Amortisseurs pour réduire les vibrations sur les composants sensibles

### 7.3 Sécurité et Fiabilité
- **Défi**: Garantir le fonctionnement sécurisé dans toutes les conditions, y compris imprévues.
- **Solutions**:
  - Redondance des systèmes critiques
  - Watchdog hardware et software
  - Procédures de mise en sécurité automatisées à plusieurs niveaux

### 7.4 Optimisation de la Génération d'Énergie
- **Défi**: Maximiser l'énergie nette générée par le système.
- **Solutions**:
  - Algorithmes adaptatifs basés sur les données historiques
  - Modélisation physique du comportement aérodynamique du kite
  - Optimisation du cycle complet génération/récupération

## 8. Conclusion et Prochaines Étapes

Ce document constitue une base solide pour le développement du projet d'autopilote de kite générateur d'électricité. Les prochaines étapes incluent:

1. **Sélection finale des composants matériels** avec considération des contraintes de budget et disponibilité
2. **Mise en place de l'architecture logicielle** en suivant les principes de modularité décrits
3. **Développement d'un prototype initial** avec fonctionnalités de base
4. **Tests progressifs** en commençant par le mode simulation
5. **Itérations d'amélioration** basées sur les résultats des tests

Le projet présente un excellent potentiel pour démontrer la viabilité des technologies d'énergie renouvelable alternatives, avec des applications possibles dans des zones isolées ou comme complément à d'autres sources d'énergie renouvelable.

## 10. Architecture des Fichiers du Projet

### 10.1 Structure Globale
Le projet Kitepilote est organisé selon une architecture modulaire pour faciliter le développement, la maintenance et l'évolution du système. Voici la structure complète des fichiers et dossiers:

```
kitepilote/
├── architecture_fichiers.md  # Description détaillée de la structure des fichiers
├── etape.md                 # Guide étape par étape pour la mise en œuvre
├── projet_kite_complet.md   # Documentation technique complète (ce document)
├── README.md                # Présentation générale du projet
├── station_sol/             # Code source pour l'ESP32 de la station au sol
│   ├── actuators.cpp        # Implémentation de la gestion des actionneurs
│   ├── actuators.h          # Interface pour la gestion des actionneurs
│   ├── communication.cpp    # Implémentation des communications sans fil
│   ├── communication.h      # Interface pour les communications
│   ├── config.h             # Configuration globale et constantes
│   ├── display.cpp          # Implémentation de l'interface d'affichage
│   ├── display.h            # Interface pour la gestion de l'affichage
│   ├── sensors.cpp          # Implémentation de la gestion des capteurs
│   ├── sensors.h            # Interface pour la gestion des capteurs
│   └── station_sol.ino      # Programme principal de la station au sol
└── util_ia/                 # Ressources et documentation supplémentaires
    ├── image/               # Images et illustrations pour la documentation
    ├── prompt.md            # Guide méthodologique pour la collaboration
    └── schema_complet.md    # Schémas détaillés du système
```

### 10.2 Fonctionnement de la Station Sol
La station au sol est basée sur une architecture modulaire comportant plusieurs fichiers spécialisés:

#### 10.2.1 `station_sol.ino`
C'est le programme principal qui coordonne l'ensemble des fonctionnalités de la station au sol. Il implémente:
- L'initialisation des différents modules (écran, capteurs, actionneurs, communication)
- La boucle principale qui gère les différents modes de fonctionnement
- La coordination entre les différentes parties du système

#### 10.2.2 `config.h`
Ce fichier de configuration centralise toutes les constantes et paramètres du système:
- Définition des broches utilisées par l'ESP32
- Constantes de calibration et limites de sécurité
- Structures de données pour l'état du système et les communications
- Énumération des modes de fonctionnement

#### 10.2.3 `display.h` et `display.cpp`
Ces fichiers gèrent l'interface utilisateur locale via l'écran LCD 20x4:
- Affichage de l'écran de démarrage avec barre de progression
- Affichage des données des capteurs et de l'état du système
- Gestion des menus et des messages d'erreur

#### 10.2.4 `communication.h` et `communication.cpp`
Ces fichiers implémentent la communication sans fil avec l'ESP32 monté sur le kite:
- Configuration et initialisation du protocole ESP-NOW
- Envoi et réception des données entre les ESP32
- Vérification de l'intégrité des données et gestion des erreurs de communication

#### 10.2.5 `sensors.h` et `sensors.cpp`
Ces fichiers gèrent l'acquisition et le traitement des données des capteurs:
- Lecture des capteurs de tension des lignes, longueur, et conditions de vent
- Filtrage et calibration des données brutes
- Calculs dérivés comme la puissance générée

#### 10.2.6 `actuators.h` et `actuators.cpp`
Ces fichiers contrôlent les différents servomoteurs et actionneurs du système:
- Contrôle des servomoteurs de direction, trim et treuil/générateur
- Implémentation des routines de calibration
- Logiques de sécurité et procédure d'arrêt d'urgence

### 10.3 Flux de Données et Interactions

Les modules interagissent selon le schéma suivant:

1. **Acquisition des données**:
   - Les capteurs de la station sol collectent des données sur la tension des lignes et leur longueur
   - L'ESP32 du kite envoie les données d'orientation (IMU) à la station sol via communication sans fil

2. **Traitement et décision**:
   - Le programme principal analyse les données en fonction du mode actuel
   - L'algorithme d'autopilote calcule les positions optimales des actionneurs en mode automatique

3. **Action et contrôle**:
   - Les commandes sont envoyées aux servomoteurs pour ajuster la position du kite
   - Le système de génération d'énergie est optimisé en fonction des conditions

4. **Interface utilisateur**:
   - L'écran LCD affiche en temps réel les informations essentielles
   - Les boutons permettent de naviguer dans les menus et de changer les modes

Cette architecture modulaire facilite le développement itératif, les tests unitaires et l'évolution future du système.

## 11. Systèmes de Journalisation, Diagnostic et Terminal Distant

### 11.1 Système de Journalisation (Logging)

Le projet Kitepilote intègre un système de journalisation complet pour faciliter le débogage, l'analyse des performances et le suivi du fonctionnement du système. Ce système est particulièrement utile pour identifier les problèmes et comprendre le comportement du système dans le temps.

#### 11.1.1 Niveaux de Log

Le système utilise différents niveaux de journalisation pour catégoriser les messages selon leur importance :

1. **LOG_NONE** : Aucun log (désactive la journalisation)
2. **LOG_ERROR** : Erreurs critiques uniquement
3. **LOG_WARNING** : Erreurs et avertissements
4. **LOG_INFO** : Informations générales (niveau par défaut)
5. **LOG_DEBUG** : Informations détaillées pour le débogage
6. **LOG_VERBOSE** : Logs très détaillés pour le développement

#### 11.1.2 Fonctionnement du Système de Log

Les logs sont stockés à deux endroits :
- Dans un buffer circulaire en mémoire RAM pour un accès rapide
- Envoyés sur la console série pour les messages de niveau ERROR et WARNING

> **Remarque :** Le stockage des logs dans un système de fichiers (comme SPIFFS) a été supprimé pour garantir la compatibilité Wokwi. Les logs ne sont donc pas persistants après redémarrage.

Pour utiliser le système de log dans le code, des macros sont disponibles :

```cpp
// Exemples d'utilisation du système de log
LOG_INFO("MODULE", "Message d'information");
LOG_ERROR("CAPTEUR", "Erreur sur le capteur de tension: %d", valeurErreur);
LOG_DEBUG("COMM", "Données reçues: %f, %f, %f", val1, val2, val3);
```

#### 11.1.3 Gestion des Logs

Le système de log offre plusieurs fonctionnalités pour gérer les données journalisées :

- **Consultation des logs** : Récupération des derniers logs, filtrage par niveau ou module
- **Exportation** : Vers la console série ou via l'interface web
- **Nettoyage automatique** : Pour éviter de saturer la mémoire RAM

### 11.2 Système de Diagnostic

Le système de diagnostic permet de vérifier l'état des différents composants matériels et logiciels du système. Il est essentiel pour identifier rapidement les pannes ou les dysfonctionnements.

#### 11.2.1 Tests de Diagnostic Disponibles

1. **Diagnostic des capteurs** : Vérifie la connexion et la validité des données de chaque capteur (IMU, tension, longueur, vent)
2. **Diagnostic des actionneurs** : Teste la réponse de chaque servomoteur
3. **Diagnostic de la communication** : Vérifie la connexion WiFi et la communication avec l'ESP32 du kite (si applicable)
4. **Diagnostic de la mémoire** : Vérifie l'état de la mémoire EEPROM utilisée pour la configuration.
5. **Diagnostic de l'alimentation** : Surveille la tension de la batterie et l'état du système de gestion d'énergie

#### 11.2.2 Lancement des Diagnostics

Les diagnostics peuvent être lancés :
- Automatiquement au démarrage du système
- Manuellement via le terminal distant
- À intervalles réguliers pendant le fonctionnement

#### 11.2.3 Gestion des Résultats

Les résultats des diagnostics sont :
- Enregistrés dans le système de log
- Affichés sur l'écran LCD ou le terminal distant
- Utilisés pour mettre à jour l'état global du système

### 11.3 Monitoring des Performances

Le système intègre des outils pour surveiller les performances et optimiser le fonctionnement :

#### 11.3.1 Métriques Collectées

- Temps d'exécution de la boucle principale (min/max/moyenne)
- Utilisation de la mémoire RAM
- Taux d'erreurs de communication
- Temps de réponse des capteurs

#### 11.3.2 Analyse des Performances

Ces métriques permettent :
- D'identifier les goulots d'étranglement
- D'optimiser les algorithmes critiques
- De prévenir les problèmes liés aux ressources limitées

### 11.4 Terminal Distant

Le terminal distant offre une interface de commande complète pour contrôler et surveiller le système Kitepilote. Il est accessible via la connexion série et potentiellement via WiFi.

#### 11.4.1 Fonctionnalités du Terminal

Le terminal permet de :
- Exécuter des commandes pour contrôler tous les aspects du système
- Surveiller l'état du système en temps réel
- Effectuer des tests et des diagnostics
- Configurer les paramètres du système
- Consulter et exporter les logs

#### 11.4.2 Liste des Commandes Principales

| Commande | Description | Exemple |
| -------- | ----------- | ------- |
| `help` | Affiche l'aide et la liste des commandes | `help servo` |
| `status` | Affiche l'état actuel du système | `status` |
| `reboot` | Redémarre l'ESP32 | `reboot` |
| `diag` | Lance les diagnostics systèmes | `diag sensors` |
| `servo` | Contrôle les servomoteurs | `servo dir 90` |
| `sensor` | Lit les données des capteurs | `sensor all` |
| `mode` | Change le mode de fonctionnement | `mode manual` |
| `logs` | Gestion des logs | `logs show 10` |
| `perf` | Monitoring des performances | `perf report` |
| `test` | Lancement de tests | `test servos` |

#### 11.4.3 Utilisation de Base

Pour utiliser le terminal distant :
1. Connecter l'ESP32 au PC via un câble USB
2. Ouvrir un terminal série (115200 bauds)
3. Taper `help` pour voir les commandes disponibles
4. Utiliser les commandes pour interagir avec le système

#### 11.4.4 Exemple de Session

```
=== Terminal Kitepilote v1.0 ===
Tapez 'help' pour la liste des commandes
> status
=== État du Système ===
Mode: Standby
Connexion au kite: Non connecté

Données des capteurs:
  Tension ligne: 0.00 N
  Longueur ligne: 0.00 m
  Orientation kite: P=0.0° R=0.0° Y=0.0°
  Vent: 5.0 m/s, 180°
  Puissance: 0.00 W

État des actionneurs:
  Servo Direction: 90°
  Servo Trim: 90°
  Servo Treuil: 90°

Informations système:
  Mémoire libre: 240 Ko
  Durée d'exécution: 65 s
  Niveau de log: Information

> servo dir 100
Positionnement du servo de direction à 100°
> sensor tension
Tension des lignes: 0.25 N
```

### 11.5 Intégration dans le Système Global

Ces systèmes sont intégrés de manière cohérente dans l'architecture globale du projet :

1. **Initialisation** : Dans la fonction `setup()` du programme principal
   ```cpp
   // Initialisation du système de journalisation
   initLogger();
   LOG_INFO("MAIN", "Démarrage de la station sol Kitepilote v1.0");
   
   // Initialisation du terminal distant
   initRemoteTerminal();
   LOG_INFO("MAIN", "Terminal distant initialisé");
   
   // Lancement d'un diagnostic initial
   runDiagnostics();
   ```

2. **Utilisation dans la boucle principale** : Dans la fonction `loop()`
   ```cpp
   // Traitement du terminal distant (commandes entrantes)
   processRemoteTerminal();
   
   // Mesure et enregistrement du temps de boucle pour les performances
   unsigned long loopTime = millis() - loopStartTime;
   updatePerformanceMetrics(loopTime);
   
   // Vérification périodique du système de logs
   static unsigned long lastLogCheck = 0;
   if (millis() - lastLogCheck > 30000) {
     cleanupOldLogs();
     lastLogCheck = millis();
   }
   ```

### 11.6 Avantages pour la Maintenance et le Débogage

Ces systèmes offrent plusieurs avantages importants :

1. **Débogage facilité** : Les logs permettent de retracer les événements qui ont mené à un problème
2. **Maintenance à distance** : Le terminal distant permet d'intervenir sans accès physique à l'appareil
3. **Diagnostic préventif** : Détection des problèmes potentiels avant qu'ils ne causent une panne
4. **Support à l'apprentissage** : Interface pédagogique pour comprendre le fonctionnement du système

Ces outils sont particulièrement précieux pour un projet complexe comme Kitepilote, où plusieurs systèmes interagissent et où les conditions d'utilisation peuvent varier considérablement.

## 12. Historique des Modifications

### 12.1 Version du Document
| Date | Version | Description |
|------|---------|-------------|
| 14/04/2025 | 1.1 | Amélioration du document avec ajout d'une table des matières et mise à jour de l'architecture |
| 01/04/2025 | 1.0 | Version initiale de la documentation technique complète |

### 12.2 Historique du Code
| Date | Description |
|------|-------------|
| 14/04/2025 | Refactorisation de l'architecture monolithique vers une structure modulaire avec séparation des responsabilités |
| 15/03/2025 | Implémentation initiale du système de journalisation et diagnostic |
| 01/03/2025 | Première version fonctionnelle du prototype avec contrôle basique |

*Pour des informations plus détaillées sur la refactorisation récente, consultez le fichier `kite-pilote_refactorisation.md` dans le dossier docs.*