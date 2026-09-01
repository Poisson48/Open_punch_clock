# Open_punch_clock

Alternative open source à Time Squared — pointeuse / feuille de temps, Qt 6 + QML, Linux et Android.

## Fonctionnalités

- Punch In / Out / Break avec timer live et estimation des gains
- Projets/clients avec taux horaire
- Time cards manuelles, historique, rapports hebdo/mensuels
- Export CSV et XLSX
- Sync multi-appareils E2E (Nostr, optionnelle)
- Notifications de rappel, audit local, GPS opt-in

## Release

```bash
# Une fois : clé Android + secrets GitHub
bash scripts/make-release-key.sh

# Publier
git tag -a v0.1.0 -m "Notes visibles dans l'app avant installation."
git push origin v0.1.0
```

Workflow `release.yml` : APK + AppImage → GitHub Release → colo-apps (cron).

Serveur : `sudo ./deploy/releases/install-all.sh` — voir [deploy/releases/README.md](deploy/releases/README.md).

Manifest client : `https://colo-apps.les-crevettes-cevenoles.fr/releases/open-punch-clock-manifest.json`

## Build (Linux)

```bash
./scripts/setup-dev.sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build
./build/src/openpunchclock
```

## Documentation

- [Plan de développement](docs/PLAN.md)
- [Spécification technique](docs/SPEC.md)
- [Wireframes UX](docs/UX.md)

## Base technique

Réutilise l'infrastructure [Colo_Course](https://github.com/Poisson48/Colo_Course) : CMake, SQLite, sync E2E Nostr, CI AppImage/APK.

## Licence

GPLv3