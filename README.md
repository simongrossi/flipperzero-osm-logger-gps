# flipperzero-osm-logger-gps

> Logger de **POI OSM** sur le terrain avec **Flipper Zero** + **GPS NEO‑6M V2** : choisir un type de point (banc, poubelle, fontaine…), ajouter une note courte, et sauvegarder sur microSD en **JSONL**, **CSV (notes)**, **GPX**, **GeoJSON**.

## ✨ Fonctionnalités
- Lecture GPS **NMEA 0183** (RMC/GGA) via **UART 9600**
- Écran statut : fix, lat/lon, HDOP, satellites
- **Menu de presets OSM** (modifiable) + **note courte**
- Sauvegarde multi‑formats sur `/ext/apps_data/osm_logger/`
  - `points.jsonl`
  - `notes.csv`
  - `points.gpx`
  - `points.geojson`

⚠️ **Alpha** : code minimal, GPX/GeoJSON en append naïf.

## 🔧 Build & Installation
1. Installer le **Flipper SDK** (`fbt`).
2. Cloner ce repo dans `applications_user/` :
   ```bash
   git clone https://github.com/simongrossi/flipperzero-osm-logger-gps $FLIPPER_SDK/applications_user/flipperzero-osm-logger-gps
   ```
3. Compiler :
   ```bash
   cd $FLIPPER_SDK
   ./fbt fap_dist
   ```
4. Copier le `.fap` généré vers `/ext/apps/Utilities/`.

## 🖊️ Licence
MIT — voir `LICENSE`.
