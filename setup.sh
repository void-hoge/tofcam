#!/usr/bin/bash

media-ctl -d /dev/media0 -r
media-ctl -d /dev/media0 -V "'arducam-pivariety 10-000c':0 [fmt:Y12_1X12/240x180 field:none]"
media-ctl -d /dev/media0 -V "'csi2':0 [fmt:Y12_1X12/240x180 field:none]"
media-ctl -d /dev/media0 -V "'csi2':4 [fmt:Y12_1X12/240x180 field:none]"
media-ctl -d /dev/media0 -l "'csi2':4 -> 'rp1-cfe-csi2_ch0':0 [1]"
media-ctl -d /dev/media0 -l "'csi2':4 -> 'pisp-fe':0 [0]"
