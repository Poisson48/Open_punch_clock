# Open_punch_clock — Spécification technique (v1)

> Référence d'implémentation. Les sessions codeur suivent ce document.

## 1. Identifiants

- **deviceId** : UUID v4, premier lancement, immuable.
- **entryId** : UUID v4 par entrée de temps.
- **projectId** : UUID v4 par projet/client.
- **workspaceId** : UUID v4 du canal de sync (équivalent « liste » Colo Course).
- **ver** : `[lamport, deviceId]` — ordre total LWW.
- Horodatages : **ms epoch int64_t** partout.

## 2. Modèle de données

### 2.1 Project

| Champ | Type | Notes |
|---|---|---|
| projectId | string | PK |
| name | string | ≤ 200 chars |
| hourlyRate | double | €/h ou devise locale |
| color | string | hex `#RRGGBB` |
| isDefault | bool | un seul défaut |
| created | int64 | ms |
| del | bool | tombstone sync |

### 2.2 TimeEntry

| Champ | Type | Notes |
|---|---|---|
| entryId | string | PK |
| projectId | string | FK |
| startMs | int64 | début |
| endMs | int64 | 0 = en cours |
| breakMs | int64 | total pauses |
| notes | string | libre |
| tags | string | CSV tags |
| reimburse | double | remboursement |
| deduct | double | déduction |
| source | string | `punch` ou `manual` |
| lat, lon | double | GPS opt-in, 0 = absent |
| created, touched | int64 | métadonnées |
| del | bool | tombstone |

Champs versionnés pour sync LWW : name-level fields use per-field ver in payload.

### 2.3 PunchState (singleton local, non sync)

| Champ | Type |
|---|---|
| clockedIn | bool |
| projectId | string |
| clockInMs | int64 |
| breakStartMs | int64 (0 = pas en pause) |
| accumulatedBreakMs | int64 |

Persisté à **chaque transition** (G9).

### 2.4 Settings (KV)

`deviceId`, `displayName`, `payPeriodDays`, `overtimeThresholdH`, `reminderClockOutMin`, `gpsEnabled`, `locale`, `syncWorkspaceId`, `syncKey` (base64).

## 3. Calcul durée

```
grossMs = endMs - startMs  (ou now - startMs si endMs=0)
netMs = max(0, grossMs - breakMs)
earnings = (netMs / 3600000) * hourlyRate + reimburse - deduct
```

Heures sup : au-delà de `overtimeThresholdH` × 3600000 ms par période de paie.

## 4. Sync (optionnelle)

Réutilise le transport Colo Course : Nostr kind 4545, chiffrement XChaCha20, relais `wss://colo-apps.les-crevettes-cevenoles.fr`.

Payload JSON v1 type `punch` :

```json
{"v":1,"t":"delta","ws":"<workspaceId>","by":"<deviceId>",
 "entries":[{"id":"...", "project":"...", "start":..., "end":..., ...}]}
```

Merge LWW par champ versionné. Snap complet tous les 100 deltas ou 7 jours.

## 5. Export

- **CSV** : colonnes Date, Projet, Début, Fin, Pause (h), Net (h), Taux, Gains, Notes, Tags.
- **XLSX** : OOXML minimal via `core/xlsx.cpp` + zip store.

## 6. Schéma SQLite

Tables : `projects`, `time_entries`, `punch_state`, `settings`, `history`, `workspaces`, `outbox`, `seen_events`, `members`.

WAL mode, `PRAGMA foreign_keys=ON`.

## 7. Notifications

- Rappel clock-out : `reminderClockOutMin` après clock-in sans break/end.
- Canal Android réutilisé de Colo Course (`platformNotify`).

## 8. GPS

Compile flag `PUNCH_HAS_GPS`. Si activé et permission OK, enregistre lat/lon au clock-in/out.
