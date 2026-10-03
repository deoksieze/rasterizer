#!/usr/bin/env python3
"""Generate a randomised stress-test scene for the rasterizer.

The scene holds many randomly chosen meshes, randomly chosen textures and
random transforms, so the whole pipeline (clipping, projection, depth test,
perspective-correct UVs) gets exercised at once.

Meshes are re-centred analytically instead of relying on their authored origin.
SceneLoader builds `model = T * Rz * Ry * Rx * S`, so to land the bounding box
centre of a mesh on a chosen world point we have to emit

    T = target - Rz * Ry * Rx * (S * centre)

Otherwise every object would orbit its own offset origin and the layout would
not line up with the camera.

Only the standard library is used. Textures can also be generated procedurally
(checker/stripes/plaid/noise/blocks/rings/gradient/mandelbrot) and written as
PNGs, which doubles as a check of the PNG reader.

Examples:
    tools/generate_scene.py --count 24 --seed 1 --render
    tools/generate_scene.py --count 60 --layout scatter --procedural 8 --cull
    tools/generate_scene.py --mesh cube torus --no-texture-ratio 0.5
    tools/generate_scene.py --layout shell --distance 14 --flip-ratio 0.5
"""

from __future__ import annotations

import argparse
import json
import math
import os
import random
import struct
import subprocess
import sys
import zlib
from dataclasses import dataclass

Vec3 = tuple[float, float, float]

TEXTURE_EXTENSIONS = (".png", ".ppm")
UNSUPPORTED_EXTENSIONS = (".jpg", ".jpeg", ".bmp", ".tga", ".gif", ".webp")
PATTERN_KINDS = ("checker", "stripes", "plaid", "noise", "blocks", "rings",
                 "gradient", "mandelbrot")
LAYOUTS = ("grid", "scatter", "spiral", "stack", "shell")
GOLDEN_ANGLE = math.pi * (3.0 - math.sqrt(5.0))

# Interesting corners of the Mandelbrot set, as (real, imaginary).
MANDELBROT_SPOTS = ((-0.743643887037151, 0.131825904205330),
                   (-0.235125, 0.827215),
                   (-0.1011, 0.9563),
                   (-1.25066, 0.02012),
                   (-0.16070135, 1.0375665),
                   (0.2929859127507, 0.6117848324958))

# A world position chosen by one of the layout helpers below.
Placement = Vec3


# --------------------------------------------------------------------------
# mesh stats
# --------------------------------------------------------------------------


@dataclass(frozen=True)
class MeshInfo:
    path: str
    low: Vec3
    high: Vec3
    vertices: int
    faces: int
    has_uv: bool

    @property
    def center(self) -> Vec3:
        return ((self.low[0] + self.high[0]) / 2.0,
                (self.low[1] + self.high[1]) / 2.0,
                (self.low[2] + self.high[2]) / 2.0)

    @property
    def longest(self) -> float:
        return max(self.high[0] - self.low[0], self.high[1] - self.low[1],
                   self.high[2] - self.low[2])


def read_mesh_info(path: str) -> MeshInfo:
    """Scan the `v`, `vt` and `f` lines of an OBJ file."""
    low = [math.inf, math.inf, math.inf]
    high = [-math.inf, -math.inf, -math.inf]
    vertices = 0
    uv_coords = 0
    faces = 0

    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        for line in handle:
            if line.startswith("v "):
                parts = line.split()
                for axis in range(3):
                    value = float(parts[axis + 1])
                    low[axis] = min(low[axis], value)
                    high[axis] = max(high[axis], value)
                vertices += 1
            elif line.startswith("vt "):
                uv_coords += 1
            elif line.startswith("f "):
                faces += 1

    if vertices == 0:
        raise ValueError(f"no vertices in {path}")

    return MeshInfo(path=path,
                    low=(low[0], low[1], low[2]),
                    high=(high[0], high[1], high[2]),
                    vertices=vertices,
                    faces=faces,
                    has_uv=uv_coords > 0)


