#!/bin/bash

# Output file
OUTPUT="prompt_context.txt"

# Clear/create the output file
> "$OUTPUT"

# Files to collect
FILES=(
    "life.c"
    "Makefile"
    "plot.c"
    "real_rand.c"
    "timer.c"
)

for FILE in "${FILES[@]}"; do

    if [ -f "$FILE" ]; then

        echo "============================================================" >> "$OUTPUT"
        echo "FILE: $FILE" >> "$OUTPUT"
        echo "============================================================" >> "$OUTPUT"
        echo "" >> "$OUTPUT"

        cat "$FILE" >> "$OUTPUT"

        echo "" >> "$OUTPUT"
        echo "====================== END OF $FILE ========================" >> "$OUTPUT"
        echo "" >> "$OUTPUT"
        echo "" >> "$OUTPUT"

    else
        echo "WARNING: $FILE not found" >> "$OUTPUT"
    fi

done

echo "All files collected into: $OUTPUT"
