# Projet Kite Pilote

## Version: 2.0.0.0 — Dernière mise à jour: 19 Avril 2025

## Présentation du projet

Le projet Kite Pilote est un système de contrôle automatique pour cerf-volant générateur d'électricité. Il utilise un ESP32 pour piloter un kite (cerf-volant de traction) afin de produire de l'électricité grâce aux mouvements optimisés.

Cette version du code a été refactorisée pour être plus accessible aux débutants en programmation Arduino et ESP32. La structure modulaire permet de comprendre facilement le fonctionnement de chaque partie du système.

### Concept technique

Le système utilise un contrôleur ESP32 qui coordonne plusieurs modules fonctionnels pour gérer l'orientation et la tension des lignes d'un cerf-volant de traction. L'objectif est d'optimiser les mouvements du kite (typiquement en forme de 8 ou circulaires) pour maximiser la génération d'électricité via un treuil générateur.

L'architecture logicielle repose sur une approche orientée objet avec une séparation claire des responsabilités entre les différents modules. Le système intègre :

- Un monitoring avancé en temps réel
- La gestion des erreurs et des conditions d'exploitation sécuritaires
- Des modes de vol optimisés pour différentes conditions de vent
- Un système de simulation pour faciliter les tests sans matériel

## Structure du projet

Le projet est organisé de façon modulaire pour faciliter la compréhension et la maintenance :

```plaintext
kite_pilote/
├── include/                  # Fichiers d'en-tête (headers)
│   ├── core/                 # Composants fondamentaux du système
│   │   ├── config.h          # Configuration centrale et constantes
│   │   └── data_types.h      # Structures de données partagées
│   ├── modules/              # Modules fonctionnels
│   │   ├── autopilot_module.h      # Logique d'autopilotage
│   │   ├── communication_module.h  # Communication externe
│   │   ├── diagnostic_module.h     # Diagnostic système
│   │   ├── lcd_module.h            # Affichage sur écran LCD
│   │   ├── led_module.h            # Contrôle des LEDs d'état
│   │   ├── sensor_module.h         # Gestion des capteurs
│   │   ├── servo_module.h          # Contrôle des servomoteurs
│   │   ├── simulation_module.h     # Simulation pour tests
│   │   └── web_interface_module.h  # Interface web
│   ├── interfaces/           # Interfaces utilisateur (structure prête pour extension)
│   └── utils/                # Utilitaires
│       ├── circular_buffer.h # Buffer circulaire pour les données
│       └── logger.h          # Système de journalisation
├── src/                      # Fichiers d'implémentation
│   ├── core/                 # Implémentation des composants de base
│   ├── modules/              # Implémentation des modules
│   ├── utils/                # Implémentation des utilitaires
│   └── main.cpp              # Point d'entrée du programme
├── lib/                      # Librairies tierces ou modules additionnels
├── docs/                     # Documentation du projet
│   ├── api/                  # Documentation technique de l'API
│   ├── guides/               # Guides d'utilisation
│   ├── kite/                 # Documentation sur les cerfs-volants
│   ├── changelog.md          # Journal des modifications
│   ├── projet_kite_complet.md
│   └── README.md
├── tests/                    # Tests unitaires et d'intégration
│   ├── unit/                 # Tests unitaires par module
│   ├── integration/          # Tests d'intégration
│   ├── system/               # Tests système
│   └── README.md
├── platformio.ini            # Configuration de PlatformIO
├── prompt.md                 # Guide de documentation et de gestion des modifications
├── diagram.json              # Diagrammes d'architecture
└── wokwi.toml                # Configuration de simulation Wokwi
```

## Modules principaux

### 1. Module LED

Gère les LEDs pour indiquer l'état du système avec différents motifs de clignotement :

- LED verte : indique le statut opérationnel du système
  - Clignotement lent : système en attente
  - Clignotement rapide : système actif
  - Fixe : opération normale
  - Éteinte : système désactivé
- LED rouge : signale les erreurs et problèmes
  - Clignotement lent : avertissement
  - Clignotement rapide : erreur critique
  - Fixe : erreur système grave

La classe `LedModule` permet de définir des motifs de clignotement personnalisés pour communiquer divers états système.

### 2. Module LCD

Contrôle l'affichage sur un écran LCD 20x4 caractères pour montrer l'état du système, les données des capteurs et les modes de fonctionnement :

- Ligne 1 : Mode actuel et statut du système
- Ligne 2 : Valeurs d'orientation (roll, pitch) et tension des lignes
- Ligne 3 : Paramètres de vol et puissance générée
- Ligne 4 : Barre de progression pour les séquences et notifications d'erreur