def discover_meshes(assets_dir: str,
                    filters: list[str]) -> list[MeshInfo]:
    names = sorted(name for name in os.listdir(assets_dir)
                   if name.lower().endswith(".obj"))

    if filters:
        wanted = {name.lower() if name.lower().endswith(".obj")
                  else (name + ".obj").lower() for name in filters}
        names = [name for name in names if name.lower() in wanted]

    if not names:
        raise SystemExit(f"no meshes found in {assets_dir} "
                         f"(filters: {', '.join(filters) or 'none'})")

    return [read_mesh_info(os.path.join(assets_dir, name)) for name in names]


def discover_textures(assets_dir: str,
                      filters: list[str]) -> tuple[list[str], list[str]]:
    names = sorted(name for name in os.listdir(assets_dir)
                   if name.lower().endswith(TEXTURE_EXTENSIONS))

    if filters:
        wanted = {name.lower() if name.lower().endswith(TEXTURE_EXTENSIONS)
                  else (name + ".png").lower() for name in filters}
        names = [name for name in names if name.lower() in wanted]

    if not names:
        raise SystemExit(f"no usable textures in {assets_dir} "
                         f"(filters: {', '.join(filters) or 'none'})")

    unsupported = sorted(name for name in os.listdir(assets_dir)
                         if name.lower().endswith(UNSUPPORTED_EXTENSIONS))

    return [os.path.join(assets_dir, name) for name in names], unsupported


# --------------------------------------------------------------------------
# transforms, mirroring src/core/Matrix.cpp and src/app/SceneLoader.cpp
# --------------------------------------------------------------------------


def rotation_matrix(degrees: Vec3) -> list[list[float]]:
    """Rz * Ry * Rx, the linear part SceneLoader builds for rotate_degrees."""
    to_radians = math.pi / 180.0
    ax, ay, az = (math.radians(value) for value in degrees)

    cx, sx = math.cos(ax), math.sin(ax)
    cy, sy = math.cos(ay), math.sin(ay)
    cz, sz = math.cos(az), math.sin(az)

    rot_x = ((1.0, 0.0, 0.0), (0.0, cx, -sx), (0.0, sx, cx))
    rot_y = ((cy, 0.0, sy), (0.0, 1.0, 0.0), (-sy, 0.0, cy))
    rot_z = ((cz, -sz, 0.0), (sz, cz, 0.0), (0.0, 0.0, 1.0))

    result = [[0.0] * 3 for _ in range(3)]
    for row in range(3):
        for col in range(3):
            result[row][col] = sum(
                rot_z[row][k] *
                sum(rot_y[k][j] * rot_x[j][col] for j in range(3))
                for k in range(3))
    return result


def apply_matrix(matrix: list[list[float]], v: Vec3) -> Vec3:
    return (sum(matrix[0][k] * v[k] for k in range(3)),
            sum(matrix[1][k] * v[k] for k in range(3)),
            sum(matrix[2][k] * v[k] for k in range(3)))


def centred_translation(target: Vec3, matrix: list[list[float]], scale: Vec3,
                        center: Vec3) -> Vec3:
    """Translate that lands the scaled, rotated bbox centre on `target`."""
    scaled = (center[0] * scale[0], center[1] * scale[1], center[2] * scale[2])
    rotated = apply_matrix(matrix, scaled)
    return (target[0] - rotated[0], target[1] - rotated[1],
            target[2] - rotated[2])


def frustum_span(distance: float, fov_y_degrees: float,
                 aspect: float) -> tuple[float, float]:
    """Visible width and height of the frustum at `distance`."""
    half_height = distance * math.tan(math.radians(fov_y_degrees) / 2.0)
    return (2.0 * half_height * aspect, 2.0 * half_height)


# --------------------------------------------------------------------------
# procedural textures
# --------------------------------------------------------------------------


def write_png(path: str, width: int, height: int, pixels: bytes) -> None:
    """Write an 8-bit truecolour RGB buffer as a non-interlaced PNG."""

    def chunk(tag: bytes, data: bytes) -> bytes:
        return (struct.pack(">I", len(data)) + tag + data +
                struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF))

    stride = width * 3
    raw = bytearray()
    for y in range(height):
        raw.append(0)  # per-scanline filter type: none
        raw += pixels[y * stride:(y + 1) * stride]

    blob = (b"\x89PNG\r\n\x1a\n" +
            chunk(b"IHDR",
                  struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)) +
            chunk(b"IDAT", zlib.compress(bytes(raw), 6)) +
            chunk(b"IEND", b""))

    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    with open(path, "wb") as handle:
        handle.write(blob)


