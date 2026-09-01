# Mises à jour hébergées sur colo-apps (Open Punch Clock)

Les clients interrogent :

`https://colo-apps.les-crevettes-cevenoles.fr/releases/open-punch-clock-manifest.json`

(même infra que Colo Course, manifest séparé pour ne pas écraser `manifest.json`).

## Release GitHub

```bash
git tag -a v0.1.0 -m "Première release : pointeuse MVP."
git push origin v0.1.0
```

Le workflow `.github/workflows/release.yml` :

1. Build APK arm64 + AppImage Linux
2. Publie la GitHub Release avec les deux artefacts
3. Le serveur colo-apps synchronise via cron (ci-dessous)

Secrets GitHub requis (Settings → Secrets → Actions) :

| Secret | Description |
|---|---|
| `ANDROID_KEYSTORE_B64` | `base64 -w0 openpunchclock-release.jks` |
| `ANDROID_KEY_ALIAS` | alias du keystore |
| `ANDROID_KEYSTORE_PASS` | mot de passe |

Générer la clé une fois : `bash scripts/make-release-key.sh`

## Installation sur le serveur colo-apps

Si Colo Course est déjà configuré (`/releases/` nginx actif) :

```bash
sudo ./deploy/releases/install-all.sh
```

Ajoute uniquement le cron Open Punch Clock ; ne touche pas au sync Colo Course.

## Format `open-punch-clock-manifest.json`

```json
{
  "version": "0.1.0",
  "publishedAt": "2026-09-01T12:00:00Z",
  "notes": "Texte affiché avant installation",
  "apkUrl": "https://colo-apps…/releases/openpunchclock-v0.1.0-arm64.apk",
  "appImageUrl": "https://colo-apps…/releases/OpenPunchClock-0.1.0-x86_64.AppImage",
  "releaseUrl": "https://github.com/Poisson48/Open_punch_clock/releases/tag/v0.1.0",
  "changelog": [{ "version": "0.1.0", "notes": "…", "publishedAt": "…" }]
}
```

Le script ne supprime que les anciens fichiers `openpunchclock-*` / `OpenPunchClock-*`.
