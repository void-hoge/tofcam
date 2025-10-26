## Benchmarks
- Values shown in parentheses indicate results when only depth is computed, without computing confidence.
- The frame rate indicates the number of raw frames processed per second, Four raw frames are combined to produce one depth image.

### Raspberry Pi5 (Cortex A76 @ 2.4GHz)

- CPU frequency fixed at 2.4GHz.

|              | Arducam ToF SDK | tofcam scalar (depth) | tofcam neon (depth) |
|--------------|-----------------|-----------------------|---------------------|
| CPU Load     | 18.4%           | 1.21% (1.14%)         | 0.68% (0.57%)       |
| Max Cvt Rate | (not supported) | 11500fps (12300fps)   | 22900fps (27800fps) | 

### Raspberry Pi Zero 2 W (Cortex A53 @ 1.0GHz)

- CPU frequency fixed at 1.0GHz.

|              | Arducam ToF SDK | tofcam scalar (depth) | tofcam neon (depth) |
|--------------|-----------------|-----------------------|---------------------|
| CPU Load     | 46.1%           | 6.9% (6.1%)           | 6.0% (4.6%)         |
| Max Cvt Rate | (not supported) | 2300fps (2500fps)     | 3000fps (4500fps)   |