def random_color(rng: random.Random, min_luma: float = 0.0,
                 max_luma: float = 1.0) -> tuple[int, int, int]:
    """A random colour whose luma lands in the requested band."""
    while True:
        color = tuple(rng.randrange(256) for _ in range(3))
        luma = (0.299 * color[0] + 0.587 * color[1] + 0.114 * color[2]) / 255.0
        if min_luma <= luma <= max_luma:
            return color  # type: ignore[return-value]


def mix(first: tuple[int, int, int],
        second: tuple[int, int, int], t: float) -> tuple[int, int, int]:
    return (int(round(first[0] + (second[0] - first[0]) * t)),
            int(round(first[1] + (second[1] - first[1]) * t)),
            int(round(first[2] + (second[2] - first[2]) * t)))


def paint_pattern(kind: str, size: int,
                  rng: random.Random) -> bytes:
    """Render one procedural texture into an RGB byte buffer."""
    pixels = bytearray(size * size * 3)

    cells = rng.choice((2, 4, 8, 16, 32))
    phase = rng.uniform(0.0, math.tau)
    angle = rng.choice((0.0, math.pi / 4.0, math.pi / 2.0, math.pi / 6.0))
    cos_a, sin_a = math.cos(angle), math.sin(angle)
    dark = random_color(rng, 0.0, 0.35)
    light = random_color(rng, 0.65, 1.0)
    block = rng.choice((4, 8, 16, 32))
    spot_x, spot_y = rng.choice(MANDELBROT_SPOTS)
    # Width of the slice of the complex plane that fits into the image.
    span = 3.0 / rng.uniform(1.5, 60.0)
    max_iterations = rng.choice((24, 32, 48))
    middle = mix(dark, light, 0.5)
    center = size / 2.0

    for y in range(size):
        for x in range(size):
            index = (y * size + x) * 3

            if kind == "checker":
                color = light if ((x * cells) // size +
                                  (y * cells) // size) % 2 else dark
            elif kind == "stripes":
                wave = math.sin((x * cos_a + y * sin_a) * cells * 0.35 +
                                phase)
                color = light if wave > 0.0 else dark
            elif kind == "plaid":
                cell = ((x * cells) // size + (y * cells) // size) % 2
                wave = math.sin((x * cos_a + y * sin_a) * cells * 0.35 +
                                phase) > 0.0
                color = (light if wave else dark) if cell else \
                    (dark if wave else light)
            elif kind == "noise":
                color = tuple(rng.randrange(256) for _ in range(3))  # type: ignore[assignment]
            elif kind == "blocks":
                cell = (x // block + (y // block) * 7) % 2
                color = light if cell else dark
            elif kind == "rings":
                radius = math.hypot(x - center, y - center)
                color = light if int(radius * cells / 8.0) % 2 else dark
            elif kind == "gradient":
                t = ((x * cos_a + y * sin_a) / (size + size) + 0.5)
                color = mix(dark, light, min(max(t, 0.0), 1.0))
            else:  # mandelbrot, shaded by escape time
                zx = spot_x + (x - center) * span / size
                zy = spot_y + (y - center) * span / size
                cr, ci = zx, zy
                iterations = max_iterations
                for step in range(max_iterations):
                    zx, zy = zx * zx - zy * zy + cr, 2.0 * zx * zy + ci
                    if zx * zx + zy * zy > 4.0:
                        iterations = step
                        break
                color = mix(dark, light, iterations / max_iterations)

            pixels[index:index + 3] = bytes(color)

    if kind in ("noise", "mandelbrot"):
        # Lift the darkest samples a little so the pattern stays visible
        # against a dark background.
        for i in range(0, len(pixels), 3):
            pixels[i] = min(255, pixels[i] + middle[0] // 4)
            pixels[i + 1] = min(255, pixels[i + 1] + middle[1] // 4)
            pixels[i + 2] = min(255, pixels[i + 2] + middle[2] // 4)

    return bytes(pixels)


def generate_textures(directory: str, count: int, size: int,
                      rng: random.Random) -> list[str]:
    """Write `count` procedural textures and return their paths."""
    os.makedirs(directory, exist_ok=True)
    paths = []

    for index in range(count):
        kind = PATTERN_KINDS[index % len(PATTERN_KINDS)]
        path = os.path.join(directory, f"proc_{kind}_{index:02d}.png")
        write_png(path, size, size, paint_pattern(kind, size, rng))
        paths.append(path)

    return paths


# --------------------------------------------------------------------------
# layouts
# --------------------------------------------------------------------------


def place(index: int, count: int, rng: random.Random, layout: str,
          span_x: float, span_y: float, distance: float,
          jitter: float) -> Placement:
    """Pick the world position of one object."""
    if layout == "scatter":
        return (rng.uniform(-span_x / 2.0, span_x / 2.0),
                rng.uniform(-span_y / 2.0, span_y / 2.0),
                -distance + rng.uniform(-span_y / 4.0, span_y / 4.0))

    if layout == "spiral":
        radius = span_x * 0.45 * math.sqrt((index + 0.5) / count)
        angle = index * GOLDEN_ANGLE
        return (radius * math.cos(angle), radius * math.sin(angle),
                -distance * (0.35 + 1.3 * index / max(count - 1, 1)))

    if layout == "stack":
        # Everything piled on one spot: interpenetration and depth fights.
        return (rng.gauss(0.0, span_x * 0.10), rng.gauss(0.0, span_y * 0.10),
                -distance + rng.gauss(0.0, span_y * 0.06))

    if layout == "shell":
        # A cloud around the camera, so plenty of back faces face away.
        forward = rng.uniform(0.25, 1.0)
        planar = math.sqrt(max(0.0, 1.0 - forward * forward))
        theta = rng.uniform(0.0, math.tau)
        radius = distance * rng.uniform(0.7, 1.4)
        return (radius * planar * math.cos(theta),
                radius * planar * math.sin(theta), -radius * forward)

    columns = max(1, math.ceil(math.sqrt(count * span_x / max(span_y, 1e-6))))
    rows = max(1, math.ceil(count / columns))
    cell_x = span_x / columns
    cell_y = span_y / rows
    column = index % columns
    row = index // columns

    return ((column + 0.5) * cell_x - span_x / 2.0 +
            rng.uniform(-jitter, jitter) * cell_x,
            span_y / 2.0 - (row + 0.5) * cell_y +
            rng.uniform(-jitter, jitter) * cell_y,
            -distance + rng.uniform(-jitter, jitter) * distance * 0.5)


# --------------------------------------------------------------------------
# scene assembly
# --------------------------------------------------------------------------


def rounded(values, digits: int) -> list[float]:
    return [round(value, digits) for value in values]


def build_object(index: int, count: int, rng: random.Random, meshes: list[MeshInfo],
                 textures: list[str], args: argparse.Namespace,
                 span_x: float, span_y: float) -> dict:
    mesh = rng.choice(meshes)

    # Normalise every mesh to a comparable world size, then stretch it.
    uniform_scale = rng.uniform(args.min_size, args.max_size) / max(
        mesh.longest, 1e-6)
    scale = [uniform_scale * rng.uniform(1.0 / args.stretch, args.stretch)
             for _ in range(3)]

    if rng.random() < args.flip_ratio:
        scale[rng.randrange(3)] *= -1.0

    rotation = (rng.uniform(0.0, 360.0), rng.uniform(0.0, 360.0),
                rng.uniform(0.0, 360.0))

    target = list(place(index, count, rng, args.layout, span_x, span_y,
                        args.distance, args.jitter))

    if rng.random() < args.clip_ratio:
        # Straddle the near plane so the clipper runs on real geometry.
        target[2] = -args.near * rng.uniform(0.15, 3.0)

    matrix = rotation_matrix(rotation)
    translation = centred_translation(tuple(target), matrix, tuple(scale),
                                      mesh.center)

    obj: dict = {"mesh": mesh.path}
    if rng.random() >= args.no_texture_ratio:
        obj["texture"] = rng.choice(textures)
    obj["translate"] = rounded(translation, args.digits)
    obj["rotate_degrees"] = rounded(rotation, args.digits)
    obj["scale"] = rounded(tuple(scale), args.digits)
    return obj


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate a random stress-test scene for the rasterizer.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)

    group = parser.add_argument_group("output")
    group.add_argument("--out", default="out/random_scene.json",
                       help="where to write the scene JSON")
    group.add_argument("--output", default=None,
                       help="render target; defaults to the scene path "
                            "with a .png extension")
    group.add_argument("--width", type=int, default=1024)
    group.add_argument("--height", type=int, default=1024)
    group.add_argument("--digits", type=int, default=6,
                       help="decimals kept in the emitted numbers")
    group.add_argument("--quiet", action="store_true")
    group.add_argument("--render", action="store_true",
                       help="run the rasterizer on the generated scene")
    group.add_argument("--rasterizer", default="build/rasterizer")

    group = parser.add_argument_group("content")
    group.add_argument("--seed", type=int, default=None,
                       help="random seed; omitted picks a fresh one")
    group.add_argument("--count", type=int, default=24,
                       help="number of objects")
    group.add_argument("--assets", default="assets",
                       help="directory with .obj meshes and textures")
    group.add_argument("--mesh", nargs="*", default=[],
                       help="restrict to these meshes (name without .obj)")
    group.add_argument("--texture", nargs="*", default=[],
                       help="restrict to these textures (name without extension)")

    group = parser.add_argument_group("transforms")
    group.add_argument("--layout", default="grid", choices=LAYOUTS)
    group.add_argument("--distance", type=float, default=10.0,
                       help="camera distance to the layout centre")
    group.add_argument("--min-size", type=float, default=1.2,
                       help="smallest object size in world units")
    group.add_argument("--max-size", type=float, default=2.6,
                       help="largest object size in world units")
    group.add_argument("--stretch", type=float, default=1.0,
                       help="max relative non-uniform scale per axis")
    group.add_argument("--flip-ratio", type=float, default=0.1,
                       help="chance of a negative scale axis (flips winding)")
    group.add_argument("--jitter", type=float, default=0.25,
                       help="how far objects wander from their slot")
    group.add_argument("--no-texture-ratio", type=float, default=0.15,
                       help="chance of leaving an object untextured")
    group.add_argument("--clip-ratio", type=float, default=0.15,
                       help="chance of placing an object across the near plane")

    group = parser.add_argument_group("textures")
    group.add_argument("--procedural", type=int, default=0, metavar="N",
                       help="generate N random PNG textures and add them "
                            "to the pool")
    group.add_argument("--procedural-size", type=int, default=256)
    group.add_argument("--procedural-dir", default="out/textures")

    group = parser.add_argument_group("camera")
    group.add_argument("--fov", type=float, default=60.0)
    group.add_argument("--near", type=float, default=0.1)
    group.add_argument("--far", type=float, default=None)
    group.add_argument("--cull", action="store_true",
                       help="enable back-face culling")
    group.add_argument("--background", type=float, nargs=3, default=None,
                       metavar=("R", "G", "B"),
                       help="background colour in 0..1; omit for a random "
                            "dark one. 0..255 values are rejected, divide "
                            "them by 255 first")

    return parser.parse_args(argv)


def validate(args: argparse.Namespace) -> None:
    def require(condition: bool, message: str) -> None:
        if not condition:
            raise SystemExit(message)

    require(args.count > 0, "--count must be positive")
    require(args.width > 0 and args.height > 0,
            "--width and --height must be positive")
    require(args.near > 0.0, "--near must be positive")
    require(args.stretch >= 1.0, "--stretch must be at least 1.0")
    require(args.min_size > 0.0, "--min-size must be positive")
    require(args.max_size >= args.min_size,
            "--max-size must not be below --min-size")
    require(args.distance > args.near,
            "--distance must be beyond the near plane")
    require(args.procedural >= 0, "--procedural must not be negative")
    require(args.procedural_size >= 4, "--procedural-size must be at least 4")

    if args.background is not None:
        # The framebuffer stores colours as 0..1 doubles and ToByte clamps
        # them, so a 0..255 value would silently render as pure white.
        bad = [value for value in args.background
               if not 0.0 <= value <= 1.0]
        require(not bad, f"--background components must be within [0, 1], got "
                         f"{args.background}; divide by 255 for 8-bit values")
    for name in ("no_texture_ratio", "clip_ratio", "flip_ratio"):
        value = getattr(args, name)
        require(0.0 <= value <= 1.0, f"--{name.replace('_', '-')} must be "
                                     "within [0, 1]")


def report(args: argparse.Namespace, meshes: list[MeshInfo],
           textures: list[str], unsupported: list[str],
           generated: list[str], objects: list[dict],
           seed: int, far: float) -> None:
    print(f"seed {seed}")
    print(f"meshes ({len(meshes)}):")
    for mesh in meshes:
        print(f"  {os.path.basename(mesh.path):18} "
              f"verts={mesh.vertices:5} faces={mesh.faces:5} "
              f"size={mesh.longest:5.2f} "
              f"{'vt' if mesh.has_uv else 'no vt (spherical UV)'}")
    print(f"textures ({len(textures)}): "
          f"{', '.join(os.path.basename(t) for t in textures)}")
    for path in generated:
        print(f"  generated {path}")
    if unsupported:
        print(f"ignored (no decoder): {', '.join(unsupported)}")

    untextured = sum(1 for obj in objects if "texture" not in obj)
    clipped = sum(1 for obj in objects
                  if abs(obj["translate"][2]) < args.near * 8.0)
    print(f"\n{len(objects)} objects, {args.width}x{args.height}, "
          f"cull={args.cull}, near={args.near}, far={far}")
    print(f"untextured: {untextured}, near the camera: {clipped}")

    print(f"\n{'#':>3}  {'mesh':16} {'texture':22} "
          f"{'translate':26} {'rotate':26} scale")
    for index, obj in enumerate(objects):
        print(f"{index:3}  {os.path.basename(obj['mesh']):16} "
              f"{os.path.basename(obj.get('texture', '-')):22} "
              f"{str(obj['translate']):26} {str(obj['rotate_degrees']):26} "
              f"{[round(v, 3) for v in obj['scale']]}")


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    validate(args)

    seed = args.seed if args.seed is not None else random.randrange(1 << 30)
    rng = random.Random(seed)

    meshes = discover_meshes(args.assets, args.mesh)
    textures, unsupported = discover_textures(args.assets, args.texture)

    generated: list[str] = []
    if args.procedural > 0:
        generated = generate_textures(args.procedural_dir, args.procedural,
                                      args.procedural_size, rng)
        textures = textures + generated

    span_x, span_y = frustum_span(args.distance, args.fov,
                                  args.width / args.height)
    far = args.far if args.far is not None else max(100.0, args.distance * 8.0)

    objects = [build_object(index, args.count, rng, meshes, textures, args,
                            span_x, span_y)
               for index in range(args.count)]

    background = args.background
    if background is None:
        # Dark and slightly desaturated so black and white textures read well.
        background = [round(rng.uniform(0.04, 0.18), args.digits)] * 3

    output = args.output or (os.path.splitext(args.out)[0] + ".png")

    scene = {
        "width": args.width,
        "height": args.height,
        "background": background,
        "output": output,
        "cull_back_faces": args.cull,
        "camera": {
            "position": [0.0, 0.0, 0.0],
            "look_at": [0.0, 0.0, -1.0],
            "up": [0.0, 1.0, 0.0],
            "fov_y_degrees": args.fov,
            "near": args.near,
            "far": far,
        },
        "objects": objects,
    }

    os.makedirs(os.path.dirname(args.out) or ".", exist_ok=True)
    with open(args.out, "w", encoding="utf-8") as handle:
        json.dump(scene, handle, indent=2)
        handle.write("\n")

    if not args.quiet:
        report(args, meshes, textures, unsupported, generated, objects, seed,
               far)
        print(f"\nwrote {args.out} -> {output}")

    if args.render:
        if not os.path.exists(args.rasterizer):
            raise SystemExit(f"rasterizer not found: {args.rasterizer}")
        print(f"\n$ {args.rasterizer} {args.out}")
        return subprocess.run([args.rasterizer, args.out], check=False).returncode

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
