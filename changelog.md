# Changelog

## 0.2 — 2026-04-19

### Added
- **Mode Trace (auto-log GPX)** : une nouvelle vue avec un timer périodique (5 s) qui écrit des `<trkpt>` dans `track.gpx`. Chaque session démarre un nouveau `<trkseg>` pour ne pas tracer de ligne fictive entre deux sessions.
- **Presets dynamiques depuis la microSD** : `/ext/apps_data/osm_logger/presets.txt` (format `label;key;value[,variant1,...]`) remplace la liste compilée si présent.
- **Variantes de preset** : `←` / `→` cyclent entre plusieurs valeurs alternatives pour le même key OSM (ex. `amenity=cafe` → `pub` → `bar` → `fast_food`).
- **Éditeur de note** : `Up` dans Quick Log ouvre un `TextInput` Flipper, `Down` efface. La note est incluse dans les fichiers de sortie.
- **Notification fix acquis** : vibration + LED verte automatiques quand le GPS passe de no-fix à fix.
- **Statut GPS** devient une vue dédiée (plus un one-off action) avec toutes les infos GPS live + compteurs diagnostiques NMEA (octets reçus / lignes parsées).
- **About** : écran auteur + lien GitHub + licence.
- **Total cumulatif** : compte les lignes de `points.jsonl` au démarrage, affiché sur Quick Log.
- **Métadonnées Apps Catalog** : `fap_category`, `fap_version`, `fap_icon`, `fap_author`, `fap_weburl`, `fap_description`.
- **Doc** : nouveau `docs/ROADMAP.md`, `docs/DEVELOPMENT.md` et `docs/FORMATS.md` à jour.
- **Anglais** : toute l'UI (menu, footers, headers, noms des presets par défaut) passe en anglais pour la soumission au catalogue Flipper. `presets.txt.sample` reste en français comme exemple de personnalisation.

### Changed
- **Format JSONL / CSV** enrichi avec `alt`, `hdop`, `sats` en plus de `time/lat/lon/tag/note`.

## 0.1 — 2026-04-19

### Added
- **Mode Rapide (Quick Log)** : sous-menu de 23 presets OSM → écran live avec coords, altitude, HDOP, satellites, âge du fix, compteurs.
- **HDOP gate** : `OK` court refuse la sauvegarde si pas de fix ou HDOP > 2.5, `OK` long force.
- **5 formats natifs** écrits dans `/ext/apps_data/osm_logger/` :
  - `points.jsonl` (JSON Lines)
  - `notes.csv` (tableur)
  - `points.gpx` (waypoints GPX 1.1)
  - `points.geojson` (FeatureCollection)
  - L'écriture utilise un **write-append-framed** pour garder GPX et GeoJSON valides à tout moment.
- **Parser NMEA** minimal (RMC + GGA), sans `atof` / `strtok`, ISR-safe.
- **Gestion du service Expansion** : désactivé pendant que l'app tourne pour libérer l'UART.
- **Doc** : `README.md`, `docs/HARDWARE.md`, `docs/FORMATS.md`, `docs/DEVELOPMENT.md`.
