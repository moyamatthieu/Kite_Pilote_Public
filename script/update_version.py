#!/usr/bin/env python3
"""
Script d'automatisation pour mettre à jour le numéro de version dans config.h
à partir des informations du changelog.

Ce script:
1. Lit le fichier changelog.md pour extraire la dernière version
2. Met à jour les définitions VERSION_* dans config.h
3. Met à jour également la date de build

Usage:
    python3 script/update_version.py
"""

import re
import os
import sys
from datetime import datetime

# Chemins des fichiers
CHANGELOG_PATH = "docs/changelog.md"
CONFIG_PATH = "include/core/config.h"

def extract_version_from_changelog():
    """Extrait la dernière version mentionnée dans le changelog."""
    if not os.path.exists(CHANGELOG_PATH):
        print(f"Erreur: Fichier {CHANGELOG_PATH} introuvable.")
        return None

    with open(CHANGELOG_PATH, 'r', encoding='utf-8') as f:
        content = f.read()

    # Recherche la dernière version mentionnée (format v2.0.0.5)
    version_matches = re.findall(r'v(\d+)\.(\d+)\.(\d+)\.(\d+)', content)
    
    if not version_matches:
        print("Aucune version trouvée dans le changelog.")
        return None
    
    # Retourne la première occurrence (supposée être la plus récente)
    return {
        'major': int(version_matches[0][0]),
        'minor': int(version_matches[0][1]),
        'patch': int(version_matches[0][2]),
        'build': int(version_matches[0][3])
    }

def update_config_version(version):
    """Met à jour les définitions de version dans config.h"""
    if not os.path.exists(CONFIG_PATH):
        print(f"Erreur: Fichier {CONFIG_PATH} introuvable.")
        return False
    
    with open(CONFIG_PATH, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Met à jour les définitions de version
    content = re.sub(
        r'#define VERSION_MAJOR \d+', 
        f'#define VERSION_MAJOR {version["major"]}', 
        content
    )
    content = re.sub(
        r'#define VERSION_MINOR \d+', 
        f'#define VERSION_MINOR {version["minor"]}', 
        content
    )
    content = re.sub(
        r'#define VERSION_PATCH \d+', 
        f'#define VERSION_PATCH {version["patch"]}', 
        content
    )
    content = re.sub(
        r'#define VERSION_BUILD \d+', 
        f'#define VERSION_BUILD {version["build"]}', 
        content
    )
    
    # Met à jour la chaîne de version complète
    version_string = f'v{version["major"]}.{version["minor"]}.{version["patch"]}.{version["build"]}'
    content = re.sub(
        r'#define VERSION_STRING "v[\d\.]+"', 
        f'#define VERSION_STRING "{version_string}"', 
        content
    )
    
    # Met à jour la date de build
    today = datetime.now().strftime("%d/%m/%Y")
    content = re.sub(
        r'#define BUILD_DATE "[\d/]+"',
        f'#define BUILD_DATE "{today}"',
        content
    )
    
    # Écrit le contenu mis à jour
    with open(CONFIG_PATH, 'w', encoding='utf-8') as f:
        f.write(content)
    
    print(f"Version mise à jour à {version_string} dans {CONFIG_PATH}")
    print(f"Date de build mise à jour à {today}")
    return True

def main():
    """Fonction principale"""
    print("Script de mise à jour de version: démarrage...")
    
    version = extract_version_from_changelog()
    if not version:
        print("Impossible de continuer sans information de version.")
        sys.exit(1)
    
    print(f"Version extraite du changelog: v{version['major']}.{version['minor']}.{version['patch']}.{version['build']}")
    
    if update_config_version(version):
        print("Mise à jour réussie !")
    else:
        print("La mise à jour a échoué.")
        sys.exit(1)

if __name__ == "__main__":
    main()
