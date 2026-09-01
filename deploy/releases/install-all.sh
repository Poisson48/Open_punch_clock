#!/usr/bin/env bash
# Installe la synchro colo-apps pour Open Punch Clock (manifest + APK/AppImage).
#
# Sur le serveur colo-apps (nginx /releases/ déjà actif pour Colo Course) :
#   git -C /opt/colo-apps/Open_punch_clock pull
#   sudo ./deploy/releases/install-all.sh
#
# Installe le script de sync et une entrée cron dédiée ; ne modifie pas Colo Course.
set -euo pipefail

DOMAIN="colo-apps.les-crevettes-cevenoles.fr"
PUBLIC_BASE="https://${DOMAIN}/releases"
RELEASES_DIR="${PUNCH_RELEASES_DIR:-/var/lib/colo-apps/releases}"
OPT_DIR="${PUNCH_OPT_DIR:-/opt/colo-apps}"
REPO_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
SCRIPT_NAME="sync-openpunch-from-github.sh"
MANIFEST_NAME="open-punch-clock-manifest.json"

log() { printf '[%s] %s\n' "$(date -Iseconds)" "$*"; }
die() { log "ERREUR: $*"; exit 1; }

if [[ $EUID -ne 0 ]]; then
  die "Relancer avec sudo : sudo $0"
fi

for cmd in curl python3; do
  command -v "$cmd" >/dev/null || die "$cmd introuvable"
done

[[ -f "$REPO_DIR/deploy/releases/sync-from-github.sh" ]] \
  || die "Fichier manquant : deploy/releases/sync-from-github.sh"

log "=== 1/3 — répertoire releases + script ==="
mkdir -p "$RELEASES_DIR" "$OPT_DIR"
chmod -R a+rX "$RELEASES_DIR" 2>/dev/null || true
install -m 0755 "$REPO_DIR/deploy/releases/sync-from-github.sh" "$OPT_DIR/$SCRIPT_NAME"

log "=== 2/3 — cron (*/5 min) ==="
cat > /etc/cron.d/colo-apps-openpunch-releases <<EOF
# Open Punch Clock — manifest + APK/AppImage depuis GitHub
*/5 * * * * root PUNCH_RELEASES_DIR=$RELEASES_DIR PUNCH_RELEASES_PUBLIC_URL=$PUBLIC_BASE PUNCH_GITHUB_REPO=${PUNCH_GITHUB_REPO:-Poisson48/Open_punch_clock} $OPT_DIR/$SCRIPT_NAME >>/var/log/colo-apps-openpunch-releases.log 2>&1
EOF
chmod 644 /etc/cron.d/colo-apps-openpunch-releases

log "=== 3/3 — première synchro ==="
PUNCH_RELEASES_DIR="$RELEASES_DIR" PUNCH_RELEASES_PUBLIC_URL="$PUBLIC_BASE" \
  PUNCH_GITHUB_REPO="${PUNCH_GITHUB_REPO:-Poisson48/Open_punch_clock}" \
  "$OPT_DIR/$SCRIPT_NAME"

body="$(curl -fsSL --max-time 30 "$PUBLIC_BASE/$MANIFEST_NAME")" \
  || die "Échec GET $PUBLIC_BASE/$MANIFEST_NAME (nginx /releases/ actif ?)"
python3 -c "import json,sys; json.loads(sys.stdin.read())" <<<"$body" \
  || die "$MANIFEST_NAME : JSON invalide"

log "Terminé — $PUBLIC_BASE/$MANIFEST_NAME"
echo "  Logs : /var/log/colo-apps-openpunch-releases.log"
