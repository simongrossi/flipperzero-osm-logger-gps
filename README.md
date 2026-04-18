# flipperzero-osm-logger-gps

> Logger de **POI OSM** sur le terrain avec **Flipper Zero** + **GPS NEO-6M V2** :
> choisir un type de point (banc, poubelle, fontaine, défibrillateur…) et le sauvegarder
> sur microSD avec position, altitude, timestamp et tag OSM.

![icône osm_logger](osm_logger.png)

---

## ✨ Fonctionnalités

- Lecture GPS **NMEA 0183** (RMC + GGA) via UART 9600
- Écran **Statut GPS** : fix, lat/lon, HDOP, satellites (LED + son selon fix)
- **Mode Rapide** :
  - Sous-menu de **23 presets OSM** classés par catégorie (mobilier urbain, voirie, stationnement, commerces, urgence…)
  - Écran live : coordonnées, altitude, HDOP, satellites, âge du dernier fix, compteur de points session
  - **`OK` court** : sauvegarde si `fix OK` et `HDOP ≤ 2.5`, sinon son d'erreur
  - **`OK` long** : force la sauvegarde quel que soit le fix (utile en canyon urbain)
- Sauvegarde dans `/ext/apps_data/osm_logger/` en **4 formats natifs** (tous valides à tout moment) :
  - `points.jsonl` — 1 ligne JSON / point, format dense
  - `notes.csv` — tableur
  - `points.gpx` — GPX 1.1 avec waypoints, import direct JOSM/iD
  - `points.geojson` — FeatureCollection, import direct QGIS/geojson.io
- Compatible Flipper **firmware stock** et **Momentum** (gère automatiquement le conflit UART avec le service Expansion)

---

## 🚀 Quickstart

**1. Installer `ufbt`** (outil officiel pour compiler les apps externes Flipper) :

```bash
python3 -m pip install --upgrade ufbt    # macOS / Linux
py -m pip install --upgrade ufbt         # Windows
```

**2. Cloner et compiler :**

```bash
git clone https://github.com/simongrossi/flipperzero-osm-logger-gps
cd flipperzero-osm-logger-gps
ufbt                   # produit ./dist/osm_logger.fap
```

**3. Flasher + lancer sur le Flipper** (USB branché, qFlipper fermé) :

```bash
ufbt launch
```

> 💡 Si `ufbt: command not found`, ajouter `$HOME/Library/Python/3.9/bin` (macOS) ou l'équivalent à ton `$PATH`.

**4. Câblage GPS** — voir [docs/HARDWARE.md](docs/HARDWARE.md). En résumé :

| NEO-6M | Flipper      |
|--------|--------------|
| VCC    | pin 9 (3V3)  |
| GND    | pin 11       |
| TX     | pin 14 (RX)  |

**⚠️ 3,3 V uniquement, pas 5 V.**

---

## ▶️ Utilisation

1. Lancer **OSM Logger** sur le Flipper.
2. Menu principal → `Mode rapide (Quick Log)`.
3. Choisir un preset dans la liste (Banc, Poubelle, Pharmacie…).
4. Écran **Quick Log** — attendre un fix (`fix=Xs` indique l'âge du dernier fix).

| Touche      | Action                                                       |
|-------------|--------------------------------------------------------------|
| `OK` court  | Enregistrer si fix OK et HDOP ≤ 2.5 (sinon son d'erreur)     |
| `OK` long   | Forcer l'enregistrement même sans fix ou avec HDOP dégradé   |
| `Back`      | Retour à la liste des presets                                |

5. En fin de session, récupérer les fichiers via qFlipper ou mode storage USB :
   - `/ext/apps_data/osm_logger/points.jsonl`
   - `/ext/apps_data/osm_logger/notes.csv`
   - `/ext/apps_data/osm_logger/points.gpx`
   - `/ext/apps_data/osm_logger/points.geojson`

Le menu principal propose aussi `Statut GPS` qui déclenche un log système + vibration/LED selon la qualité du fix.

### Anatomie de l'écran Quick Log

```
        Banc              <- preset sélectionné
     amenity=bench        <- tag OSM
   48.43123, -0.09321     <- coords (lat, lon)
   HDOP=1.3 sats=9  #12   <- qualité + compteur session
   alt=45m  fix=3s        <- altitude + âge du fix
  OK save  Hold=force     <- rappel touches
```

---

## 📚 Documentation

- **[docs/HARDWARE.md](docs/HARDWARE.md)** — modules GPS supportés, câblage détaillé, premier fix, diagnostic "pas de données NMEA"
- **[docs/FORMATS.md](docs/FORMATS.md)** — specs des fichiers JSONL/CSV, champs par champ, post-traitement
- **[docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)** — structure du code, ajouter un preset, ajouter une vue, debug, pièges connus
- **[src/presets.c](src/presets.c)** — liste complète des 23 presets (éditable, recompilation requise)

---

## 🗺️ Workflow OSM

- **JOSM / iD** : importer `points.gpx` directement (waypoints nommés avec le tag OSM).
- **QGIS / geojson.io** : importer `points.geojson` directement.
- **Notes OSM** : `notes.csv` fournit une base texte à poster manuellement.
- **Alternative** : `python3 scripts/jsonl_to_geojson.py points.jsonl > points.geojson` si tu préfères repartir du JSONL (conserve note + fix age).

---

## 🚧 Pas encore implémenté

- **Mode trace GPX** (log auto périodique pour enregistrer un parcours en déplacement).
- **Éditeur de note** on-device (text input custom).
- **Chargement dynamique de `presets.json`** depuis la microSD.
- **Variantes de preset** via ←/→ (ex. `bench` + `material=wood`).
- **Support autres modules GPS** (PA1010D, BN-180 — devrait fonctionner mais non testé).

---

## 🖊️ Licence

MIT — voir [LICENSE](LICENSE).
