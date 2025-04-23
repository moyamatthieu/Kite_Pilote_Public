#!/usr/bin/env python3
"""
Script d'automatisation pour sauvegarder le projet Kite Pilote sur GitHub.

Ce script:
1. Exécute update_version.py pour garantir que la version est à jour
2. Extrait la version et les dernières modifications du changelog
3. Ajoute tous les fichiers modifiés
4. Crée un commit avec un message descriptif incluant les modifications
5. Pousse les changements vers GitHub

Usage:
    python3 script/github_backup.py [--remote NOM_REMOTE] [--branch NOM_BRANCHE]
"""

import os
import sys
import subprocess
import re
import argparse
from datetime import datetime

# Chemins des fichiers
CHANGELOG_PATH = "docs/changelog.md"
UPDATE_VERSION_SCRIPT = "script/update_version.py"

def run_command(command, error_message=None):
    """Exécute une commande shell et retourne le résultat"""
    try:
        result = subprocess.run(command, shell=True, check=True, text=True,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return result.stdout.strip()
    except subprocess.CalledProcessError as e:
        if error_message:
            print(f"ERREUR: {error_message}")
            print(f"Commande: {command}")
            print(f"Code de sortie: {e.returncode}")
            print(f"Sortie d'erreur: {e.stderr}")
        sys.exit(1)

def check_git_repo():
    """Vérifie si le répertoire courant est un dépôt Git"""
    return os.path.isdir(".git")

def update_version():
    """Exécute le script update_version.py pour mettre à jour la version"""
    print("Mise à jour de la version...")
    output = run_command(f"python3 {UPDATE_VERSION_SCRIPT}", 
                        "Échec de la mise à jour de la version")
    print(output)
    
    # Extraire la version du résultat
    version_match = re.search(r'Version mise à jour à (v[\d\.]+)', output)
    if version_match:
        return version_match.group(1)
    else:
        print("AVERTISSEMENT: Impossible d'extraire la version depuis la sortie du script")
        return "version inconnue"

def extract_latest_changes():
    """Extrait les dernières modifications du changelog"""
    if not os.path.exists(CHANGELOG_PATH):
        print(f"AVERTISSEMENT: Fichier {CHANGELOG_PATH} introuvable.")
        return "Modifications non détaillées"
    
    with open(CHANGELOG_PATH, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Recherche la section la plus récente (premier ### après un ## de date)
    sections = re.findall(r'## [^\n]+\n\n### ([^\n]+)[^\n]*\n- \*\*Catégorie\*\*: ([^\n]+)\n- \*\*Action\*\*:(.*?)(?=\n\n###|\Z)', 
                          content, re.DOTALL)
    
    if not sections:
        return "Modifications non détaillées"
    
    # Première section trouvée (la plus récente)
    title = sections[0][0].strip()
    category = sections[0][1].strip()
    
    # Extraire les points d'action avec une indentation propre
    action_lines = []
    for line in sections[0][2].strip().split('\n'):
        line = line.strip()
        if line.startswith('-'):
            action_lines.append(f"  {line}")
        else:
            action_lines.append(f"    {line}")
    
    action = '\n'.join(action_lines)
    
    return f"{title}\nCatégorie: {category}\n{action}"

def stage_all_changes():
    """Ajoute tous les fichiers modifiés au staging"""
    print("Ajout des fichiers modifiés...")
    run_command("git add .", "Échec de l'ajout des fichiers")

def create_commit(version, changes):
    """Crée un commit avec un message descriptif"""
    print("Création du commit...")
    
    # Date au format ISO
    today = datetime.now().strftime("%Y-%m-%d")
    
    # Message de commit multi-lignes
    commit_message = f"Version {version} - {today}\n\n{changes}"
    
    # Écrire le message dans un fichier temporaire pour gérer les caractères spéciaux
    with open(".git_commit_msg.tmp", "w", encoding="utf-8") as f:
        f.write(commit_message)
    
    # Créer le commit en utilisant le fichier temporaire
    run_command('git commit -F .git_commit_msg.tmp', "Échec de la création du commit")
    
    # Supprimer le fichier temporaire
    if os.path.exists(".git_commit_msg.tmp"):
        os.remove(".git_commit_msg.tmp")

def push_to_github(remote, branch):
    """Pousse les changements vers GitHub"""
    print(f"Push vers GitHub ({remote}/{branch})...")
    run_command(f"git push {remote} {branch}", 
                f"Échec du push vers {remote}/{branch}")

def check_for_changes():
    """Vérifie s'il y a des changements à committer"""
    output = run_command("git status --porcelain")
    return bool(output.strip())

def main():
    """Fonction principale"""
    # Parser les arguments de ligne de commande
    parser = argparse.ArgumentParser(description="Sauvegarde le projet Kite Pilote sur GitHub")
    parser.add_argument("--remote", default="origin", help="Nom du remote Git (défaut: origin)")
    parser.add_argument("--branch", default="main", help="Nom de la branche Git (défaut: main)")
    args = parser.parse_args()
    
    print(f"Script de sauvegarde GitHub: démarrage ({datetime.now().strftime('%H:%M:%S')})...")
    
    # Vérifier si le répertoire courant est un dépôt Git
    if not check_git_repo():
        print("ERREUR: Le répertoire courant n'est pas un dépôt Git.")
        sys.exit(1)
    
    # Mettre à jour la version
    version = update_version()
    
    # Vérifier s'il y a des changements
    if not check_for_changes():
        print("Aucun changement à sauvegarder.")
        return
    
    # Extraire les dernières modifications
    changes = extract_latest_changes()
    
    # Ajouter les fichiers
    stage_all_changes()
    
    # Créer le commit
    create_commit(version, changes)
    
    # Pousser les changements
    push_to_github(args.remote, args.branch)
    
    print(f"Sauvegarde terminée avec succès! ({datetime.now().strftime('%H:%M:%S')})")
    print(f"Version {version} sauvegardée sur GitHub.")

if __name__ == "__main__":
    main()
