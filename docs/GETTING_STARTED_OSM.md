# Getting started with OSM Logger — first contribution guide

Ce guide t'accompagne **de zéro** jusqu'à ton premier POI ajouté sur OpenStreetMap avec l'app OSM Logger. Aucune connaissance préalable d'OSM n'est requise.

---

## 🗺️ Pourquoi contribuer à OSM ?

**OpenStreetMap** est une carte du monde libre, construite par des millions de contributeurs bénévoles. Quand tu ajoutes un banc, une fontaine ou un défibrillateur, ta donnée est **immédiatement utilisable** par :
- Les apps de cartographie (Organic Maps, OsmAnd, MapComplete…)
- Les services de livraison, logistique, secours (SOS Amitié a sauvé des vies grâce aux défibrillateurs mappés)
- Les chercheurs, les urbanistes, les touristes

OSM Logger te permet d'enrichir cette carte **sans sortir ton téléphone** et **sans être connecté** — tu enregistres sur le terrain, tu importes plus tard au calme.

---

## 🧩 Ce qu'il te faut

- Un Flipper Zero
- Un module GPS NMEA (NEO-6M V2 conseillé, ~5€ sur AliExpress)
- 3 fils dupont femelle-femelle
- 5 min pour câbler
- 5 min pour installer l'app
- Une microSD dans le Flipper (déjà là normalement)

**Pas besoin** : de compte OSM pour logger sur le terrain. Tu n'en auras besoin qu'**au moment d'uploader** tes contributions.

---

## 🔧 Étape 1 — Brancher le GPS

3 fils, aucun risque si tu respectes le 3,3 V :

| NEO-6M | Fil       | Flipper      |
|--------|-----------|--------------|
| VCC    | 🔴 rouge  | pin 9 (3V3)  |
| GND    | ⚫ noir   | pin 11       |
| TX     | 🟢 vert   | **pin 14** (RX) |

⚠️ **Ne connecte pas** le NEO-6M en 5 V. Son TX sortirait en 5 V et pourrait griller l'UART du Flipper.

Le schéma complet : [docs/HARDWARE.md](HARDWARE.md).

---

## 📲 Étape 2 — Installer l'app

