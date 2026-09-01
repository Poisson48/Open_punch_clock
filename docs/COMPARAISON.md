# Open Punch Clock vs Time Squared — état des lieux

> Rapport au **1 septembre 2026** — version Open Punch Clock **v0.1.3**  
> Référence propriétaire : **Time Squared / Time Clock: Easy Tracker** v3.4.x (`co.timesquared.timetracker`)  
> Sources : [timesquared.co](https://timesquared.co/), fiche Play Store, APK de référence dans `reference/` (gitignored).

---

## 1. Synthèse exécutive

| Critère | Time Squared | Open Punch Clock v0.1.3 |
|---|---|---|
| **Modèle économique** | Freemium + pub + abonnements (individuel / équipe) | Gratuit, GPLv3, sans pub ni achat in-app |
| **Données** | Cloud propriétaire, analytics (`AD_ID`, services pub) | Local-first, SQLite sur l'appareil ; sync E2E chiffrée **optionnelle** (Nostr, pas de télémétrie) |
| **Plateformes** | iOS, Android, web (gestion équipe) | Android + **Linux desktop** ; pas d'iOS |
| **Usage solo / freelance** | Complet (avec limites gratuites) | **~80 % de parité** sur le cœur pointeuse + feuille de temps |
| **Usage équipe / employeur** | Compte Team, rôles, vue manager | **Non couvert** (backlog v2+) |
| **Maturité produit** | App store depuis des années, polish, widget, intégrations cloud | MVP solide, release récente, polish et intégrations à finaliser |

**Verdict :** Open Punch Clock est déjà une alternative crédible pour un **travailleur solo** qui veut éviter pub, abonnement et cloud opaque. Il **ne remplace pas** Time Squared pour une **PME** qui gère une équipe, des permissions, ou des workflows paie / facturation avancés.

---

## 2. Tableau fonctionnel détaillé

Légende : ✅ implémenté · 🟡 partiel · ❌ absent · 💰 premium Time Squared

### 2.1 Pointeuse et saisie du temps

| Fonction | Time Squared | Open Punch Clock |
|---|---|---|
| Clock In / Out en un tap | ✅ | ✅ |
| Pause (break) one-tap | ✅ | ✅ |
| État « en service » persistant (crash / reboot) | ✅ | ✅ |
| Time cards manuelles | ✅ | ✅ |
| Ajustement heure de début / fin | ✅ | ✅ (éditeur) |
| Tags et notes sur entrée | ✅ | ✅ |
| Remboursements / déductions | 💰 / ✅ | ✅ |
| Widget Android (sans ouvrir l'app) | ✅ | ❌ |
| Vibrations / retour haptique | ✅ | 🟡 (code `platformVibrate`, peu branché sur punch) |
| Écran allumé sur l'écran pointeuse | — | 🟡 (API `keepScreenOn`, non exposée UI) |

### 2.2 Projets, gains et rapports

| Fonction | Time Squared | Open Punch Clock |
|---|---|---|
| Projets / clients / jobs | ✅ | ✅ |
| Taux horaire par projet | ✅ | ✅ |
| Estimation gains en temps réel | ✅ | ✅ |
| Rapports hebdo / mensuels | ✅ | ✅ |
| Période de paie configurable | ✅ | ✅ |
| Seuil heures sup | ✅ | ✅ |
| Overtime / taxes / retenues sur entrée | 💰 | ❌ |
| Pourboires, frais kilométriques | 💰 | ❌ |
| Statut payé / impayé | 💰 | ❌ |
| Photos jointes aux entrées | 💰 | ❌ |

### 2.3 Export et partage

| Fonction | Time Squared | Open Punch Clock |
|---|---|---|
| Export XLSX | ✅ | ✅ |
| Export CSV | ✅ | ✅ |
| Partage email / messagerie | ✅ | 🟡 (écrit dans Documents ; pas de share sheet depuis Rapports) |
| Enregistrement Google Drive / Dropbox | ✅ | ❌ |
| Import depuis autre app | — | ❌ (backlog) |

### 2.4 Sync, sauvegarde, multi-appareils

| Fonction | Time Squared | Open Punch Clock |
|---|---|---|
| Sync cloud automatique | ✅ (serveur TS) | 🟡 (sync Nostr E2E **optionnelle**, relais colo-apps) |
| iOS + Android + web | ✅ | ❌ (Android + Linux seulement) |
| Sauvegarde cloud automatique | ✅ | ❌ |
| Backup / restore chiffré local | — | ❌ (ZIP prévu, `FilePickers` Colo non branché) |
| Comptes équipe (owner / manager / employé) | ✅ | ❌ |
| « Qui est en ligne » côté manager | ✅ | ❌ |

### 2.5 Localisation et vie privée

| Fonction | Time Squared | Open Punch Clock |
|---|---|---|
| GPS au punch | 💰 | 🟡 (opt-in UI + code ; build sans Qt Positioning = désactivé) |
| Géofencing / suivi live | 💰 Team | ❌ |
| Pub et tracking publicitaire | ✅ (gratuit) | ❌ (aucun) |
| Biométrie / verrou app | ✅ | ❌ |
| Langues | Multiples (store) | ✅ **16 langues** (Qt Linguist) |

### 2.6 Notifications

| Fonction | Time Squared | Open Punch Clock |
|---|---|---|
| Rappel clock-out oublié | ✅ | ✅ |
| Rappel début de shift | ✅ | 🟡 (seulement rappel après X min clock-in) |
| Alarmes exactes (Android) | ✅ | ❌ |

### 2.7 Distribution et technique

| Fonction | Time Squared | Open Punch Clock |
|---|---|---|
| Play Store / App Store | ✅ | ❌ (APK direct + colo-apps) |
| Mises à jour in-app | ✅ | ✅ (manifest colo-apps) |
| Linux desktop | ❌ | ✅ AppImage |
| Code source | ❌ | ✅ GPLv3 |
| Taille install (~) | ~112 Mo (XAPK) | ~27 Mo (APK v0.1.3) |

---

## 3. Avantages d'Open Punch Clock

1. **Liberté et transparence** — code ouvert, pas de vendor lock-in, pas de pub.
2. **Vie privée** — données par défaut sur l'appareil ; sync chiffrée de bout en bout si activée (modèle différent du cloud Time Squared).
3. **Coût zéro** — pas d'abonnement individuel ni de compte équipe payant.
4. **Desktop Linux** — usage bureau natif (Time Squared n'a que mobile + web manager).
5. **i18n large** — 16 langues dès v0.1.3 (Time Squared est surtout orienté marchés anglophones + localisations store).
6. **Léger** — APK ~4× plus petit que le XAPK Time Squared analysé.

---

## 4. Lacunes prioritaires (par rapport à Time Squared)

Classées par impact utilisateur solo :

| Priorité | Manque | Effort estimé |
|---|---|---|
| **P1** | Widget Android clock in/out | Moyen |
| **P1** | Partage natif (share sheet) après export XLSX/CSV | Faible |
| **P1** | Finition Android (nom app, package `org.colocourse.app` → identité Open Punch Clock) | Faible |
| **P2** | Backup / restore ZIP chiffré (export complet) | Moyen |
| **P2** | GPS fonctionnel sur builds release (Qt Positioning dans CI Android) | Moyen |
| **P2** | Collage presse-papier du lien d'invitation sync | Faible |
| **P3** | Export sur période personnalisée (pas seulement semaine) | Faible |
| **P3** | Rappels début de shift configurables | Faible |
| **v2** | Comptes équipe / rôles / vue manager | Élevé |
| **v2** | Kilométrage, photos, statut payé, taxes | Élevé |
| **v2** | iOS (Qt ou autre) | Très élevé |
| **v2** | Intégration Drive / Dropbox | Moyen |

---

## 5. Ce qui est « faux positif » dans le plan interne

Le [PLAN.md](PLAN.md) marque certaines tâches ✅ alors que l'implémentation est incomplète ou héritée de Colo Course :

| Tâche cochée | Réalité v0.1.3 |
|---|---|
| 7.1 Widget Android | ❌ Aucun widget dans `android/` |
| 6.5 Backup ZIP chiffré | ❌ `FilePickers.qml` référence `exportAllZip` (API Colo) non exposée |
| 7.3 Traductions FR/EN | ✅ Dépassé : 16 langues |
| 4.4 Keep-screen-on PunchPage | 🟡 API présente, non câblée QML |
| 4.5 Vibrations punch | 🟡 API présente, non appelée dans `PunchEngine` |

Ce document reflète l'état **réel** du code, pas uniquement le plan.

---

## 6. Positionnement recommandé

**Cible immédiate :** freelancers, artisans, étudiants, petites structures où **une personne** gère sa feuille de temps et exporte un XLSX pour la compta — en refusant pub, abonnement et cloud non audité.

**Ne pas promettre (encore) :** remplacement de Time Squared **Team**, paie RH, géolocalisation employés, conformité multi-sites.

**Message clé vs concurrence :** *« Même cœur pointeuse + feuille de temps, sans pub ni abo, vos données restent les vôtres — et le code aussi. »*

---

## 7. Références

- Time Squared : https://timesquared.co/
- Play Store : https://play.google.com/store/apps/details?id=co.timesquared.timetracker
- APK référence locale : `reference/README.md` (v3.4.1746, non versionné)
- Release Open Punch Clock : https://github.com/Poisson48/Open_punch_clock/releases
