import subprocess
import datetime
import sys

def run_command(command):
    """Exécute une commande shell et retourne True en cas de succès, False sinon."""
    try:
        print(f"Exécution: {command}")
        # Utilisation de sys.executable pour s'assurer que le bon interpréteur Python est utilisé si nécessaire
        # pour des commandes spécifiques à Python, bien que pour git ce ne soit pas directement utile.
        # check=True lève une exception CalledProcessError si la commande échoue.
        # capture_output=True capture stdout et stderr.
        # text=True décode stdout et stderr en texte.
        result = subprocess.run(command, shell=True, check=True, capture_output=True, text=True, encoding='utf-8')
        print(f"Succès:\n{result.stdout}")
        return True
    except subprocess.CalledProcessError as e:
        print(f"Erreur lors de l'exécution de la commande: {command}", file=sys.stderr)
        print(f"Code de retour: {e.returncode}", file=sys.stderr)
        print(f"Sortie d'erreur:\n{e.stderr}", file=sys.stderr)
        print(f"Sortie standard:\n{e.stdout}", file=sys.stderr)
        return False
    except FileNotFoundError:
        # Cette exception est levée si la commande elle-même (par exemple, 'git') n'est pas trouvée.
        print(f"Erreur: La commande '{command.split()[0]}' n'a pas été trouvée.", file=sys.stderr)
        print("Assurez-vous que Git est installé et accessible dans le PATH de votre système.", file=sys.stderr)
        return False
    except Exception as e:
        # Capturer d'autres exceptions potentielles
        print(f"Une erreur inattendue est survenue: {e}", file=sys.stderr)
        return False

def backup_and_commit():
    """Effectue la sauvegarde (git add, commit, push)."""
    print("--- Début du processus de sauvegarde Git ---")

    # 1. Vérifier si le répertoire actuel est un dépôt Git
    if not run_command("git rev-parse --is-inside-work-tree"):
         print("Erreur: Le répertoire actuel n'est pas un dépôt Git valide.", file=sys.stderr)
         print("--- Processus de sauvegarde échoué ---", file=sys.stderr)
         return

    # 2. Ajouter tous les fichiers modifiés et nouveaux au staging area
    print("\n1. Ajout des fichiers (git add .)...")
    if not run_command("git add ."):
        print("Échec de l'ajout des fichiers. Il peut y avoir des problèmes de permissions ou de configuration Git.", file=sys.stderr)
        print("--- Processus de sauvegarde échoué ---", file=sys.stderr)
        return

    # 3. Vérifier s'il y a des changements à commiter
    print("\n2. Vérification des changements à commiter (git status)...")
    status_result = subprocess.run("git status --porcelain", shell=True, capture_output=True, text=True, encoding='utf-8')
    if not status_result.stdout.strip():
         print("Aucun changement détecté à commiter. Le dépôt est à jour.")
         # Optionnel: Pousser quand même pour s'assurer que le distant est synchronisé ?
         # Pour l'instant, on arrête ici si pas de changements locaux.
         # print("\n4. Tentative de push pour synchroniser...")
         # if not run_command("git push"):
         #     print("Échec du push vers le dépôt distant.", file=sys.stderr)
         # else:
         #     print("Push réussi (ou déjà à jour).")
         print("--- Processus de sauvegarde terminé (aucun changement) ---")
         return
    else:
        print("Changements détectés, préparation du commit.")

    # 4. Créer un message de commit avec la date et l'heure actuelles
    print("\n3. Création du commit...")
    now = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    commit_message = f"Automated backup {now}"
    # Échapper les guillemets doubles dans le message si nécessaire, bien que peu probable ici.
    commit_command = f'git commit -m "{commit_message}"'
    if not run_command(commit_command):
        # Si 'git add .' a réussi mais 'git commit' échoue, c'est plus problématique.
        print("Échec de la création du commit. Vérifiez la configuration Git (user.name, user.email) et les éventuels hooks pre-commit.", file=sys.stderr)
        print("--- Processus de sauvegarde échoué ---", file=sys.stderr)
        return

    # 5. Pousser les changements vers le dépôt distant (branche actuelle)
    print("\n4. Poussée des changements vers le dépôt distant (git push)...")
    if not run_command("git push"):
        print("Échec du push vers le dépôt distant. Vérifiez la connexion réseau, les permissions du dépôt distant et l'état de la branche distante.", file=sys.stderr)
        print("--- Processus de sauvegarde échoué ---", file=sys.stderr)
        return

    print("\n--- Processus de sauvegarde terminé avec succès ---")

if __name__ == "__main__":
    # Vérifier si Git est installé avant de commencer
    try:
        subprocess.run("git --version", shell=True, check=True, capture_output=True, text=True, encoding='utf-8')
        print("Git est détecté.")
        backup_and_commit()
    except (subprocess.CalledProcessError, FileNotFoundError):
         print("Erreur critique: Git n'est pas installé ou n'est pas accessible dans le PATH.", file=sys.stderr)
         print("Veuillez installer Git et vous assurer qu'il est correctement configuré.", file=sys.stderr)
         sys.exit(1) # Quitter le script avec un code d'erreur