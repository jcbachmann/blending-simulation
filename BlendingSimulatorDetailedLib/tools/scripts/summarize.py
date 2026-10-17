"""Summarizes DemCalibration output (one JSON object per line): time, volumes, F1 of the linear input and F2 (standard deviation of
the 1 m slice volumes from the ideal stockpile of the bmh_ml bed, as bmh_ml computes it). Usage: python summarize.py file [file ...]"""

import json
import math
import sys


def ideal_volumes(n, total, x_min=10.0, x_max=49.0):
    """Slice volumes of the ideal stockpile (45° ridge with half cones at the ends), reclaimed at 45° in 1 m slices, by numerical
    integration of the cross section along the reclaim position."""
    core = x_max - x_min
    # height h of an ideal pile: core * h^2 + pi h^3 / 3 = total
    lo, hi = 0.0, 50.0
    for _ in range(100):
        h = 0.5 * (lo + hi)
        (lo, hi) = (h, hi) if core * h * h + math.pi * h**3 / 3 < total else (lo, h)
    h = 0.5 * (lo + hi)

    def height(x, z):
        if x < x_min:
            return max(0.0, h - math.hypot(x_min - x, z))
        if x > x_max:
            return max(0.0, h - math.hypot(x - x_max, z))
        return max(0.0, h - abs(z))

    volumes = [0.0] * n
    step = 0.05
    xs = [x_min - h + (i + 0.5) * step for i in range(int((core + 2 * h) / step))]
    zs = [-h + (k + 0.5) * step for k in range(int(2 * h / step))]
    for x in xs:
        for z in zs:
            top = height(x, z)
            if top <= 0:
                continue
            # the column from 0 to top is reclaimed at positions x - y for y in [0, top]
            ys = 20
            for j in range(ys):
                y = (j + 0.5) * top / ys
                s = x - y
                index = min(max(int(math.floor(s)) + 1, 1), n - 1)
                volumes[index] += step * step * top / ys
    return volumes


def main():
    for path in sys.argv[1:]:
        for line in open(path):
            line = line.strip()
            if not line.startswith("{"):
                continue
            r = json.loads(line.replace(": nan", ": null"))
            v = r["volumes"]
            f2 = None
            if r["mode"] == "chevron":
                try:  # bmh's ideal stockpile when available, else the numerical one above
                    import numpy as np
                    from bmh.helpers.stockpile_math import get_ideal_stockpile_volumes

                    ideal = list(get_ideal_stockpile_volumes(np.arange(len(v), dtype=float), sum(v), 10.0, 49.0))
                except ImportError:
                    ideal = ideal_volumes(len(v), sum(v))
                d = [a - b for a, b in zip(ideal, v)]
                mean = sum(d) / len(d)
                f2 = math.sqrt(sum((x - mean) ** 2 for x in d) / len(d))
            print(f"{r['mode']:8} ppm3 {r['ppm3']:5g} {r.get('args', ''):28} {r['seconds']:8.1f} s  sim {r['simulatedSeconds']:7.1f} s  "
                  f"blocked {r.get('blockedSeconds', float('nan')):6.1f} s  particles {r['particles']:6d} maxDyn {r['maxDynamic']:5d}  bulk {r['bulkVolume']:7.1f}  packing {r['packing']:.3f}  "
                  f"angle {r['angle'] if r['angle'] is not None else float('nan'):5.1f}  F1 {r['F1']:.4f}  F2 {f2 if f2 is not None else float('nan'):6.2f}  settled {r['settled']}")


if __name__ == "__main__":
    main()
