#!/bin/bash
# generate_icons.sh - Generate App Icon Set
# Task 18i - macOS App Bundle

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
ICONSET_DIR="${ROOT_DIR}/resources/macos/icon.iconset"
SOURCE_ICON="${1:-${ROOT_DIR}/resources/icon_1024.png}"

echo "=== Generating Icon Set ==="

if [ ! -f "$SOURCE_ICON" ]; then
    echo "ERROR: Source icon not found: ${SOURCE_ICON}"
    echo "Provide a 1024x1024 PNG as the source"
    exit 1
fi

# Create iconset directory
mkdir -p "$ICONSET_DIR"

# Generate all required sizes
# Format: icon_<size>x<size>[@2x].png
sizes=(16 32 128 256 512)

for size in "${sizes[@]}"; do
    # Standard resolution
    echo "Generating ${size}x${size}..."
    sips -z $size $size "$SOURCE_ICON" --out "${ICONSET_DIR}/icon_${size}x${size}.png"

    # Retina (@2x)
    double=$((size * 2))
    if [ $double -le 1024 ]; then
        echo "Generating ${size}x${size}@2x (${double}x${double})..."
        sips -z $double $double "$SOURCE_ICON" --out "${ICONSET_DIR}/icon_${size}x${size}@2x.png"
    fi
done

# Generate .icns
echo "Generating icon.icns..."
iconutil -c icns -o "${ROOT_DIR}/resources/macos/icon.icns" "$ICONSET_DIR"

echo ""
echo "=== Icon generation complete ==="
echo "Iconset: ${ICONSET_DIR}"
echo "ICNS: ${ROOT_DIR}/resources/macos/icon.icns"
