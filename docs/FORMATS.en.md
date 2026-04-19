# Output formats

🇫🇷 French version: [FORMATS.md](FORMATS.md)

All files are written to `/ext/apps_data/osm_logger/` on the microSD, in **append-only** mode.

## `points.jsonl` — JSON Lines

One JSON line per saved point:

```json
{"time":"2026-04-19T08:45:12Z","lat":48.431234,"lon":-0.093210,"alt":45.2,"hdop":1.3,"sats":9,"tag":"amenity=bench","note":""}
```

| Field  | Type     | Notes                                                            |
|--------|----------|------------------------------------------------------------------|
| `time` | string   | ISO-8601 UTC from the Flipper RTC                                |
| `lat`  | number   | Decimal degrees, 6 decimals (~11 cm precision)                   |
| `lon`  | number   | Decimal degrees, 6 decimals                                      |
| `alt`  | number   | Altitude in meters (MSL from GGA sentence), 1 decimal            |
| `hdop` | number   | HDOP at save time (2 → 4 m, 1 → 2 m, 5+ = rough)                 |
| `sats` | integer  | Number of satellites used in the fix                             |
| `tag`  | string   | OSM tag `key=value`, or multi-tag separated by `;` (e.g. `amenity=bench` or `amenity=bench;material=wood` when a preset variant adds an extra tag) |
| `note` | string   | Free text. May contain: user-typed note (via `Up` → TextInput), `photo:N` if `Auto photo ID` is on, `avg` if the point was saved through averaging mode. Combined example: `"wooden photo:12 avg"` |

**Forced point** (OK long without fix): `lat=0`, `lon=0`, `alt=0`, `hdop=99.9` → skip these at post-processing if needed.

**Averaged point**: saved via the Averaging mode (Settings → `Averaging` ≥ 5s). `lat`/`lon` are the mean of N samples collected during capture. `hdop` reflects the **best** HDOP observed during averaging (not the mean). The note contains `avg`.

## `notes.csv` — headerless CSV

Columns, in order: `time, lat, lon, alt, hdop, sats, tag, note`.

```csv
"2026-04-19T08:45:12Z",48.431234,-0.093210,45.2,1.3,9,"amenity=bench",""
```

Quotes around text fields for Excel/LibreOffice compatibility. `"` inside `note` is replaced by `'` to avoid breaking the parser.

> 💡 No header → if you open it in a spreadsheet, manually add a first row.

## `points.gpx` — GPX 1.1 (waypoints) with OsmAnd Favorites extensions

Valid XML at all times (after every OK). Enriched with the **OsmAnd Favorites** namespace (icon + color + category) for nice visual rendering in OsmAnd, while remaining 100% compatible with **JOSM**, **iD**, **QGIS**, **GPX Viewer** (which silently ignore unknown extensions).

```xml
<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.1" creator="Flipper Zero OSM Logger"
     xmlns="http://www.topografix.com/GPX/1/1"
     xmlns:osmand="https://osmand.net">
  <wpt lat="48.431234" lon="-0.093210">
    <ele>45.2</ele>
    <time>2026-04-19T08:45:12Z</time>
    <name>Bench</name>
    <desc>amenity=bench  HDOP=1.3 sats=9</desc>
    <type>Street furniture</type>
    <extensions>
      <osmand:icon>bench</osmand:icon>
      <osmand:background>circle</osmand:background>
      <osmand:color>#10c0f0</osmand:color>
    </extensions>
  </wpt>
</gpx>
```

**Field mapping**:
- `<name>`: **human-readable label** of the preset (e.g. `"Bench"`, `"Drinking water"`). Cleaner than the old format `amenity=bench`.
- `<desc>`: **technical details** → the full OSM tag + HDOP + satellite count.
- `<type>`: **category** (Street furniture, Roads & signs, Tourism, Emergency, …). OsmAnd uses this to group favorites into folders.
- `<extensions>`: OsmAnd display directives:
  - `icon` derived from the OSM primary value (e.g. `bench`, `atm`, `cafe`) — OsmAnd looks up a matching icon in its library, falling back to a generic one.
  - `background` = `circle` (pill shape around the icon).
  - `color` per category: e.g. red for Emergency, green for Nature, cyan for Street furniture.

