# Guide de documentation des modifications – Kite Pilote

## Règles Générales
- Parle en français
- Francise le projet autant que possible
- Utilise des noms de fonction en français sauf pour les fonctions système d'Arduino comme `setup()` et `loop()`

## Documentation des Modifications
- Documenter toute modification, même mineure, dans le changelog.
- Mettre à jour le numéro de version dans config.h
- Utiliser un format standard : date, heure, titre bref, liste des changements, impact et fichiers concernés.
- Organiser le changelog par version, puis par date (ordre antéchronologique).
- Expliquer le pourquoi de chaque modification, pas seulement le quoi.
- Catégoriser : bugfix, nouvelle fonctionnalité, refactorisation, optimisation, documentation, configuration.
- Mettre à jour les fichiers README.md concernés lorsque des modifications impactent l'usage ou l'architecture du projet.

## Conventions de Nommage
- Membres privés de classe: préfixe `_` (underscore) ex: `_variable`
- Classes et structures: PascalCase ex: `MaClasse`
- Méthodes et fonctions: camelCase ex: `maFonction()`
- Constantes: UPPER_SNAKE_CASE ex: `MA_CONSTANTE`
- Énumérations: préfixe du type puis valeurs en UPPER_SNAKE_CASE ex: `enum TypeEnum { TYPE_VALEUR_1, TYPE_VALEUR_2 }`

## Structure des Fichiers
- Séparer l'interface (header) de l'implémentation (.cpp)
- Un fichier header (.h) ne doit contenir que les déclarations, pas les implémentations (sauf inline si nécessaire)
- Organiser le code en modules thématiques dans des dossiers dédiés:
  - `/include/core/` : Fichiers d'en-tête fondamentaux
  - `/include/modules/` : Interfaces des modules fonctionnels
  - `/include/utils/` : Utilitaires et outils
  - `/src/modules/` : Implémentations des modules
  - `/src/utils/` : Implémentations des utilitaires

## Documentation du Code
- Documenter toutes les classes, méthodes et propriétés publiques avec des commentaires Doxygen
- Format Doxygen pour les classes:
  ```cpp
  /**
   * @class NomClasse
   * @brief Description brève
   * 
   * Description détaillée si nécessaire
   */
  ```
- Format Doxygen pour les méthodes:
  ```cpp
  /**
   * @brief Description de la méthode
   * @param parametre Description du paramètre
   * @return Description de la valeur de retour
   */
  ```

## Gestion de Version
- Incrémenter correctement la version selon la nature du changement:
  - VERSION_MAJOR: Changements majeurs incompatibles
  - VERSION_MINOR: Nouvelles fonctionnalités compatibles
  - VERSION_PATCH: Corrections de bugs
  - VERSION_BUILD: Modifications mineures ou workarounds
- Justifier tout changement de version dans le changelog

## Pratiques de Codage
- Limiter la taille des fonctions à moins de 50 lignes si possible
- Préférer l'utilisation de constantes nommées aux valeurs en dur
- Vérifier systématiquement les valeurs de retour des fonctions critiques
- Utiliser le système de logging (`LOG_INFO`, `LOG_ERROR`, etc.) plutôt que des `Serial.print()`
- Éviter les variables globales autant que possible

## Standards C++ Modernes et Bonnes Pratiques
- Utiliser les fonctionnalités du C++11/C++14 quand approprié
- Préférer les types forts et statiques (utiliser `enum class` plutôt que `enum`)
- Utiliser `nullptr` plutôt que `NULL` ou `0` pour les pointeurs
- Privilégier les smart pointers (`std::unique_ptr`, `std::shared_ptr`) aux pointeurs bruts
- Appliquer le RAII (Resource Acquisition Is Initialization) pour la gestion des ressources
- Privilégier les références aux pointeurs quand possible
- Utiliser `constexpr` pour les expressions évaluables à la compilation
- Marquer les méthodes qui ne modifient pas l'objet comme `const`
- Favoriser la composition à l'héritage quand c'est approprié
- Construire les classes avec une responsabilité unique (principe SOLID)
- Initialiser les variables membres dans les listes d'initialisation des constructeurs
- Utiliser la capture par référence dans les lambdas pour éviter les copies inutiles
- Préférer `auto` quand le type est évident ou verbeux, mais sans en abuser
- Appliquer le principe d'encapsulation (accesseurs, modificateurs, visibilité appropriée)
- Éviter l'utilisation de macros, préférer les constexpr, inline et templates
- Utiliser les fonctions noexcept pour les fonctions qui ne lèvent pas d'exceptions
- Préférer les algorithmes de la STL aux boucles manuelles quand approprié
- Utiliser std::array au lieu des tableaux C statiques
- Appliquer le principe "Rule of Five" ou "Rule of Zero" pour la gestion des ressources

## Sécurité et Robustesse
- Valider toutes les entrées, particulièrement celles provenant de sources externes
- Éviter les débordements de buffer en utilisant des conteneurs avec vérification de limites
- Gérer correctement les erreurs avec des mécanismes appropriés (codes de retour, exceptions, etc.)
- Utiliser des assertions pour vérifier les préconditions et postconditions
- Appliquer une gestion rigoureuse de la mémoire pour éviter les fuites
- Implémenter une journalisation complète des événements critiques
- Tester explicitement les cas limites et les conditions d'erreur
