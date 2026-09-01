# Open_punch_clock — Wireframes UX

## PunchPage (écran principal)

```
┌─────────────────────────────┐
│  Open Punch Clock      ⚙   │
├─────────────────────────────┤
│  Projet: [Client ABC    ▼]  │
│                             │
│       ┌─────────────┐       │
│       │  02:34:17   │       │  ← timer live (64px)
│       └─────────────┘       │
│       ~ 42,50 €             │
│                             │
│  ┌───────────────────────┐  │
│  │     PUNCH IN          │  │  72px, vert — visible si !clockedIn
│  └───────────────────────┘  │
│                             │
│  ┌──────────┐ ┌──────────┐  │
│  │  BREAK   │ │ PUNCH OUT│  │  64px, espacement fixe 16px
│  └──────────┘ └──────────┘  │  visible si clockedIn
│                             │
│  [Historique] [Rapports]    │
└─────────────────────────────┘
```

**Règle G10** : boutons In/Out/Break jamais recouverts par bannière ; zone fixe en bas.

## HistoryPage

Entrées groupées par jour (Aujourd'hui / Hier / date). Swipe ou menu → éditer / supprimer.

## ProjectsPage

Liste projets + FAB créer. Édition : nom, taux horaire, couleur, défaut.

## TimeCardEditor

Formulaire : projet, date, heures début/fin, pause, notes, tags, remb./déduction.

## ReportsPage

Onglets Semaine / Mois. Totaux par projet. Bouton Export CSV / XLSX.

## SettingsPage

Rappels, période de paie, heures sup, GPS, sync, thème, à propos.