Le module inclut des fonctions d'affichage spécialisées comme les barres de progression et différents formats d'affichage adaptés aux types de données.

### 3. Module Capteurs

Centralise la lecture de tous les capteurs :

- IMU (MPU6050) : mesure l'orientation du kite
  - Roll : inclinaison latérale (±180°)
  - Pitch : inclinaison avant/arrière (±90°)
  - Yaw : orientation horizontale (0-359°)
- Capteur de tension des lignes : mesure la force exercée (en Newtons)
- Capteur de longueur des lignes : détermine la distance du kite
- Anémomètre/girouette : mesure la vitesse (m/s) et direction du vent (0-359°)

Le module garantit la validité et la fraîcheur des données avec des mécanismes de détection d'anomalies. Les structures de données comme `IMUData`, `WindData` et `LineData` encapsulent les mesures avec des métadonnées de validité.

### 4. Module Servomoteurs

Contrôle les trois servomoteurs du système :

- Servo Direction : orientation gauche/droite du kite (±45°)
- Servo Trim : angle d'incidence/puissance du kite (±30°)
- Servo Treuil/Générateur : génération d'énergie et rembobinage
  - 3 modes opérationnels : IDLE, REELING_IN (rembobinage), REELING_OUT (déroulement), BRAKE (frein), GENERATOR (générateur)
  - Contrôle de puissance variable (0-100%)

Le module implémente des protections contre les dépassements d'angle et les mouvements brusques, assurant des transitions fluides entre les positions cibles.

### 5. Module Autopilote

Implémente plusieurs modes de vol et séquences automatiques :

- **Mode Standby** : maintient le kite stable en position neutre
- **Mode Launch** : séquence de décollage en trois phases
  - Préparation (30% du temps)
  - Décollage (40% du temps)
  - Stabilisation (30% du temps)
- **Mode Land** : séquence d'atterrissage en trois phases
  - Réduction de puissance (30% du temps)
  - Rembobinage contrôlé (50% du temps)
  - Arrêt final (20% du temps)
- **Mode Eight Pattern** : vol en forme de huit pour maximiser la génération d'énergie
  - Utilise une fonction sinusoïdale avec `sin(2x)` pour créer un 8 horizontal
  - Amplitude configurable (par défaut 35°)
  - Optimise la puissance en fonction de la position dans le cycle
- **Mode Circular** : vol circulaire stabilisé
  - Génération d'énergie plus stable mais moins efficace
  - Rayon configurable (par défaut 30°)
- **Mode Power Generation** : optimisation de la génération d'énergie
  - Alterne entre trois phases :
    1. Vol en 8 rapide avec angle agressif
    2. Traction maximale au centre
    3. Transition de retour

Le module calcule en temps réel la puissance instantanée générée (watts) et l'énergie totale cumulée (watt-heures).

### 6. Module Simulation

Permet de tester le système sans matériel réel, notamment dans l'environnement Wokwi :

- Simule les capteurs IMU, vent et tension des lignes
- Fournit des contrôles virtuels pour changer de mode d'autopilote
- Permet de tester les séquences de vol et de génération d'énergie
- Génère des variations réalistes dans les données simulées

Activé par la définition `SIMULATION_MODE` dans le fichier de configuration.

### 7. Autres modules

- **Module de journalisation** : système de logs multi-niveaux pour déboguer
- **Module de diagnostic** : surveillance en temps réel des performances
- **Module de communication** : interfaces avec systèmes externes (optionnel)
- **Module Web** : interface web pour configuration et monitoring (optionnel)

## Structures de données clés

### SensorData

Structure de base pour tous les capteurs avec :

- Horodatage des mesures
- Indicateur de validité
- Méthodes pour vérifier la fraîcheur des données

### SystemStatus

Encapsule l'état global du système :

- État d'initialisation et erreurs
- Mémoire disponible et température CPU
- Temps de fonctionnement (uptime)
- Tension de la batterie

### AutopilotStatus

Contient les informations sur l'état actuel de l'autopilote :

- Mode de fonctionnement actif
- Pourcentage de complétion des séquences
- Message de statut textuel
- Puissance générée (W) et énergie totale (Wh)
- Compteur de cycles de vol

## Fonctionnalités de sécurité

Le système intègre plusieurs mécanismes de sécurité :

- Surveillance continue de la tension des lignes avec seuils de sécurité

- Vérification de validité des données capteurs avant prise de décision

