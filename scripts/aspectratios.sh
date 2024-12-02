#!/bin/bash

# define common photo and film aspect ratios
aspect_ratios=("1.33" "1.5" "1.77" "1.85" "2.35" "2.39")

# loop through the aspect ratios and run symmetrytool for each
for ar in "${aspect_ratios[@]}"; do
    # calculate the width based on the aspect ratio and make it a multiple of 1000
    width=$(echo "scale=0; $ar * 1000 / 1" | bc)
    height=1000    
    output_file="./symmetrytool_${ar}.png"
    ./symmetrytool --aspectratio "$ar" --outputfile "$output_file" --symmetrygrid --centerpoint --size "${width},${height}" -v -d --scale 1
    echo "Created $output_file"
done
