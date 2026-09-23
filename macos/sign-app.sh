#!/usr/bin/env bash
set -euo pipefail

IDENTITY=""
APPLE_ID=""
TEAM_ID=""
PASSWORD=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --identity) IDENTITY="$2"; shift 2 ;;
    --apple-id) APPLE_ID="$2"; shift 2 ;;
    --team-id)  TEAM_ID="$2";  shift 2 ;;
    --password) PASSWORD="$2"; shift 2 ;;
    *) echo "Unknown argument: $1" >&2; exit 1 ;;
  esac
done

: "${IDENTITY:?--identity is required}"
: "${APPLE_ID:?--apple-id is required}"
: "${TEAM_ID:?--team-id is required}"
: "${PASSWORD:?--password is required}"

# Sign it
codesign -v -f --deep -s "$IDENTITY" -o runtime --timestamp Cheech.app

# Zip it
rm -f Cheech.zip
ditto -c -k --keepParent "Cheech.app" "Cheech.zip"

# Submit and capture output
SUBMIT_OUT=$(xcrun notarytool submit --apple-id "$APPLE_ID" --team-id "$TEAM_ID" --password "$PASSWORD" --output-format json Cheech.zip)
SUB_ID=$(echo "$SUBMIT_OUT" | jq -r '.id')

# Wait for processing to complete
xcrun notarytool wait "$SUB_ID" --apple-id "$APPLE_ID" --team-id "$TEAM_ID" --password "$PASSWORD"

# Staple ticket
xcrun stapler staple "Cheech.app"
rm -f Cheech.zip
