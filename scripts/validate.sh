#!/usr/bin/env bash
# SPDX-License-Identifier: AGPL-3.0-only
# Copyright (C) 2026 Marco Franceschini
#
# Validates the installed Lion Voice plugin:
#   - VST3 with pluginval (https://github.com/Tracktion/pluginval)
#   - AU with Apple's auval
#
# Usage: scripts/validate.sh [--strict] [--no-gui]
#   --strict   pluginval strictness 10 instead of 5
#   --no-gui   skip pluginval's editor (window) tests
#
# Environment:
#   PLUGINVAL  path to the pluginval binary (default: /Applications/pluginval.app/Contents/MacOS/pluginval)

set -euo pipefail

strictness=5
gui_flag=()

for arg in "$@"; do
    case "$arg" in
        --strict) strictness=10 ;;
        --no-gui) gui_flag=(--skip-gui-tests) ;;
        -h|--help) sed -n '4,14p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "Unknown option: $arg" >&2; exit 2 ;;
    esac
done

pluginval="${PLUGINVAL:-/Applications/pluginval.app/Contents/MacOS/pluginval}"
vst3="$HOME/Library/Audio/Plug-Ins/VST3/Lion Voice.vst3"
au_type="aufx"
au_subtype="Lnvc"
au_manufacturer="MrFr"

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
log_dir="$repo_root/build/validation"
mkdir -p "$log_dir"

failures=0

# --- VST3 -------------------------------------------------------------------
echo "==> pluginval (VST3, strictness $strictness)"
if [[ ! -x "$pluginval" ]]; then
    echo "    pluginval not found at $pluginval (set PLUGINVAL to override)" >&2
    failures=$((failures + 1))
elif [[ ! -d "$vst3" ]]; then
    echo "    plugin not installed at $vst3 - build first" >&2
    failures=$((failures + 1))
else
    vst3_log="$log_dir/pluginval.log"
    if "$pluginval" --strictness-level "$strictness" ${gui_flag[@]+"${gui_flag[@]}"} \
        --validate "$vst3" > "$vst3_log" 2>&1; then
        echo "    PASS"
    else
        echo "    FAIL - last lines of $vst3_log:" >&2
        tail -n 25 "$vst3_log" >&2
        failures=$((failures + 1))
    fi
fi

# --- AU ---------------------------------------------------------------------
echo "==> auval (AU $au_type $au_subtype $au_manufacturer)"
au_log="$log_dir/auval.log"
if auval -v "$au_type" "$au_subtype" "$au_manufacturer" > "$au_log" 2>&1 \
    && grep -q "AU VALIDATION SUCCEEDED" "$au_log"; then
    echo "    PASS"
else
    echo "    FAIL - last lines of $au_log:" >&2
    tail -n 25 "$au_log" >&2
    echo "    Tip: if a fresh build is not picked up, run: killall -9 AudioComponentRegistrar" >&2
    failures=$((failures + 1))
fi

# --- Summary ----------------------------------------------------------------
if [[ $failures -eq 0 ]]; then
    echo "All validations passed. Logs: $log_dir"
else
    echo "$failures validation(s) failed. Logs: $log_dir" >&2
    exit 1
fi
