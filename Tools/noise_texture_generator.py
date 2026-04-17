"""
Perlin Noise Texture Generator for Unreal Engine Smoke Material
Outputs a seamlessly tileable grayscale PNG.
"""

import numpy as np
from PIL import Image

# ── Config ────────────────────────────────────────────────────────────────────
WIDTH       = 512       # Must be power of 2 for UE tiling
HEIGHT      = 512
OCTAVES     = 6         # More octaves = more detail
PERSISTENCE = 0.5       # Amplitude falloff per octave (0–1)
LACUNARITY  = 2.0       # Frequency multiplier per octave
SCALE       = 4.0       # Base frequency (higher = more zoomed-out noise)
OUTPUT_PATH = "smoke_noise.png"
# ─────────────────────────────────────────────────────────────────────────────


def make_permutation_table(seed: int = 42) -> np.ndarray:
    rng = np.random.default_rng(seed)
    p = np.arange(256, dtype=np.int32)
    rng.shuffle(p)
    return np.tile(p, 2)          # doubled to avoid index wrapping


def fade(t: np.ndarray) -> np.ndarray:
    """Smoothstep: 6t^5 - 15t^4 + 10t^3"""
    return t * t * t * (t * (t * 6 - 15) + 10)


def lerp(a: np.ndarray, b: np.ndarray, t: np.ndarray) -> np.ndarray:
    return a + t * (b - a)


def gradient(h: np.ndarray, x: np.ndarray, y: np.ndarray) -> np.ndarray:
    """Map hash value to one of 8 gradient directions."""
    h = h & 7
    u = np.where(h < 4, x, y)
    v = np.where(h < 4, y, x)
    return np.where(h & 1, -u, u) + np.where(h & 2, -v, v)


def perlin(x: np.ndarray, y: np.ndarray, perm: np.ndarray) -> np.ndarray:
    xi = np.floor(x).astype(np.int32) & 255
    yi = np.floor(y).astype(np.int32) & 255
    xf = x - np.floor(x)
    yf = y - np.floor(y)

    u = fade(xf)
    v = fade(yf)

    aa = perm[perm[xi    ] + yi    ]
    ab = perm[perm[xi    ] + yi + 1]
    ba = perm[perm[xi + 1] + yi    ]
    bb = perm[perm[xi + 1] + yi + 1]

    x1 = lerp(gradient(aa, xf,     yf    ),
              gradient(ba, xf - 1, yf    ), u)
    x2 = lerp(gradient(ab, xf,     yf - 1),
              gradient(bb, xf - 1, yf - 1), u)

    return lerp(x1, x2, v)


def generate_fbm(width: int, height: int,
                 octaves: int, persistence: float,
                 lacunarity: float, scale: float,
                 seed: int = 42) -> np.ndarray:
    """
    Fractional Brownian Motion — stacks multiple Perlin octaves.
    Uses modular coordinates so the result tiles seamlessly.
    """
    perm = make_permutation_table(seed)

    # Coordinate grids (normalized 0–1, then scaled)
    xs = np.linspace(0, scale, width,  endpoint=False)
    ys = np.linspace(0, scale, height, endpoint=False)
    x_grid, y_grid = np.meshgrid(xs, ys)

    noise    = np.zeros((height, width), dtype=np.float64)
    amp      = 1.0
    freq     = 1.0
    max_val  = 0.0

    for _ in range(octaves):
        noise   += perlin(x_grid * freq, y_grid * freq, perm) * amp
        max_val += amp
        amp     *= persistence
        freq    *= lacunarity

    # Normalize to [0, 1]
    noise = (noise / max_val + 1.0) * 0.5
    noise = np.clip(noise, 0, 1)
    return noise


def main():
    print(f"Generating {WIDTH}x{HEIGHT} smoke noise  ({OCTAVES} octaves)…")
    noise = generate_fbm(WIDTH, HEIGHT, OCTAVES, PERSISTENCE, LACUNARITY, SCALE)

    img_array = (noise * 255).astype(np.uint8)
    img = Image.fromarray(img_array, mode="L")
    img.save(OUTPUT_PATH)
    print(f"Saved → {OUTPUT_PATH}")
    print("Import into UE as a Texture2D, set Compression = Grayscale,")
    print("and plug into Opacity/Density in your smoke material.")


if __name__ == "__main__":
    main()
