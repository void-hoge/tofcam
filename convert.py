#!/usr/bin/env python3
import argparse
import sys
from pathlib import Path

import numpy as np
import cv2


WIDTH = 240
HEIGHT = 180
CONF_THRESHOLD = 30.0  # Amplitude used as confidence threshold


def load_float_image(path, width=WIDTH, height=HEIGHT):
    data = np.fromfile(path, dtype=np.float32)
    expected = width * height
    if data.size != expected:
        raise ValueError(
            f"{path}: invalid element count (expected={expected}, actual={data.size})"
        )
    return data.reshape((height, width))


def depth_to_color(depth, amplitude, dmin, dmax):
    if dmax <= dmin:
        raise ValueError("min must be smaller than max")

    # Normalize depth to 0-255
    depth_clip = np.clip(depth, dmin, dmax)
    norm = (depth_clip - dmin) / (dmax - dmin)
    depth_8u = (norm * 255.0).astype(np.uint8)

    # Apply color map
    color = cv2.applyColorMap(depth_8u, cv2.COLORMAP_RAINBOW)

    # Mask conditions
    mask = (depth < dmin) | (depth > dmax) | np.isnan(depth)
    mask |= (amplitude < CONF_THRESHOLD)

    color[mask] = (0, 0, 0)
    return color


def amplitude_to_gray(amplitude):
    flat = amplitude.reshape(-1).astype(np.float32)

    # Use 2nd and 98th percentiles for auto gain
    p2, p98 = np.percentile(flat, [2.0, 98.0])

    if p98 <= p2:
        p2 = float(flat.min())
        p98 = float(flat.max())

    if p98 <= p2:
        return np.zeros_like(amplitude, dtype=np.uint8)

    gain = 255.0 / (p98 - p2)
    offset = -p2 * gain

    amp_8u = amplitude * gain + offset
    amp_8u = np.clip(amp_8u, 0, 255).astype(np.uint8)

    return amp_8u


def parse_args():
    parser = argparse.ArgumentParser(
        description="Convert ToF depth/amplitude raw floats to PNG images"
    )
    parser.add_argument("amplitude_file")
    parser.add_argument("depth_file")
    parser.add_argument("--min", dest="min_range", type=float, required=True)
    parser.add_argument("--max", dest="max_range", type=float, required=True)
    return parser.parse_args()


def main():
    args = parse_args()

    try:
        depth = load_float_image(args.depth_file)
        amplitude = load_float_image(args.amplitude_file)
    except Exception as e:
        print(e, file=sys.stderr)
        sys.exit(1)

    depth_color = depth_to_color(
        depth,
        amplitude,
        args.min_range,
        args.max_range,
    )

    amp_gray = amplitude_to_gray(amplitude)

    cv2.imwrite(str(base.with_name(f"{args.depth_file}.png")), depth_color)
    cv2.imwrite(str(base.with_name(f"{args.amplitude_file}.png")), amp_gray)


if __name__ == "__main__":
    main()