**Import into OsmAnd**: File > Import > this `.gpx` → each point becomes a favorite with its colored pill and icon.

The app uses a *write-append-framed* strategy: on each save it opens the file in R/W, seeks just before `</gpx>`, truncates, writes the new `<wpt>` and rewrites the footer. So the file stays valid XML even after a power cut **between two saves**.

## `points.geojson` — GeoJSON FeatureCollection

Also valid at all times, ready for direct import into **QGIS**, **geojson.io**, **iD**, etc.

```json
{"type":"FeatureCollection","features":[
  {"type":"Feature","geometry":{"type":"Point","coordinates":[-0.093210,48.431234,45.2]},"properties":{"time":"2026-04-19T08:45:12Z","tag":"amenity=bench","hdop":1.3,"sats":9}}
]}
```

> ⚠️ GeoJSON convention: coordinates = `[lon, lat, alt]` (lon first, opposite of GPX).

## Optional post-processing

The [scripts/jsonl_to_geojson.py](../scripts/jsonl_to_geojson.py) script is still available if you prefer starting from JSONL (richer — keeps `note` and timestamp separate). Otherwise the native GeoJSON is directly usable.

```bash
python3 scripts/jsonl_to_geojson.py points.jsonl > points_from_jsonl.geojson
```

## `track.gpx` — Track mode (auto-log GPX)

Written by the **Track mode** entry in the main menu. A periodic timer (interval configurable via Settings → `Track interval`: 1 / 5 / 10 / 30 / 60 s) appends a `<trkpt>` on each tick as long as the view is active and a fix is available.

Two optional filters for track quality:
- `Track min dist` (off / 2 / 5 / 10 m): skip trkpts that didn't move enough since the previous one (avoids GPX polluted by red-light stops)
- `Track HDOP strict` (on/off): reject trkpts with HDOP above the configured threshold

Each entry into track mode starts a new `<trkseg>` (segment), so JOSM/QGIS won't draw a bogus line between two paused tracking sessions.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<gpx version="1.1" creator="Flipper Zero OSM Logger" xmlns="http://www.topografix.com/GPX/1/1">
<trk><name>Track</name>
<trkseg>
  <trkpt lat="48.431234" lon="-0.093210"><ele>45.2</ele><time>2026-04-19T08:45:00Z</time></trkpt>
  <trkpt lat="48.431267" lon="-0.093185"><ele>45.3</ele><time>2026-04-19T08:45:05Z</time></trkpt>
</trkseg>
<trkseg>
  <trkpt lat="48.432000" lon="-0.094000"><ele>47.0</ele><time>2026-04-19T08:55:00Z</time></trkpt>
</trkseg>
</trk>
</gpx>
```

Same **write-append-framed** strategy as `points.gpx`: the file stays valid XML after every trkpt written, even through a power cut.

Typical use: enable track mode before starting a bike ride, let it run. On arrival, import `track.gpx` into JOSM or a viewer to see the path, then add your OSM waypoints on top.

## `notes_cache.txt` — per-preset notes cache (internal)

Internal file, TAB-separated:
```
amenity=bench<TAB>wooden seat
amenity=drinking_water<TAB>public fountain
```

Filled automatically on each save when you typed a note via the `Up` editor. Next time you pick the same preset, the associated note is pre-loaded. Useful to avoid retyping `"out of order"` or `"material=wood"` on every bench.

You can edit it manually via qFlipper to pre-fill notes.

## Archive session

From **Main menu → Last points → Archive session**, the app moves the 5 current files (JSONL / CSV / GPX / GeoJSON / track.gpx) into a dated subfolder:

```
/ext/apps_data/osm_logger/session_20260419T143025/
  ├── points.jsonl
  ├── notes.csv
  ├── points.gpx
  ├── points.geojson
  └── track.gpx
```

After archiving, the session and total counters reset to zero, and the next session starts fresh. Handy to separate sessions (e.g. "Paris morning", "Brest afternoon").
