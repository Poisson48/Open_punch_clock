#!/usr/bin/env bash
# Génère LA clé de publication Open Punch Clock — une fois pour toutes.
#
#   bash scripts/make-release-key.sh
#
# ⚠️  Sauvegardez le .jks et le mot de passe hors du dépôt.
set -euo pipefail

OUT="${OUT:-$HOME/openpunchclock-release.jks}"
ALIAS="${ALIAS:-openpunchclock}"

if [ -f "$OUT" ]; then
  echo "Un keystore existe déjà : $OUT" >&2
  echo "Pour réafficher le secret : base64 -w0 \"$OUT\"" >&2
  exit 1
fi

STOREPASS="$(head -c 48 /dev/urandom | base64 | tr -d '/+=' | cut -c1-32)"

keytool -genkeypair \
  -keystore "$OUT" -alias "$ALIAS" \
  -storepass "$STOREPASS" -keypass "$STOREPASS" \
  -keyalg RSA -keysize 4096 -validity 10000 \
  -dname "CN=Open Punch Clock, O=OpenPunchClock, C=FR" >/dev/null

chmod 600 "$OUT"

REPO="$(git -C "$(dirname "$0")/.." remote get-url origin 2>/dev/null || echo '<votre/dépôt>')"

cat <<EOF

Keystore créé : $OUT   (alias : $ALIAS)

1. Sauvegardez ce fichier et le mot de passe :

     $STOREPASS

2. Secrets GitHub :

     gh secret set ANDROID_KEYSTORE_B64 --body "\$(base64 -w0 "$OUT")"
     gh secret set ANDROID_KEY_ALIAS    --body "$ALIAS"
     gh secret set ANDROID_KEYSTORE_PASS --body "$STOREPASS"

   (ou : $REPO → Settings → Secrets and variables → Actions)

3. Publiez une release : git tag -a v0.1.0 -m "…" && git push origin v0.1.0
EOF
