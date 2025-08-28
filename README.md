# flipperzero-osm-logger-gps

> Logger de **POI OSM** sur le terrain avec **Flipper Zero** + **GPS NEO-6M V2** :  
> choisir un type de point (banc, poubelle, fontaine…), ajouter une note courte,  
> et sauvegarder sur microSD en **JSONL**, **CSV (notes)**, **GPX**, **GeoJSON**.

---

## ✨ Fonctionnalités
- Lecture GPS **NMEA 0183** (RMC/GGA) via **UART 9600**
- Écran statut : fix, lat/lon, HDOP, satellites
- **Menu de presets OSM** (modifiable) + **note courte**
- Sauvegarde multi-formats sur `/ext/apps_data/osm_logger/`
  - `points.jsonl` (facile à retraiter)
  - `notes.csv` (texte prêt pour Notes OSM)
  - `points.gpx` (waypoints)
  - `points.geojson` (Features simplifiées)
- Script d’exemple pour convertir `JSONL → GeoJSON` propre

> ⚠️ **Alpha** : code minimal, GPX/GeoJSON en *append naïf* (voir TODO pour headers/footers propres).

---

## 🧩 Matériel
- **Flipper Zero** (avec microSD)
- **GPS NEO-6M V2** (u-blox NEO-6) — sortie **NMEA** 1 Hz, 9600 bauds
- Antenne céramique (incluse) / active (option)

### Câblage (3.3V recommandé)
- **VCC 3V3** (Flipper) → **VCC** (NEO-6M)  
- **GND** ↔ **GND**  
- **TX (NEO-6M)** → **RX UART (Flipper)**

> Le TX du GPS doit être en **3.3V**. Évitez toute ligne **5V** vers le Flipper.

---

## 🔧 Build & Installation
1. Installer le **Flipper SDK** (`fbt`).
2. Cloner ce repo dans l’arborescence du SDK, dossier `applications_user/` :
   ```bash
   git clone https://github.com/<votre_compte>/flipperzero-osm-logger-gps \
       $FLIPPER_SDK/applications_user/flipperzero-osm-logger-gps
   ```
3. Compiler :
   ```bash
   cd $FLIPPER_SDK
   ./fbt fap_dist
   ```
4. Copier le `.fap` généré vers `/ext/apps/Utilities/` sur la microSD.

---

## ▶️ Utilisation
1. Lancer **OSM Logger** sur le Flipper.  
2. Attendre `Fix: OK` (ciel dégagé recommandé).  
3. `OK` → choisir un preset → saisir une note (optionnel) → **Valider**.  
4. En fin de session, brancher le Flipper en stockage USB et récupérer les fichiers :  
   - `/ext/apps_data/osm_logger/points.jsonl`  
   - `/ext/apps_data/osm_logger/notes.csv`  
   - `/ext/apps_data/osm_logger/points.gpx`  
   - `/ext/apps_data/osm_logger/points.geojson`  

---

## 🗂️ Formats

### JSONL
```json
{"ts":"2025-08-28T08:45:12Z","lat":48.431234,"lon":-0.093210,"src":"GPS","tags":{"amenity":"bench"},"note":"banc en bon état","hdop":1.3,"sat":9}
```

### CSV (notes)
```csv
ts,lat,lon,text
2025-08-28T08:45:12Z,48.431234,-0.093210,"amenity=bench | banc en bon état"
```

### GPX (waypoints, append naïf)
Chaque enregistrement ajoute un `<wpt …/>`.  
Un script ultérieur pourra encapsuler dans `<gpx>…</gpx>`.

### GeoJSON (Features en lignes)
Chaque enregistrement ajoute une Feature JSON.  
Un script peut encapsuler dans une `FeatureCollection`.

---

## 🧰 Presets
Les presets peuvent être recompilés en dur ou chargés depuis `presets.json` (optionnel) placé sur la microSD :

```json
[
  {"label":"Banc","tags":{"amenity":"bench"}},
  {"label":"Poubelle","tags":{"amenity":"waste_basket"}},
  {"label":"Fontaine","tags":{"amenity":"drinking_water"}},
  {"label":"Panier de basket","tags":{"amenity":"basketball_hoop"}},
  {"label":"Toilettes","tags":{"amenity":"toilets"}}
]
```

---

## 🗺️ Workflow OSM (exemples)
- **JOSM** : importer `points.gpx` en waypoints pour vérifier, puis tracer/éditer.  
- **MapComplete / Notes OSM** : utiliser `notes.csv` comme base de texte à poster.  
- **Scripts** : convertir `points.jsonl` → formats propres (voir `scripts/jsonl_to_geojson.py`).  

---

## ✅ Roadmap
- [ ] Headers/footers corrects pour GPX/GeoJSON  
- [ ] Choix fréquence log (auto vs manuel)  
- [ ] Support autres modules GPS (PA1010D, etc.)  
- [ ] Export direct vers Notes OSM via API  
- [ ] UI améliorée (icônes, preview)  

---

## 🖊️ Licence
MIT — voir [LICENSE](LICENSE).

---

## 📄 LICENSE (MIT)
```text
MIT License

Copyright (c) 2025 Simon

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
