#!/usr/bin/env python3

import os
import sys
import json

def create_littlefs_image():
    # Chemin du dossier data contenant les fichiers à inclure dans LittleFS
    data_dir = "data"
    
    # Vérifier si le dossier data existe
    if not os.path.exists(data_dir) or not os.path.isdir(data_dir):
        print(f"Erreur: Le dossier '{data_dir}' n'existe pas.")
        return False
        
    # Récupérer la liste des fichiers dans le dossier data
    files = []
    for root, _, filenames in os.walk(data_dir):
        for filename in filenames:
            file_path = os.path.join(root, filename)
            if os.path.isfile(file_path):
                files.append({
                    "path": file_path,
                    "name": file_path[len(data_dir):] if file_path.startswith(data_dir) else file_path
                })
    
    if not files:
        print(f"Erreur: Aucun fichier trouvé dans le dossier '{data_dir}'.")
        return False
    
    # Créer ou mettre à jour le fichier wokwi.toml
    wokwi_toml = "wokwi.toml"
    fs_config = ""
    
    if os.path.exists(wokwi_toml):
        with open(wokwi_toml, "r") as f:
            fs_config = f.read()
        
        # Vérifier si une section LittleFS existe déjà
        if "[wokwi.littlefs]" in fs_config:
            lines = fs_config.split("\n")
            new_lines = []
            skip = False
            for line in lines:
                if line.strip() == "[wokwi.littlefs]":
                    skip = True
                    new_lines.append(line)
                elif skip and line.startswith("["):
                    skip = False
                    new_lines.append(line)
                elif not skip:
                    new_lines.append(line)
            fs_config = "\n".join(new_lines)
            
        # Supprimer l'ancienne section SPIFFS si elle existe
        if "[wokwi.spiffs]" in fs_config:
            lines = fs_config.split("\n")
            new_lines = []
            skip = False
            for line in lines:
                if line.strip() == "[wokwi.spiffs]":
                    skip = True
                elif skip and line.startswith("["):
                    skip = False
                    new_lines.append(line)
                elif not skip:
                    new_lines.append(line)
            fs_config = "\n".join(new_lines)
    
    # Ajouter la configuration LittleFS
    if "[wokwi.littlefs]" not in fs_config:
        if fs_config and not fs_config.endswith("\n"):
            fs_config += "\n"
        fs_config += "\n[wokwi.littlefs]\n"
        
    # Ajouter les entrées pour chaque fichier
    for file_info in files:
        file_path = file_info["path"]
        file_name = file_info["name"]
        if file_name.startswith("/"):
            file_name = file_name[1:]
        fs_config += f'"{file_name}" = "{file_path}"\n'
    
    # Écrire la configuration mise à jour dans wokwi.toml
    with open(wokwi_toml, "w") as f:
        f.write(fs_config)
    
    print(f"Configuration LittleFS mise à jour avec {len(files)} fichiers.")
    return True

if __name__ == "__main__":
    if create_littlefs_image():
        print("LittleFS configuré avec succès pour Wokwi.")
    else:
        print("Échec de la configuration LittleFS.")
        sys.exit(1)
