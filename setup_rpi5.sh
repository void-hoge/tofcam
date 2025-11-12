#!/usr/bin/env bash
set -euo pipefail

DEVNODE=/dev/video0

get_media_device() {
    v4l2-ctl --list-devices |
    awk -v target="$1" '
        /:$/ { inblock=0 } 
        /\/dev\/video/ { if ($1 == target) inblock=1 }
        inblock && /\/dev\/media/ { print $1; exit }'
}

get_arducam_name() {
    local media="$1"

    media-ctl -d "$media" -p | awk -F': ' '
        BEGIN {
            IGNORECASE = 1
        }
        /- entity [0-9]+: .*arducam-pivariety/ {
            sub(/\s* \(.*/, "", $2)
            print $2
            found = 1
            exit
        }

        END {
            if (!found)
                exit 1
        }
    '
}

media=$(get_media_device $DEVNODE)

camera=$(get_arducam_name $media)

media-ctl -d "$media" -r
media-ctl -d "$media" -V "'$camera':0 [fmt:Y12_1X12/240x180 field:none]"
media-ctl -d "$media" -V "'csi2':0 [fmt:Y12_1X12/240x180 field:none]"
media-ctl -d "$media" -V "'csi2':4 [fmt:Y12_1X12/240x180 field:none]"
media-ctl -d "$media" -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0 [1]"
media-ctl -d "$media" -l "'csi2':4 -> 'pisp-fe':0 [0]"
