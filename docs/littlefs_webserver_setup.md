# Configuration de LittleFS pour le Serveur Web (Kite Pilote)

Ce document résume les étapes et la configuration nécessaires pour utiliser LittleFS afin de stocker et servir les fichiers de l'interface web (HTML, CSS, JS, images) dans le projet Kite Pilote, en particulier pour la simulation avec Wokwi et le déploiement via PlatformIO.

## 1. Structure du Projet

Le système de fichiers LittleFS est peuplé à partir du contenu d'un dossier spécifique à la racine du projet PlatformIO. Par convention, ce dossier est nommé `data/`.

```
votre_projet_cerf_volant/
├── .pio/
├── .vscode/
├── data/                 # <-- Contient les fichiers web (index.html, style.css, etc.)
│   ├── index.html
│   └── style.css
├── include/
├── lib/
├── src/
│   └── main.cpp
├── test/
├── platformio.ini        # <-- Configuration PlatformIO
└── wokwi.toml            # <-- Configuration Wokwi
```

Tout fichier placé dans le dossier `data/` sera accessible à la racine (`/`) du système de fichiers LittleFS sur l'ESP32.

## 2. Configuration `platformio.ini`

Pour que PlatformIO génère l'image LittleFS lors de la compilation ou du téléversement, la ligne suivante doit être présente dans la section de votre environnement (par exemple, `[env:esp32dev]`):

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

# Configuration pour LittleFS
board_build.filesystem = littlefs ; <-- Indique d'utiliser LittleFS
; board_build.partitions = default.csv ; Optionnel: utilise le schéma par défaut

lib_deps =
    ; ... autres dépendances ...
    ESPAsyncWebServer
    AsyncTCP
    FS # Inclus généralement avec le framework Arduino pour ESP32
    LittleFS # Inclus généralement avec le framework Arduino pour ESP32
```

## 3. Configuration `wokwi.toml`

Pour que Wokwi simule la présence des fichiers du dossier `data/` dans LittleFS, le fichier `wokwi.toml` à la racine du projet doit contenir :

```toml
# wokwi.toml
[wokwi]
version = 1

[wokwi.esp32.filesystem]
# Mappe le dossier local './data' à la racine '/' de LittleFS dans la simulation
"/": "./data"
```

## 4. Implémentation dans le Code (`WebInterfaceModule`)

L'analyse du code existant (`web_interface_module.h` et `web_interface_module.cpp`) a confirmé que :

*   Le module utilise `ESPAsyncWebServer`.
*   L'initialisation de LittleFS est effectuée dans `WebInterfaceModule::begin()` via `LittleFS.begin(true)`.
*   Le service des fichiers statiques depuis LittleFS est géré efficacement par la ligne suivante dans `WebInterfaceModule::setupRoutes()`:
    ```cpp
    _server.serveStatic("/", LittleFS, "/").setCacheControl("max-age=31536000");
    ```
*   Le fichier `main.cpp` initialise correctement le `WebInterfaceModule` et appelle sa méthode `update()`.

**Conclusion :** Aucune modification de code n'est requise pour activer le service de fichiers depuis LittleFS. Le système est déjà prêt.

## 5. Utilisation

1.  Placer tous les fichiers de l'interface web (HTML, CSS, JS, images) dans le dossier `data/`.
2.  Vérifier les configurations `platformio.ini` et `wokwi.toml`.
3.  Lancer la simulation Wokwi ou téléverser sur un ESP32 via PlatformIO (en utilisant l'option "Upload File System image").
4.  Accéder à l'interface web via l'adresse IP de l'ESP32.
