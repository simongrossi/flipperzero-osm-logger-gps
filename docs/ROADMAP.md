# Roadmap / features pas encore implémentées

Ce fichier liste tout ce qui reste à faire dans l'app, groupé par thème et priorité.

**Pour un état "v1.0 catalogue-ready polished"**, la suite recommandée est :
1. Tests unitaires parser NMEA (invoqué dans la CI)
2. Détection de doublons plus fine (seuil dynamique selon HDOP ?)
3. OSM Notes API via WiFi dev board (le gros morceau ambitieux, v1.0+)

---

## 🎯 Haute priorité — UX terrain

---

## 🧩 Qualité des données

### Altitude calibration point-zéro
Option "Calibrate altitude here" dans Settings : fige l'altitude actuelle comme offset négatif à ajouter aux prochaines mesures. Corrige les dérives GPS typiques (± 20 m).

---

## 🛠️ Robustesse & infrastructure

### GitHub Actions CI
Workflow `.github/workflows/build.yml` qui build avec ufbt à chaque push sur main et chaque PR. Valide la compat Release + RC firmware à chaque commit. Signal de sérieux pour les reviewers et contributeurs.

### Tests unitaires du parser NMEA
Des samples NMEA réels en fichier texte → attendu parsé (lat, lon, sats, etc.). Run dans la CI. Empêche les régressions sur le code le plus critique.

### Crash recovery log
Si `app_show_fatal` est appelé, dumper la backtrace dans `/ext/apps_data/osm_logger/crash.log`. Debugging grandement facilité pour les retours utilisateurs.

### Rotation des fichiers
Après N points (ex. 1000), ouvrir un fichier nouveau (`points-2.jsonl`, `track-2.gpx`) pour éviter les fichiers géants et faciliter les imports partiels.

### Mode "dry run"
Toggle dans Settings : active → l'app simule les saves (son + compteur) mais n'écrit rien sur SD. Utile pour démos et tests.

### Messages de debug conditionnels
Plusieurs `FURI_LOG_I` spament en usage normal. Passer à `FURI_LOG_D` (debug) pour être silencieux par défaut.

### Feedback haptique différencié
Actuellement même son pour save / fix acquis / entrée mode trace. Définir des `NotificationSequence` custom pour distinguer.

---

## 🎮 Navigation

### Shortcut pour relancer le mode trace
Raccourci (ex. hold Back depuis le menu) pour ré-entrer directement dans Track si l'utilisateur fait souvent start/stop.

### Retour à la dernière vue utilisée
Au prochain démarrage de l'app, ouvrir directement la dernière vue utilisée (Quick Log avec le dernier preset) au lieu du menu principal. Stocker dans `settings.txt`.

### Sous-catégories dans le menu des presets
Le submenu affiche les 23 presets en une seule liste plate. Les regrouper par catégorie (mobilier, voirie, commerces…) permettrait de naviguer plus vite avec beaucoup de presets. Submenu à 2 niveaux ou séparateurs visuels.

---

## 📝 Éditeur de note

### Prefixe ou annotation automatique (dépassé par auto_photo_id)
`auto_photo_id` couvre déjà le cas "je veux lier mes saves à mes photos téléphone". Autres automatisations possibles : auto-prefix d'un identifiant de session, tag de source (`source=survey`), etc.

---

## 🔌 Compatibilité matériel

### Support d'autres modules GPS
Testé uniquement avec **NEO-6M V2**. Devrait fonctionner avec tout module NMEA (PA1010D, BN-180, NEO-M8N, …) mais non vérifié. Le baud rate est désormais configurable via Settings.

### Support GLONASS / Galileo / BeiDou
Le parser reconnaît les préfixes `$GP*` et `$GN*` mais pas `$GL*` (GLONASS seul) ou `$GA*` (Galileo). Pas critique en pratique (les modules multi-GNSS émettent du `$GN*`), mais à documenter.

### Intégration IR / Sub-GHz pour contextualiser ?
Idée plus expérimentale : logger avec le point les signaux IR / Sub-GHz captés (ex. balise bluetooth à proximité). Overkill pour la plupart des cas mais pourrait intéresser des mappeurs indoor.

---

## 🌐 Intégrations distantes

### Export vers OSM Notes via l'API
Avec le dev board WiFi Flipper : poster directement les points en Notes OSM via leur API REST. Ferme la boucle de A à Z depuis le device. Gros chantier (auth, format OSM API, gestion réseau sur dev board).

### Export en ZIP depuis l'app
Générer une archive `osm_logger_YYYYMMDD.zip` avec les 5 fichiers de sortie + un README, prêt à transférer via qFlipper. Entry dans le menu principal "Export session".

### Import de tracks existants
Ouvrir un `track.gpx` existant pour continuer une session au lieu de redémarrer de zéro.

---

## 📚 Documentation / communauté

### Vidéo démo de 30 s
Screen recording via qFlipper → convert GIF → embed dans README. 10× plus attractif que des screenshots statiques.

### Comparatif avec les autres apps GPS
Section dans le README : "pourquoi OSM Logger vs gps_nmea / autres". Positionnement clair par cas d'usage.

### Tutoriel "Premier contributeur OSM avec Flipper"
`docs/GETTING_STARTED_OSM.md` expliquant le workflow complet : câbler → lancer → marcher → transférer → importer dans JOSM. Cible les nouveaux mappeurs OSM.

### GitHub issue templates
Bug report / Feature request / Preset request, pour structurer les retours.

### Contributing guide
`CONTRIBUTING.md` détaillé pour ajouter un preset, un module GPS, un format de sortie.

---

## 🧪 Tests à faire (hors code)

### Validation terrain
- ✅ Fix réel en extérieur : vérifier lat/lon cohérents avec un GPS de référence
- ✅ HDOP descend sous 2.5 en ciel dégagé
- ✅ Altitude cohérente (±10 m de l'altitude réelle connue)
- ✅ OK court en bon fix, OK long en canyon urbain
- ✅ Fix acquisition : démarrer indoor → sortir → vérifier la notification
- ✅ Session longue (1h+) en mode trace → pas de fuite mémoire, fichier track.gpx reste valide
- ✅ Tester tous les baud rates avec différents modules

### Import des formats de sortie
- ✅ `points.gpx` dans **JOSM** : waypoints avec le bon nom OSM
- ✅ `points.geojson` dans **iD** / **QGIS**
- ✅ `track.gpx` dans **GPX Viewer** / **Gaia GPS** / Strava
- ✅ `notes.csv` dans Excel / LibreOffice (encodage UTF-8)
- ✅ Copier-coller dans **geojson.io** pour validation rapide

### Edge cases
- ✅ App démarrée sans microSD → pas de crash, sauvegardes échouent silencieusement
- ✅ `presets.txt` corrompu / malformé → fallback propre sur les defaults
- ✅ `settings.txt` corrompu → fallback defaults
- ✅ Coupure d'alim pendant un save → fichiers GPX/GeoJSON toujours valides au prochain démarrage
- ✅ Changement de baud rate en live avec des trames en cours → pas de crash
- ✅ Saves en mode trace à 1 s pendant 10 min → fichier reste cohérent

---

> Les features finies sont trackées dans le [README](../README.md) section ✨ Features.
> Pour contribuer, ouvrir une issue ou PR sur https://github.com/simongrossi/flipperzero-osm-logger-gps
