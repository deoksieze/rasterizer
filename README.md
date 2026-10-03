# rasterizer

A small CPU rasterizer written in C++20. It loads a scene from a JSON file, renders it
in software (model/view/projection, clipping, depth buffer, perspective-correct texture
mapping, back-face culling) and writes the result to a PNG or PPM image.

Everything runs offline; there is no window and no GPU.

## Requirements

- Linux
- CMake >= 3.20
- A C++20 compiler (GCC or Clang)
- libpng development headers

On Debian/Ubuntu:

```sh
sudo apt install cmake g++ libpng-dev
```

`nlohmann/json` is vendored in `third_party/`, so no extra download is needed.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Run

Run from the repository root, because scene and asset paths are relative:

```sh
./build/rasterizer                       # renders assets/scene.json
./build/rasterizer path/to/scene.json    # renders a specific scene
```

The output path comes from the scene file. On success the program prints where it wrote
the image.

## Tests

```sh
ctest --test-dir build --output-on-failure
```

## Describing a scene

A scene is a JSON object. Only `objects` is required; everything else has a default.

```json
{
  "width": 512,
  "height": 512,
  "background": [0.25098039215686274, 0.25098039215686274, 0.25098039215686274],
  "output": "out/render.png",
  "cull_back_faces": false,
  "camera": {
    "position": [0, 0, 0],
    "look_at": [0, 0, -1],
    "up": [0, 1, 0],
    "fov_y_degrees": 60,
    "near": 0.1,
    "far": 100
  },
  "objects": [
    {
      "mesh": "assets/teapot.obj",
      "texture": "assets/Ruslan_texture.ppm",
      "translate": [0, 0, -10],
      "rotate_degrees": [30, 36, 0],
      "scale": [1, 1, 1]
    }
  ]
}
```

This is exactly `assets/scene.json`, the file rendered by default.

### Top-level fields

| Field             | Type            | Default            | Meaning                                                        |
| ----------------- | --------------- | ------------------ | -------------------------------------------------------------- |
| `width`, `height` | int             | `512`              | Image size in pixels. Both must be positive.                   |
| `background`      | 3 numbers (0-1) | dark gray (64/255) | Clear color.                                                   |
| `output`          | string          | `out/render.png`   | Output file; `.png` or `.ppm` selects the format.              |
| `cull_back_faces` | bool            | `false`            | Skip triangles facing away from the camera.                    |
| `camera`          | object          | see below          | Camera description.                                            |
| `objects`         | array           | required           | Non-empty list of drawables.                                   |

### Camera fields

| Field            | Type            | Default    | Meaning                                             |
| ---------------- | --------------- | ---------- | --------------------------------------------------- |
| `position`       | 3 numbers       | `[0,0,0]`  | Eye position.                                       |
| `look_at`        | 3 numbers       | `[0,0,-1]` | Point the camera looks at.                          |
| `up`             | 3 numbers       | `[0,1,0]`  | Up direction.                                       |
| `fov_y_degrees`  | number          | `60`       | Vertical field of view.                             |
| `near`, `far`    | number          | `0.1`, `100` | Near/far clipping planes.                         |

The camera looks down its own `-Z` axis, so with the default camera objects are visible
at negative Z (for example `translate: [0, 0, -10]`).

### Object fields

| Field            | Type            | Default       | Meaning                                              |
| ---------------- | --------------- | ------------- | ---------------------------------------------------- |
| `mesh`           | string          | required      | Path to an OBJ file.                                 |
| `texture`        | string          | none          | Path to a texture (see below).                       |
| `translate`      | 3 numbers       | `[0,0,0]`     | Position.                                            |
| `rotate_degrees` | 3 numbers       | `[0,0,0]`     | Rotation around X, Y, Z in degrees.                  |
| `scale`          | 3 numbers       | `[1,1,1]`     | Per-axis scale.                                      |

The object transform is applied to each vertex as `T * Rz * Ry * Rx * S` (scale first,
then rotations X, Y, Z, then translation).

### Adding objects

Add another entry to the `objects` array. Each object is independent and can use its own
mesh, texture and transform:

```json
"objects": [
  {
    "mesh": "assets/teapot.obj",
    "texture": "assets/Ruslan_texture.ppm",
    "translate": [-1.5, 0, -8],
    "rotate_degrees": [0, 40, 0]
  },
  {
    "mesh": "assets/manifold.obj",
    "translate": [1.5, 0, -8],
    "rotate_degrees": [20, 0, 15],
    "scale": [0.8, 0.8, 0.8]
  }
]
```

## Meshes

Meshes are Wavefront OBJ files (`.obj`). The loader understands:

- `v`, `vt`, `vn` and `f` records
- faces with 3 or more vertices (polygons are fan-triangulated)
- face tokens in the forms `v`, `v/vt`, `v//vn` and `v/vt/vn`
- positive and negative (relative) indices
- `#` comments and CRLF line endings

Other directives (`o`, `g`, `s`, `usemtl`, `mtllib`, ...) are ignored. Normals are parsed
but not currently used for shading.

## Textures

Point an object at a texture with its `texture` field:

```json
{ "mesh": "assets/teapot.obj", "texture": "assets/Ruslan_texture.ppm" }
```

Rules:

- Textures are **PNG** or **binary PPM (P6)**; the format is chosen from the file extension.
  PNG inputs go through libpng, and every PNG flavour is accepted: palette, grayscale,
  RGB, RGBA, 1-16 bits per channel and interlaced. Anything that is not 8-bit RGB is
  converted on load — 16-bit samples are reduced to their most significant 8 bits and the
  alpha channel is discarded, so the colour underneath a transparent pixel is kept as
  authored rather than blended.
- UVs come from the mesh's `vt` data and are interpolated perspective-correctly.
- If the mesh has no `vt` at all (`teapot.obj`, `4.obj`, `manifold.obj` and
  `triangle.obj` are all in this situation), a spherical projection is applied
  automatically so the texture still wraps around the model.
- If `texture` is omitted, the object is drawn with a flat per-triangle debug color
  instead.

To place a texture precisely, author `vt` coordinates in the OBJ; otherwise the automatic
spherical mapping is used.

## Output

The format is chosen from the `output` extension:

- `.png` — PNG, written with libpng (recommended; much smaller).
- `.ppm` — ASCII PPM (P3); larger, but diff-friendly.

Missing parent directories are created automatically.

## Project layout

```
src/main.cpp            entry point: load scene, render, save
src/app/                Scene struct, JSON loader, render orchestration
src/raster/             clipping, projection, triangle rasterization, framebuffer
src/geometry/           mesh types and UV projection
src/io/                 OBJ loader, PNG/PPM readers and writers
src/core/               vectors, matrices, colors, textures
tests/                  CTest unit tests and a golden image
third_party/nlohmann/   vendored JSON library
assets/                 example scene, meshes and textures
```

See [Project_description.md](Project_description.md) for the project's goals and roadmap.