- Watchdog timer pour éviter les blocages du système

- Surveillance de la mémoire disponible

- Modes d'urgence en cas de conditions anormales

- Journalisation des événements critiques

## Mode d'emploi

### Prérequis matériels

- Carte ESP32 (recommandé: ESP32 DevKit V1 ou ESP32 WROOM)
- Écran LCD 20x4 avec interface I2C
- Module IMU MPU6050
- 3 servomoteurs standard (5V, signal PWM)
- LEDs d'état (verte et rouge)
- Circuit de mesure de tension pour les lignes
- Anémomètre et girouette (optionnels pour la version complète)
- Alimentation stable 5V / 2A minimum

### Prérequis logiciels

- IDE Arduino (1.8.x ou supérieur) ou Visual Studio Code avec PlatformIO
- Bibliothèques requises :
  - LiquidCrystal_I2C
  - ESP32Servo
  - Wire
  - MPU6050_tockn (pour l'IMU)
  - ESP32 Arduino Core

### Installation

1. Cloner le dépôt ou télécharger les fichiers source
2. Ouvrir le projet dans votre IDE (Arduino ou PlatformIO)
3. Installer les bibliothèques requises
4. Configurer le fichier `include/core/config.h` selon votre matériel
5. Compiler et téléverser sur votre ESP32

### Configuration

Toutes les constantes de configuration sont centralisées dans le fichier `include/core/config.h` :

- **Broches matérielles** :
  - `LED_GREEN_PIN`, `LED_RED_PIN` : broches des LEDs d'état
  - `SERVO_DIRECTION_PIN`, `SERVO_TRIM_PIN`, `SERVO_WINCH_PIN` : broches des servomoteurs
  - `LCD_I2C_ADDRESS`, `LCD_COLS`, `LCD_ROWS` : configuration de l'écran LCD
  - Broches pour les capteurs IMU, tension, etc.

- **Paramètres système** :
  - `SERIAL_BAUD_RATE` : vitesse de communication série
  - `MIN_FREE_MEMORY` : seuil minimal de mémoire libre
  - `MAX_SAFE_TENSION` : tension maximale sécuritaire pour les lignes

- **Paramètres des servomoteurs** :
  - `DIRECTION_MIN_ANGLE`, `DIRECTION_MAX_ANGLE` : limites du servo de direction
  - `TRIM_MIN_ANGLE`, `TRIM_MAX_ANGLE` : limites du servo de trim
  - `WINCH_MIN_POWER`, `WINCH_MAX_POWER` : limites de puissance du treuil

- **Mode simulation** :
  - Activer/désactiver avec `SIMULATION_MODE`
  - Configuration des broches virtuelles pour la simulation

### Mode simulation

Le système peut fonctionner en mode simulation, activé par la définition `SIMULATION_MODE` dans `config.h`. Ce mode est particulièrement utile pour :

- Tester le logiciel sans le matériel complet
- Démontrer les fonctionnalités du système
- Développer et déboguer de nouvelles fonctionnalités
- Simuler différentes conditions de vent et scénarios de vol

En mode simulation :

1. Les capteurs réels sont remplacés par des générateurs de données virtuelles
2. Des boutons virtuels permettent de changer les modes de l'autopilote
3. Des potentiomètres virtuels simulent les variations de vent et de tension
4. L'environnement Wokwi permet une visualisation du système

### Compilation et téléversement

1. Ouvrez le projet dans Arduino IDE ou PlatformIO
2. Configurez les paramètres selon votre matériel dans `config.h`
3. Sélectionnez le bon port et type de carte (ESP32)
4. Compilez le projet pour vérifier l'absence d'erreurs
5. Téléversez sur votre ESP32
6. Ouvrez le moniteur série pour suivre les logs (115200 bauds par défaut)

### Fonctionnement

À la mise sous tension, le système :

1. Initialise tous les modules (LEDs, LCD, capteurs, servos, autopilote)
2. Effectue des auto-tests pour vérifier la fonctionnalité des composants
3. Passe en mode STANDBY, prêt à recevoir des commandes

Indicateurs d'état :

- LED verte clignotante lentement : système en attente
- LED verte clignotante rapidement : système actif
- LED rouge clignotante : avertissement ou erreur
- Écran LCD : affiche les informations détaillées en temps réel

Cycle d'opération :

1. Les capteurs sont lus à chaque cycle (environ 20 fois par seconde)
2. L'autopilote calcule les commandes optimales selon le mode actif
3. Les servomoteurs sont positionnés selon ces commandes
4. L'écran LCD est mis à jour régulièrement (4 fois par seconde)
5. Des vérifications de sécurité sont effectuées périodiquement

En mode simulation, les potentiomètres simulent les capteurs et les boutons contrôlent le changement de mode.

## Caractéristiques techniques avancées

### Optimisation des performances

- Utilisation de tampons statiques pour éviter la fragmentation mémoire
- Mise à jour conditionelle de l'affichage pour réduire la charge CPU
- Surveillance périodique de la mémoire disponible
- Conception orientée objet avec faible couplage entre modules

### Stratégies de génération d'énergie

Le système implémente plusieurs stratégies pour maximiser la génération d'énergie :

1. **Vol en forme de huit (figure 8)** :
   - Maximise la vitesse apparente du vent
   - Optimise la traction lors des phases de traversée de fenêtre
   - Adapte l'amplitude selon les conditions de vent

2. **Optimisation de puissance** :
   - Ajuste la résistance du générateur en fonction de la position
   - Augmente la puissance dans les zones de vitesse maximale
   - Réduit la puissance dans les zones de transition

### Gestion des anomalies

Le système détecte et réagit à plusieurs types d'anomalies :

- Données de capteurs invalides ou incohérentes
- Tension excessive des lignes
- Mémoire système insuffisante
- Positions anormales des servomoteurs
- Conditions de vent dangereuses

## Contribuer au projet

La structure modulaire facilite l'extension du projet. Pour ajouter de nouvelles fonctionnalités :

1. **Créer un nouveau module** :

   - Créez un fichier d'en-tête dans `include/modules/`
   - Suivez le modèle des modules existants
   - Implémentez les méthodes `begin()`, `update()` et autres selon besoin

2. **Intégrer le module dans le programme principal** :

   - Ajoutez l'inclusion du header dans `main.cpp`
   - Créez une instance globale du module
   - Appelez `begin()` dans `initializeSystem()`
   - Appelez `update()` dans la boucle principale `loop()`

3. **Mettre à jour la configuration** :

   - Ajoutez les constantes nécessaires dans `config.h`
   - Documentez les nouveaux paramètres

### Conseils pour les contributions

- Respectez les conventions de nommage existantes

- Commentez votre code de manière claire et concise

- Minimisez l'utilisation de la mémoire dynamique

- Utilisez les structures de données existantes lorsque possible

- Ajoutez des messages de log appropriés

- Mettez à jour la documentation

## Dépannage

### Problèmes courants et solutions

1. **Problème** : Le système ne démarre pas correctement (LED rouge fixe)
   - **Solution** : Vérifiez les connexions matérielles, en particulier l'alimentation et les capteurs essentiels

2. **Problème** : L'écran LCD n'affiche rien
   - **Solution** : Vérifiez l'adresse I2C configurée dans `config.h`, testez avec un scanner I2C

3. **Problème** : Les servomoteurs ne répondent pas
   - **Solution** : Vérifiez l'alimentation des servos (5V séparé recommandé), assurez-vous que les broches PWM sont correctement configurées

4. **Problème** : Erreurs de compilation liées aux bibliothèques
   - **Solution** : Assurez-vous d'avoir installé toutes les bibliothèques requises, vérifiez les versions compatibles

5. **Problème** : Redémarrages aléatoires de l'ESP32
   - **Solution** : Augmentez la capacité de l'alimentation, ajoutez un condensateur de 470µF-1000µF entre 5V et GND

### Codes d'erreur

Le système affiche des codes d'erreur sur l'écran LCD en cas de problème :

- `ERR#1` : Erreur d'initialisation des capteurs
- `ERR#2` : Mémoire insuffisante
- `ERR#3` : Erreur de données IMU
- `ERR#4` : Tension de ligne excessive
- `ERR#5` : Erreur de communication I2C
- `ERR#6` : Erreur de servomoteur

## Licence

Ce projet est distribué sous licence open source.

## Remerciements

Projet développé par l'équipe Kite Pilote.

## Historique des versions

- **v2.0.0.0** (19/04/2025) - Version actuelle
  - Refactorisation complète du code avec architecture modulaire
  - Ajout du mode simulation
  - Amélioration des algorithmes d'autopilotage
  - Optimisation de la génération d'énergie

- **v1.5.2.0** (01/03/2025)
  - Ajout du support pour anémomètre/girouette
  - Amélioration de la stabilité système

- **v1.0.0.0** (15/12/2024)
  - Première version fonctionnelle
  - Support basique pour vol en 8 et vol circulaire
