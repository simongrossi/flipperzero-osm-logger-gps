#include "presets.h"

const Preset PRESETS[] = {
    // Mobilier urbain
    {"Banc", "amenity", "bench"},
    {"Poubelle", "amenity", "waste_basket"},
    {"Fontaine", "amenity", "drinking_water"},
    {"Toilettes", "amenity", "toilets"},
    {"Lampadaire", "highway", "street_lamp"},
    {"Boite aux lettres", "amenity", "post_box"},
    {"Panneau info", "tourism", "information"},
    {"Table picnic", "leisure", "picnic_table"},
    {"Arbre", "natural", "tree"},

    // Voirie
    {"Passage pieton", "highway", "crossing"},
    {"Arret de bus", "highway", "bus_stop"},

    // Stationnement
    {"Parking velo", "amenity", "bicycle_parking"},
    {"Parking auto", "amenity", "parking"},
    {"Borne de charge", "amenity", "charging_station"},

    // Sports et loisirs
    {"Aire de jeux", "leisure", "playground"},
    {"Panier de basket", "amenity", "basketball_hoop"},

    // Dechets
    {"Recyclage", "amenity", "recycling"},

    // Commerces et services
    {"Pharmacie", "amenity", "pharmacy"},
    {"Cafe", "amenity", "cafe"},
    {"Restaurant", "amenity", "restaurant"},
    {"Boulangerie", "shop", "bakery"},

    // Urgence
    {"Defibrillateur", "emergency", "defibrillator"},
    {"Bouche incendie", "emergency", "fire_hydrant"},
};

const uint8_t PRESETS_COUNT = (uint8_t)(sizeof(PRESETS) / sizeof(PRESETS[0]));
