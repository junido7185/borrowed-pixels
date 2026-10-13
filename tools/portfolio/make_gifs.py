"""Turn headless_capture frames into the README GIFs (nearest-neighbour 2x)."""
import sys
from pathlib import Path

from PIL import Image

CLIPS = {  # name: (first tick, last tick, frame duration ms); capture step is 3 ticks
    "title": (0, 380, 100),
    "combat": (380, 760, 50),
    "settlement": (760, 1100, 50),
    "encounter3": (2040, 2700, 50),
}


def main(src: str, dst: str, step: int = 3) -> None:
    out = Path(dst)
    out.mkdir(parents=True, exist_ok=True)
    for name, (first, last, duration) in CLIPS.items():
        stride = step * (2 if duration >= 100 else 1)
        frames = [
            Image.open(Path(src) / f"f{tick // step:05d}.ppm")
            .resize((640, 360), Image.NEAREST)
            .convert("P", palette=Image.ADAPTIVE, colors=64)
            for tick in range(first, last, stride)
        ]
        frames[0].save(out / f"{name}.gif", save_all=True, append_images=frames[1:],
                       duration=duration, loop=0, optimize=True)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
