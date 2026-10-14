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

# gdk-pixbuf loaders are plain Mach-O modules under Resources; --deep skips
# them, leaving Homebrew's ad-hoc signatures, which notarization rejects
find Cheech.app/Contents/Resources/lib/gdk-pixbuf-2.0/loaders \
  \( -name '*.so' -o -name '*.dylib' \) \
  -exec codesign -f -s "$IDENTITY" -o runtime --timestamp {} +

# Sign it
codesign -v -f --deep -s "$IDENTITY" -o runtime --timestamp Cheech.app

# Zip it
rm -f Cheech.zip
ditto -c -k --keepParent "Cheech.app" "Cheech.zip"

# Submit and capture output
SUBMIT_OUT=$(xcrun notarytool submit --apple-id "$APPLE_ID" --team-id "$TEAM_ID" --password "$PASSWORD" --output-format json Cheech.zip)
SUB_ID=$(echo "$SUBMIT_OUT" | jq -r '.id')
echo "Notarytool submission id: $SUB_ID"

# Wait for processing to complete
xcrun notarytool wait "$SUB_ID" --apple-id "$APPLE_ID" --team-id "$TEAM_ID" --password "$PASSWORD"

# wait exits 0 even when the submission is Invalid; check the real status
STATUS=$(xcrun notarytool log "$SUB_ID" --apple-id "$APPLE_ID" --team-id "$TEAM_ID" --password "$PASSWORD" | jq -r '.status')
if [[ "$STATUS" != "Accepted" ]]; then
  echo "Notarization failed (status: $STATUS); submission log:" >&2
  xcrun notarytool log "$SUB_ID" --apple-id "$APPLE_ID" --team-id "$TEAM_ID" --password "$PASSWORD" | jq .
  exit 1
fi

# Staple ticket (retry once in case of transient CloudKit lookup errors)
if ! xcrun stapler staple -v "Cheech.app"; then
  echo "staple failed; retrying after 30s (CloudKit lookup can be flaky)" >&2
  sleep 30
  xcrun stapler staple -v "Cheech.app"
fi
rm -f Cheech.zip
