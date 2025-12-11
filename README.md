## Benchmarks
- Values shown in parentheses indicate results when only depth is computed, without computing confidence.
- The frame rate indicates the number of raw frames processed per second, Four raw frames are combined to produce one depth image.

### Raspberry Pi5 (Cortex A76 @ 2.4GHz)

- CPU frequency fixed at 2.4GHz.

|              | Arducam ToF SDK | tofcam std::atan2 | tofcam approx_atan  | tofcam neon         |
|--------------|-----------------|-------------------|---------------------|---------------------|
| CPU Load     | 18.4%           | 4.7% (4.3%)       | 1.20% (1.11%)       | 0.69% (0.57%)       |
| Max Cvt Rate | (not supported) | 2600fps (2800fps) | 11500fps (12300fps) | 22900fps (28300fps) |

### Raspberry Pi Zero 2 W (Cortex A53 @ 1.0GHz)

- CPU frequency fixed at 1.0GHz.

|              | Arducam ToF SDK | tofcam approx     | tofcam neon       |
|--------------|-----------------|-------------------|-------------------|
| CPU Load     | 46.1%           | 6.9% (6.1%)       | 6.0% (4.6%)       |
| Max Cvt Rate | (not supported) | 2300fps (2500fps) | 3000fps (4500fps) |
