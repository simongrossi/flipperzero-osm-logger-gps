# Formats de sortie

Tous les fichiers sont écrits dans `/ext/apps_data/osm_logger/` sur la microSD, en mode **append-only**.

## `points.jsonl` — JSON Lines

Une ligne JSON par point sauvegardé :

```json
{"time":"2026-04-19T08:45:12Z","lat":48.431234,"lon":-0.093210,"alt":45.2,"hdop":1.3,"sats":9,"tag":"amenity=bench","note":""}
```

| Champ | Type      | Notes                                                           |
|-------|-----------|-----------------------------------------------------------------|
| `time` | string   | ISO-8601 UTC depuis le RTC du Flipper                           |
| `lat`  | number   | Degrés décimaux, 6 décimales (~11 cm de précision)              |
| `lon`  | number   | Degrés décimaux, 6 décimales                                    |
| `alt`  | number   | Altitude en mètres (MSL depuis trame GGA), 1 décimale           |
| `hdop` | number   | HDOP au moment du save (2 → 4 m, 1 → 2 m, 5+ = douteux)         |
| `sats` | integer  | Nb de satellites utilisés pour le fix                           |
| `tag`  | string   | Tag OSM formaté `key=value` (ex. `amenity=bench`)               |
| `note` | string   | Note libre (toujours vide pour l'instant, éditeur non implémenté)|

**Point forcé** (OK long sans fix) : `lat=0`, `lon=0`, `alt=0`, `hdop=99.9` → ignorer ces points au post-traitement si besoin.

## `notes.csv` — CSV sans header

Colonnes : `time, lat, lon, alt, hdop, sats, tag, note` dans cet ordre.

```csv
"2026-04-19T08:45:12Z",48.431234,-0.093210,45.2,1.3,9,"amenity=bench",""
```

Guillemets doublés autour des champs texte pour Excel/LibreOffice. Les `"` dans `note` sont remplacés par `'` pour ne pas casser le parsing.

> 💡 Pas de header → si tu imports dans un tableur, ajoute une première ligne manuellement.

## `points.gpx` — GPX 1.1 (waypoints)

Fichier XML valide à tout moment (après chaque OK). Importable directement dans **JOSM**, **iD**, **QGIS**, **GPX Viewer**, etc.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.1" creator="Flipper Zero OSM Logger" xmlns="http://www.topografix.com/GPX/1/1">
  <wpt lat="48.431234" lon="-0.093210">
    <ele>45.2</ele>
    <time>2026-04-19T08:45:12Z</time>
    <name>amenity=bench</name>
    <desc>HDOP=1.3 sats=9</desc>
  </wpt>
</gpx>
```

L'app utilise un *write-append-framed* : à chaque save elle ouvre le fichier en R/W, seek juste avant `</gpx>`, truncate, écrit le nouveau `<wpt>` et réécrit le footer. Le fichier reste donc valide XML même après une coupure de courant **entre deux saves**.

## `points.geojson` — GeoJSON FeatureCollection

Également valide à tout moment, prêt pour import direct dans **QGIS**, **geojson.io**, **iD**, etc.

```json
{"type":"FeatureCollection","features":[
  {"type":"Feature","geometry":{"type":"Point","coordinates":[-0.093210,48.431234,45.2]},"properties":{"time":"2026-04-19T08:45:12Z","tag":"amenity=bench","hdop":1.3,"sats":9}}
]}
```

> ⚠️ Convention GeoJSON : coordinates = `[lon, lat, alt]` (lon d'abord, contraire du GPX).

## Post-traitement optionnel

Le script [scripts/jsonl_to_geojson.py](../scripts/jsonl_to_geojson.py) reste dispo si tu préfères partir du JSONL (plus riche — garde `note` et timestamp séparés). Sinon le GeoJSON natif est directement utilisable.

```bash
python3 scripts/jsonl_to_geojson.py points.jsonl > points_from_jsonl.geojson
```

## Pas encore produit par l'app

- **Tracks GPX** (log auto en mouvement pour enregistrer un parcours). Seuls les waypoints sont implémentés.
