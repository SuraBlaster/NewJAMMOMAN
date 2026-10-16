import json
import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

source, destination = map(Path, sys.argv[1:3])
stage = json.loads(source.read_text(encoding="utf-8"))
tiles = []

for layer in stage["layers"]:
    if layer["type"] != "tilelayer":
        continue

    for chunk in layer["chunks"]:
        for index, value in enumerate(chunk["data"]):
            if value:
                tiles.append(
                    (
                        chunk["x"] + index % chunk["width"],
                        -(chunk["y"] + index // chunk["width"]),
                    )
                )

scale = 18
left = min(x for x, y in tiles) - 2
right = max(x for x, y in tiles) + 3
top = max(25, max(y for x, y in tiles) + 4)
bottom = min(y for x, y in tiles) - 3
image = Image.new("RGB", ((right - left) * scale + 40, (top - bottom) * scale + 40), "#101822")
draw = ImageDraw.Draw(image)


def point(x, y):
    return ((x - left) * scale + 20, (top - y) * scale + 20)


for x in range(left, right + 1):
    draw.line((point(x, top), point(x, bottom)), fill="#253748")

for y in range(bottom, top + 1):
    draw.line((point(left, y), point(right, y)), fill="#253748")

for x, y in tiles:
    px, py = point(x - .5, y + .5)
    draw.rectangle((px, py, px + scale, py + scale), fill="#8895a3", outline="#c0c8cf")

font = ImageFont.truetype("C:/Windows/Fonts/meiryo.ttc", 16)

for layer in stage["layers"]:
    if not layer["name"].startswith("Guide_"):
        continue

    for obj in layer["objects"]:
        x, y = obj["x"] / 32, -obj["y"] / 32

        if "polyline" in obj:
            draw.line(
                [point(x + p["x"] / 32, y - p["y"] / 32) for p in obj["polyline"]],
                fill=layer["color"],
                width=2,
            )

        if "text" in obj:
            draw.text(point(x, y), obj["text"]["text"], font=font, fill=obj["text"]["color"])

destination.parent.mkdir(parents=True, exist_ok=True)
image.save(destination)
