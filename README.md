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
- **Mode Rapide** : carrousel de presets (↑/↓), log immédiat avec OK, 
  appui long = forcer malgré un HDOP élevé ou saisir une note courte.

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
   git clone https://github.com/simongrossi/flipperzero-osm-logger-gps        $FLIPPER_SDK/applications_user/flipperzero-osm-logger-gps
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

### Mode Rapide
1. Menu principal → `Mode rapide`
2. Utiliser les touches selon ce schéma :

| Touche     | Action                                                          |
|------------|-----------------------------------------------------------------|
| ↑ / ↓      | Changer de preset                                               |
| ← / →      | Changer de variant (si disponible)                              |
| OK         | Enregistrement immédiat (si fix OK et HDOP ≤ 2.5 par défaut)    |
| OK long    | Forcer l’enregistrement malgré précision faible ou ajouter note |
| BACK       | Retour au menu principal                                        |

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
- [ ] Choix fréquence log (auto vs manuel, mode trace GPX)  
- [ ] Support autres modules GPS (PA1010D, etc.)  
- [ ] Export direct vers Notes OSM via API  
- [ ] UI améliorée (icônes, preview)  
- [ ] Améliorer l’UI du Mode Rapide (toasts, vibration, feedback visuel)  
- [ ] Support d’un éditeur de note plus long (pas seulement 12 caractères)  
- [ ] Intégration des variantes de presets  

---

## 🖊️ Licence
MIT — voir [LICENSE](LICENSE).
