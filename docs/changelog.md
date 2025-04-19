<!-- filepath: /workspaces/Kite_Pilote_Public/docs/changelog.md -->
# Journal des Modifications du Projet Kite Pilote

Ce fichier sert de journal détaillé de toutes les modifications apportées au projet Kite Pilote.
Chaque entrée est horodatée avec précision, permettant de suivre l'évolution du projet.

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
