# Roadmap / features pas encore implémentées

Ce fichier liste tout ce qui reste à faire dans l'app, grouped par thème et par priorité subjective.

## 🎨 Polish

### Icône custom dans le launcher
Le fichier [osm_logger.png](../osm_logger.png) existe à la racine mais n'est pas référencé dans `application.fam`. Pour qu'il apparaisse comme icône de l'app dans le menu Apps du Flipper, ajouter :

```python
# application.fam
App(
    ...
    fap_icon="osm_logger.png",
    ...
)
```

Attention : l'image doit être **10×10 pixels, 1-bit PNG** (format Flipper). À convertir si le fichier actuel ne respecte pas ces contraintes.

### Messages de debug conditionnels
Plusieurs `FURI_LOG_I` sont utiles pour comprendre l'état (fix acquis, presets chargés) mais spament en usage normal. Passer à `FURI_LOG_D` (debug) pour être silencieux par défaut et vraiment bavard uniquement avec `log debug` dans la CLI.

### Feedback haptique différencié
Actuellement même son de succès pour :
- Save réussi
- Fix acquis
- Entrée du mode trace

Définir des `NotificationSequence` custom pour distinguer (ex. double-vibration pour fix acquis, une seule pour save).

## ⏱️ Mode trace

### Intervalle configurable
Actuellement codé en dur à 5 secondes (`TRACK_TICK_MS` dans [track.c](../src/track.c)). Ajouter un submenu "Choisir l'intervalle" (1 / 5 / 10 / 30 / 60 s) avant l'entrée dans la vue trace. Stocker dans App.

### Distance minimale entre deux trkpts
En mode trace statique (appareil arrêté), on écrit des trkpts identiques toutes les 5 s. Filtrer : ne pas écrire si distance au dernier point < 3 m (calcul haversine approximatif).

### Seuil HDOP pour trace
Comme pour Quick Log, refuser les trkpts en mode dégradé. Actuellement on écrit dès qu'il y a un fix, peu importe le HDOP.

## 🧰 Presets

### Variantes = tags supplémentaires (pas juste valeurs alternatives)
La V1 des variantes supporte uniquement des **valeurs alternatives** pour le même `key`. Ex. `amenity=bench` → `amenity=outdoor_seating`.

Pour supporter des tags secondaires (`bench` + `material=wood`), il faudrait :
- Étendre le format preset pour autoriser plusieurs paires `key=value` par variante
- Adapter tous les formats de sortie (JSONL, CSV, GPX) pour accepter une chaîne de tags multiples
- Décision : choisir un séparateur (pipe ? point-virgule ?) sans casser les specs existantes

### Catégories dans le sous-menu
Le submenu affiche les 23 presets en une seule liste plate. Les regrouper par catégorie (mobilier, voirie, commerces…) permettrait de naviguer plus vite avec beaucoup de presets. Nécessite soit un submenu à 2 niveaux, soit un séparateur visuel dans le widget `Submenu`.

### Validation au chargement
Actuellement `presets.txt` est accepté tel quel. Ajouter :
- Warning si label > 18 caractères
- Warning si key/value contient des caractères suspects (`=`, espaces non-échappés, accents)
- Affichage du nom du preset invalide dans les logs

## 📝 Éditeur de note

### Note persistante entre sessions
Actuellement la note se reset au redémarrage de l'app. Sauvegarder `quick_note` dans un fichier `note.txt` au save et le recharger à l'init.

### Note par preset (mémorisation)
Aujourd'hui une seule note globale, partagée par tous les presets. Mémoriser la dernière note utilisée **par preset** (ex. "banc en bois" reste attaché au preset Banc).

## 🎮 Navigation

### Shortcut pour relancer le mode trace
Actuellement : menu principal → Mode trace. Si l'utilisateur fait souvent start/stop, ajouter un raccourci (ex. hold Back depuis le menu) pour ré-entrer directement.

### Retour à la dernière vue utilisée
Au prochain démarrage de l'app, ouvrir directement la dernière vue utilisée (Quick Log avec le dernier preset) au lieu du menu principal. Stocker dans un fichier de préférences.

## 🛠️ Fiabilité

### Vérifier l'espace disque SD
Avant chaque save, vérifier que la SD a de la place. Aujourd'hui un save sur SD pleine échoue silencieusement.

### Rotation des fichiers
Après N points (ex. 1000), ouvrir un fichier nouveau (`points-2.jsonl`, `track-2.gpx`) pour éviter les fichiers géants et faciliter les imports partiels.

### Mode "dry run"
Option pour tester l'app sans écrire sur SD (dev/démo). Un toggle dans un menu settings.

## 🔌 Compatibilité matériel

### Support d'autres modules GPS
Testé uniquement avec **NEO-6M V2**. Devrait fonctionner avec tout module NMEA 9600 bauds (PA1010D, BN-180, NEO-M8N, …) mais non vérifié. Pistes :
- Ajouter un menu "Vitesse UART" (4800 / 9600 / 38400 / 115200) pour les modules plus rapides
- Tester avec des modules fournissant plus de trames (GSA, VTG, GSV) et vérifier que le parser les ignore correctement

### Support GLONASS / Galileo / BeiDou
Le parser reconnaît les préfixes `$GP*` et `$GN*` mais pas `$GL*` (GLONASS seul) ou `$GA*` (Galileo). Pas critique car en pratique les modules multi-GNSS émettent du `$GN*`, mais à documenter.

## 🧪 Tests à faire (hors code)

### Validation terrain
- ✅ Fix réel en extérieur : vérifier lat/lon cohérents avec un GPS de référence
- ✅ Vérifier que HDOP descend sous 2.5 en ciel dégagé
- ✅ Vérifier que l'altitude est cohérente (±10 m de l'altitude réelle connue)
- ✅ Tester OK court en bon fix, OK long en canyon urbain
- ✅ Test fix acquisition : démarrer indoor → sortir → vérifier la notification

### Import GPX/GeoJSON
- ✅ `points.gpx` dans **JOSM** : les waypoints doivent apparaître avec le bon nom
- ✅ `points.geojson` dans **iD** ou **QGIS**
- ✅ `track.gpx` dans **GPX Viewer** (Android) ou **Gaia GPS**
- ✅ Copier-coller dans **geojson.io** pour validation rapide

### Edge cases
- ✅ App démarrée sans microSD → pas de crash, sauvegardes échouent silencieusement
- ✅ `presets.txt` corrompu / malformé → fallback propre sur les defaults
- ✅ Coupure d'alim pendant un save → fichiers GPX/GeoJSON toujours valides au prochain démarrage
- ✅ Session très longue (1h+) en mode trace → pas de fuite mémoire, fichier track.gpx reste valide

---

> Les features finies sont trackées dans le [README](../README.md) section ✨ Fonctionnalités.
> Pour contribuer, ouvrir une issue ou PR sur https://github.com/simongrossi/flipperzero-osm-logger-gps
