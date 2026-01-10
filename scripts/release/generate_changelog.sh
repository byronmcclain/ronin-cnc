#!/bin/bash
# generate_changelog.sh - Generate Changelog from Git
# Task 18j - Release Verification

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

VERSION="${1:-1.0.0}"
SINCE="${2:-}"  # Optional: tag or commit to start from

cd "$ROOT_DIR"

echo "# Changelog"
echo ""
echo "## Version $VERSION"
echo ""
echo "Release Date: $(date +%Y-%m-%d)"
echo ""

# Get commits since last tag (or all if no tag)
if [ -n "$SINCE" ]; then
    COMMITS=$(git log --oneline "$SINCE"..HEAD 2>/dev/null)
else
    LAST_TAG=$(git describe --tags --abbrev=0 2>/dev/null || echo "")
    if [ -n "$LAST_TAG" ]; then
        COMMITS=$(git log --oneline "$LAST_TAG"..HEAD 2>/dev/null)
    else
        COMMITS=$(git log --oneline 2>/dev/null | head -50)
    fi
fi

# Categorize commits
echo "### Features"
FEATURES=$(echo "$COMMITS" | grep -iE "feat|add|new|implement" | sed 's/^/- /')
if [ -n "$FEATURES" ]; then
    echo "$FEATURES"
else
    echo "- None"
fi
echo ""

echo "### Bug Fixes"
FIXES=$(echo "$COMMITS" | grep -iE "fix|bug|repair|correct" | sed 's/^/- /')
if [ -n "$FIXES" ]; then
    echo "$FIXES"
else
    echo "- None"
fi
echo ""

echo "### Improvements"
IMPROVEMENTS=$(echo "$COMMITS" | grep -iE "improve|enhance|optimize|update|refactor" | sed 's/^/- /')
if [ -n "$IMPROVEMENTS" ]; then
    echo "$IMPROVEMENTS"
else
    echo "- None"
fi
echo ""

echo "### Documentation"
DOCS=$(echo "$COMMITS" | grep -iE "doc|readme|comment" | sed 's/^/- /')
if [ -n "$DOCS" ]; then
    echo "$DOCS"
else
    echo "- None"
fi
echo ""

# Known issues
if [ -f "$ROOT_DIR/docs/KNOWN_ISSUES.md" ]; then
    echo "### Known Issues"
    echo ""
    echo "See [KNOWN_ISSUES.md](KNOWN_ISSUES.md) for current known issues."
fi
