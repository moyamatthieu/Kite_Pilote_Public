import os
import subprocess

def build_firmware():
    """Compile le firmware principal."""
    print("Compilation du firmware...")
    result = subprocess.run(["pio", "run"], cwd="/workspaces/Kite_Pilote_Public")
    if result.returncode == 0:
        print("Compilation réussie.")
    else:
        print("Erreur lors de la compilation du firmware.")
        exit(1)

def build_filesystem():
    """Génère l'image LittleFS."""
    print("Génération de l'image LittleFS...")
    result = subprocess.run(["pio", "run", "--target", "buildfs"], cwd="/workspaces/Kite_Pilote_Public")
    if result.returncode == 0:
        print("Image LittleFS générée avec succès.")
    else:
        print("Erreur lors de la génération de l'image LittleFS.")
        exit(1)

if __name__ == "__main__":
    build_filesystem()
    build_firmware()