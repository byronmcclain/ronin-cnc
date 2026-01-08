#!/bin/bash
# Verification script for documentation

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "=== Checking Documentation ==="

cd "$ROOT_DIR"

# Step 1: Check main README exists and has content
echo "Step 1: Checking README.md..."
if [ ! -f "README.md" ]; then
    echo "VERIFY_FAILED: README.md not found"
    exit 1
fi
if [ $(wc -l < README.md) -lt 50 ]; then
    echo "VERIFY_FAILED: README.md too short"
    exit 1
fi
echo "  README.md exists and has content"

# Step 2: Check docs directory
echo "Step 2: Checking docs directory..."
mkdir -p docs
for doc in BUILD.md ARCHITECTURE.md PLATFORM-API.md; do
    if [ ! -f "docs/$doc" ]; then
        echo "VERIFY_FAILED: docs/$doc not found"
        exit 1
    fi
done
echo "  All documentation files exist"

# Step 3: Check CHANGELOG
echo "Step 3: Checking CHANGELOG.md..."
if [ ! -f "CHANGELOG.md" ]; then
    echo "VERIFY_FAILED: CHANGELOG.md not found"
    exit 1
fi
echo "  CHANGELOG.md exists"

# Step 4: Verify README has key sections
echo "Step 4: Verifying README content..."
for section in "Quick Start" "Building" "Requirements" "License"; do
    if ! grep -qi "$section" README.md; then
        echo "VERIFY_FAILED: README missing '$section' section"
        exit 1
    fi
    echo "  Found '$section' section"
done

# Step 5: Check no broken internal links (basic)
echo "Step 5: Checking documentation links..."
for doc in README.md docs/BUILD.md; do
    if grep -o '\[.*\](docs/[^)]*\.md)' "$doc" 2>/dev/null | while read link; do
        target=$(echo "$link" | grep -o 'docs/[^)]*\.md')
        if [ ! -f "$target" ]; then
            echo "  WARNING: Broken link to $target in $doc"
        fi
    done; then
        true  # Continue even with warnings
    fi
done
echo "  Link check complete"

echo ""
echo "VERIFY_SUCCESS"
exit 0
