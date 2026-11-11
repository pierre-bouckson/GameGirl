#!/bin/sh
# Importe un ou plusieurs composants JLCPCB/LCSC (symbole + footprint + 3D)
# dans la librairie projet "EasyEDA". Usage : ./libs/add_lcsc.sh C2040 C25804 ...
cd "$(dirname "$0")/.." || exit 1
for id in "$@"; do
    easyeda2kicad --full --lcsc_id="$id" --output "$PWD/libs/EasyEDA/EasyEDA" --project-relative --overwrite
done
