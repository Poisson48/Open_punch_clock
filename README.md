# Open Punch Clock

Alternative **open source** (GPLv3) à [Time Squared / Time Clock: Easy Tracker](https://play.google.com/store/apps/details?id=co.timesquared.timetracker) — pointeuse et feuille de temps, sans pub ni abonnement.

**Plateformes :** Linux (AppImage) · Android (APK arm64)  
**Version actuelle :** [v0.1.5](https://github.com/Poisson48/Open_punch_clock/releases/tag/v0.1.5)

## Téléchargement

| Plateforme | Fichier |
|---|---|
| Android | [`openpunchclock-v0.1.5-arm64.apk`](https://github.com/Poisson48/Open_punch_clock/releases/download/v0.1.5/openpunchclock-v0.1.5-arm64.apk) |
| Linux | [`OpenPunchClock-0.1.5-x86_64.AppImage`](https://github.com/Poisson48/Open_punch_clock/releases/download/v0.1.5/OpenPunchClock-0.1.5-x86_64.AppImage) |

Mises à jour automatiques (hors Play Store) via [colo-apps](https://colo-apps.les-crevettes-cevenoles.fr/releases/open-punch-clock-manifest.json).

## Fonctionnalités

- **Pointage** Clock In / Out / Pause, timer live, estimation des gains
- **Projets / clients** avec taux horaire et projet par défaut
- **Time cards** manuelles (début, fin, pause, notes, tags, remboursements / déductions)
- **Historique** groupé par jour, **rapports** hebdo / mensuels, heures sup, période de paie
- **Export** CSV et XLSX
- **Sync multi-appareils** E2E chiffrée (Nostr, optionnelle — infra [Colo Course](https://github.com/Poisson48/Colo_Course))
- **16 langues** (Qt Linguist) : fr, en, de, es, it, pt, pt_BR, nl, pl, ru, zh_CN, ja, ko, ar, tr, uk
- Rappels clock-out, audit local, GPS opt-in, thème clair / sombre

Voir [docs/COMPARAISON.md](docs/COMPARAISON.md) pour le détail face à Time Squared (propriétaire).

## Build (Linux)

```bash
./scripts/setup-dev.sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build
./build/src/openpunchclock
```

Android : `./scripts/setup-android.sh` puis `./scripts/build-android.sh`

## Release (mainteneurs)

```bash
# Une fois : clé Android + secrets GitHub
bash scripts/make-release-key.sh
# → gh secret set ANDROID_KEYSTORE_B64 / ANDROID_KEY_ALIAS / ANDROID_KEYSTORE_PASS

git tag -a v0.1.5 -m "Notes visibles dans l'app avant installation."
git push origin v0.1.5
```

Workflow [`release.yml`](.github/workflows/release.yml) : APK signé + AppImage → GitHub Release → sync colo-apps (cron).

Serveur : `sudo ./deploy/releases/install-all.sh` — voir [deploy/releases/README.md](deploy/releases/README.md).

## Documentation

| Fichier | Contenu |
|---|---|
| [docs/COMPARAISON.md](docs/COMPARAISON.md) | État vs Time Squared (concurrence propriétaire) |
| [docs/PLAN.md](docs/PLAN.md) | Plan de développement et phases |
| [docs/SPEC.md](docs/SPEC.md) | Spécification technique |
| [docs/UX.md](docs/UX.md) | Wireframes UX |

## Base technique

Réutilise l'infrastructure Colo Course : CMake, Qt 6 / QML, SQLite WAL, sync Nostr chiffrée, CI GitHub Actions, distribution colo-apps.

## Licence

[GPLv3](LICENSE) — Poisson48 / contributeurs.
