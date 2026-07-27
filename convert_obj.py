"""Triangulate a Wavefront .obj so it matches what model.cpp expects.

model.cpp requires every face line to be exactly 3 vertices in
"v/t/n" form (v, t, n all present). FinalBaseMesh.obj has:
  - quads instead of triangles (4 verts per face)
  - "v//n" form (no texture index)

This script fan-triangulates each polygon face and inserts a dummy
texture index (model.cpp never reads it, so any int works) so every
face line becomes "f v/t/n v/t/n v/t/n".
"""
import sys


def convert(in_path, out_path):
    out_lines = []
    face_count = 0
    tri_count = 0

    with open(in_path, "r") as f:
        for line in f:
            if line.startswith("f "):
                face_count += 1
                tokens = line.split()[1:]
                # each token looks like "v", "v/t", "v//n" or "v/t/n"
                verts = []
                for tok in tokens:
                    parts = tok.split("/")
                    v = parts[0]
                    n = parts[2] if len(parts) > 2 and parts[2] != "" else v
                    verts.append((v, n))

                # fan triangulation: (0,1,2), (0,2,3), (0,3,4), ...
                for i in range(1, len(verts) - 1):
                    tri = [verts[0], verts[i], verts[i + 1]]
                    face_str = " ".join(f"{v}/{v}/{n}" for v, n in tri)
                    out_lines.append(f"f {face_str}\n")
                    tri_count += 1
            else:
                out_lines.append(line)

    with open(out_path, "w") as f:
        f.writelines(out_lines)

    print(f"faces: {face_count} polygons -> {tri_count} triangles")
    print(f"written to {out_path}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("usage: python convert_obj.py <input.obj> <output.obj>")
        sys.exit(1)
    convert(sys.argv[1], sys.argv[2])