**Méthode 1 (recommandée)** : via le [Flipper Apps Catalog](https://lab.flipper.net) → chercher "OSM Logger" → Install.

**Méthode 2 (développeur)** : compile depuis le source :

```bash
python3 -m pip install --upgrade ufbt
git clone https://github.com/simongrossi/flipperzero-osm-logger-gps
cd flipperzero-osm-logger-gps
ufbt launch
```

---

## 🛰️ Étape 3 — Premier fix GPS

1. Lance l'app : **Apps → GPIO → OSM Logger**
2. Va **dehors**, ciel dégagé (pas sous un arbre, pas en ville dense)
3. Attends 30 s à 2 min au premier démarrage (le GPS télécharge ses éphémérides)
4. Dans le menu, ouvre **GPS status** pour suivre le fix en live
5. Quand la LED verte clignote + vibration → **fix acquis** 🎉

Quelques repères sur l'écran GPS Status :
- `FIX OK` : le GPS a une position valide
- `sats=N` : nombre de satellites utilisés (4+ = ok, 7+ = très bien)
- `HDOP` : précision horizontale (1-2 = excellent, 2-5 = correct, 5+ = douteux)
- `NMEA: X B / Y lines` : données brutes reçues (utile si le GPS ne capte rien)

---

## 🎯 Étape 4 — Logger un POI

1. Menu principal → **Quick Log (waypoints)**
2. Choisis un preset dans la liste (ex. **Bench** pour un banc, **Waste basket** pour une poubelle)
3. Tu arrives sur l'écran Quick Log :
   - En haut : le nom du preset (ex. `Bench 2/4`) et son tag OSM (`amenity=bench;material=wood`)
   - En bas : les coordonnées live + compteur de saves
4. Touches à retenir :

| Touche      | Action                                                       |
|-------------|--------------------------------------------------------------|
| `←` / `→`   | Cycler entre les variantes (wood / metal / concrete…)        |
| `Up`        | Éditer une note courte (ex. "vandalisé", "en bois")          |
| `Down`      | Effacer la note                                              |
| `OK` court  | Sauvegarder (si fix bon, HDOP ≤ 2.5)                         |
| `OK` long   | Forcer la sauvegarde même si le fix est dégradé              |

**Règle d'or** : **positionne-toi à côté du POI** (moins de 2 m) avant de presser OK. Le GPS a une précision de l'ordre de 2-5 m.

---

## 🏠 Étape 5 — Logger des numéros de rue

L'app inclut un preset spécial **House number** (catégorie Address). Il sauve un tag placeholder `addr:housenumber=TBD`. Workflow :

1. Stationne-toi devant la maison **#23** rue Victor Hugo
2. Quick Log → catégorie **Address** → **House number**
3. Appuie `Up` pour ouvrir l'éditeur de note, tape **`23`**, OK
4. Retour Quick Log → OK pour sauvegarder → tu as un point avec `addr:housenumber=TBD` et `note=23`
5. Au post-traitement (JOSM), remplace `TBD` par `23` sur chaque point, supprime la note

Encore mieux : dans **JOSM**, utilise le plugin `utilsplugin2` qui permet de générer les numéros en masse à partir d'un champ.

Preset **Building** et **Building entry** sont aussi dispos pour enrichir les adresses.

## 📸 Étape 6 — Workflow avec photos (optionnel)

Si tu prends aussi des photos pour documenter, active la feature **Auto photo ID** dans Settings. À chaque save, l'app ajoute automatiquement `photo:<N>` dans la note (N incrémenté). Tu fais tes photos **dans le même ordre**, et à l'import tu sais quelle photo correspond à quel point.

**Exemple** :
- Point 1 : `photo:1` → ta photo 1 = le banc #1
- Point 2 : `photo:2` → ta photo 2 = la poubelle
- ...

Tes photos gardent leurs noms `IMG_1234.jpg` mais tu peux les aligner sur les numéros du Flipper avec un script à l'arrivée.

---

## 💾 Étape 7 — Récupérer les données

À la fin de la balade :

**Via qFlipper** (desktop app officielle) :
1. Branche le Flipper en USB
2. Lance qFlipper → **File Manager** → navigue vers `/ext/apps_data/osm_logger/`
3. Télécharge ces fichiers vers ton ordi :
   - `points.gpx` — pour JOSM / iD
   - `points.geojson` — pour QGIS / geojson.io
   - `points.jsonl` — format riche, avec la note / timestamp complets
   - `notes.csv` — pour Excel
   - `track.gpx` — ton itinéraire (si tu as utilisé Track mode)

**Sans ordi, mode USB** : depuis le Flipper, `Apps → Settings → Storage → Mass Storage mode` → ton Flipper apparaît comme une clé USB.

---

## ✏️ Étape 8 — Importer dans JOSM

[JOSM](https://josm.openstreetmap.de/) est l'éditeur OSM de référence, gratuit, puissant.

1. Installe JOSM (Java 8+ requis)
2. **File → Open** → choisis `points.gpx`
3. Tes waypoints apparaissent sur la carte avec leurs tags (`amenity=bench` etc.)
4. Pour chaque waypoint :
   - Clique dessus
   - Vérifie que tu es bien à la bonne position (zoom à fond, corrige avec la souris si besoin)
   - Ouvre l'éditeur de tags (touche `P` ou **Edit → Preferences → Tag**)
   - Vérifie / enrichis les tags : ajoute par exemple `check_date=2026-04-19`, `source=survey`
5. Quand tout est bon, **File → Upload** → authentifie-toi avec ton compte OSM → description de ton changeset (ex. "Ajout bancs et poubelles quartier du port")

Félicitations, ta donnée est sur OSM en quelques minutes ! ⭐

---

## 🎨 Alternative plus simple : MapComplete / iD

Si JOSM te fait peur, utilise **[iD](https://www.openstreetmap.org/edit)** (éditeur web officiel d'OSM, plus simple).

Workflow :
1. Sur openstreetmap.org, connecte-toi
2. Zoom sur la zone de ta balade
3. Clique **Edit**
4. Ouvre `points.geojson` dans [geojson.io](https://geojson.io) en parallèle pour voir où sont tes points
5. Dans iD, ajoute manuellement chaque POI aux coordonnées correspondantes, avec les tags

C'est plus manuel mais ne demande aucun setup.

---

## 💡 Bonnes pratiques OSM

- **Ne duplique pas** : avant de logger, regarde si le POI existe déjà sur OSM (utilise https://www.openstreetmap.org/#map=19/YOUR_LAT/YOUR_LON)
- **Sois précis** : moins de 3 m d'écart entre ta position et le POI réel
- **Source=survey** : ajoute ce tag quand tu importes, ça valide que tu as vérifié sur place (valeur très prisée par la communauté OSM)
- **Photo=yes / photo=non** : certains contributeurs mappent aussi quand ils ne sont pas sûrs. Indique si ton tag vient d'une photo vérifiée ou d'une supposition
- **Respecte le terrain** : ne log pas l'intérieur de bâtiments privés, les propriétés de gens qui n'ont rien demandé, etc.
- **Commence petit** : 5 points bien placés valent mieux que 50 points approximatifs

---

## 🆘 Ça ne marche pas

- **GPS ne capte rien** : check les fils TX/RX (GPS TX → pin **14** Flipper), vérifie avec `GPS status` que `NMEA: X B / Y lines` augmente
- **Fix jamais acquis** : va vraiment dehors, attends 5 min, verifie que `sats` monte
- **Coordonnées bizarres (0,0 ou loin)** : fix pas encore établi, attends
- **App crashe** : reboot (Back long), reprends. Si ça persiste, ouvre une issue sur https://github.com/simongrossi/flipperzero-osm-logger-gps/issues

Pour plus de debug, la CLI Flipper :

```bash
ufbt cli
> log debug
```

Puis reproduire le bug — les erreurs internes s'affichent.

---

## 🔗 Pour aller plus loin

- [OpenStreetMap Wiki](https://wiki.openstreetmap.org/) — l'encyclopédie OSM, trouve les bons tags
- [taginfo.openstreetmap.org](https://taginfo.openstreetmap.org/) — quels tags sont réellement utilisés
- [OSM MapComplete](https://mapcomplete.org) — interface web pour ajouter des POI simples
- [Learn OSM](https://learnosm.org) — tutoriels officiels pour débutants

**Happy mapping! 🗺️**
