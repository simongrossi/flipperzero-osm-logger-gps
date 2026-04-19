# Changelog

## 0.5 — 2026-04-19

### Added
- **Détection de doublons** : au save, si un point avec le même tag existe à moins de X mètres, afficher un écran de confirmation ("Same tag Ym away — OK=save anyway, Back=cancel"). Seuil configurable dans Settings : `Dup check` = off / 5m / 10m / 25m (défaut 10m).
- **Écrans d'erreur explicites** : si le save échoue (SD absente, dossier impossible à créer, disque plein), un overlay rouge affiche `SAVE FAILED` + message précis. Une touche pour dismiss.
- **Vérification pré-save** : avant chaque sauvegarde, check de la présence SD + espace libre (seuil 10 Ko). Évite les échecs silencieux.

### Fixed
- Le tag multi-ligne (`amenity=bench;material=wood`) était wrappé par `elements_multiline_text_aligned` par-dessus la ligne coords. Maintenant splitté proprement sur 2 lignes en Quick Log et Preview, les autres lignes décalées en conséquence.

### Changed
- `settings.txt` ajoute `duplicate_check_m` (entier en mètres, 0 = désactivé).

## 0.4 — 2026-04-19

### Added
- **Notes persistantes par preset** : la note saisie pour un preset (ex. "wooden bench") est sauvegardée dans `notes_cache.txt` sur la SD et rechargée automatiquement à la prochaine sélection du même preset.
- **Auto photo ID** (toggle Settings) : append automatique de `photo:N` dans la note au save (N = total cumulatif + 1). Pratique pour aligner les saves Flipper avec des photos téléphone prises en parallèle.
- **Tutoriel OSM pour débutants** : `docs/GETTING_STARTED_OSM.md` — guide complet de A à Z pour un premier contributeur OSM (câblage, app, premier fix, logger un POI, récupérer les fichiers, importer dans JOSM / iD, bonnes pratiques).

### Changed
- `settings.txt` ajoute la clé `auto_photo_id`.
- Nouveau fichier `/ext/apps_data/osm_logger/notes_cache.txt` (format TAB-séparé `primary_tag<TAB>note`).

## 0.3 — 2026-04-19

### Added
- **Last points browser** : nouvelle vue "Last points (browse/undo)" accessible depuis le menu principal. Liste les 10 derniers saves sous la forme `HH:MM  tag`. Actions en bas de liste : **Delete last** (supprime le dernier point des 4 fichiers : JSONL, CSV, GPX, GeoJSON) et **Clear all** (wipe de tous les fichiers de points + reset des compteurs).
- **Point detail view** : clic sur un point dans la liste Last Points → écran avec détails complets (time ISO, tag, lat/lon 6 décimales, altitude, HDOP, sats, note).
- **Variantes = tags additionnels** : un preset peut désormais avoir des variantes qui ajoutent un tag secondaire (pas juste une valeur alternative). Exemple `Bench` → `amenity=bench` / `amenity=bench;material=wood` / `amenity=bench;material=metal`. Les variantes contenant `=` deviennent des tags additionnels, les autres restent des valeurs alternatives.
- **Filtre de distance en mode trace** : setting `Track min dist` (off / 2m / 5m / 10m) — skip les trkpts trop proches du précédent. Calcul haversine approx (équirectangulaire, <1% erreur à l'échelle km).
- **HDOP strict en mode trace** : toggle `Track HDOP strict` dans Settings — rejette les trkpts quand HDOP > 2.5 (comme le gate Quick Log).
- **Preview avant save** : toggle `Preview save` dans Settings. OK court sur Quick Log ouvre une vue de confirmation avec tag/coords/HDOP/note avant le vrai save. OK long court-circuite le preview.
- **Cap / heading** parsé depuis `$GPRMC` champ 8, affiché en mode trace (`hdg=225°`).
- **Badge batterie** `%%` en haut à droite sur Quick Log, Track et GPS Status.

### Changed
- **Tag multi-valué** dans les fichiers de sortie : quand une variante additionnelle est utilisée, le champ `tag` contient maintenant des tags séparés par `;` (ex. `amenity=bench;material=wood`).
- **`settings.txt`** : nouveau fichier `/ext/apps_data/osm_logger/settings.txt` persiste les préférences (baud_rate, track_interval_s, track_min_dist_m, track_hdop_strict, preview_before_save).
- **Parser NMEA** : extraction de lat/lon depuis GGA aussi (pas seulement RMC) — corrige un bug où les coordonnées restaient à 0 quand GGA indiquait un fix mais RMC pas encore.

### Fixed
- Cooldown de 10 s sur la notification "fix acquired" pour éviter le spam si le fix flippe entre trames RMC et GGA.

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
