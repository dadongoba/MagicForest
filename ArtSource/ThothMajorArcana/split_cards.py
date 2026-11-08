"""Losslessly split the supplied 2-by-11 major-arcana sheet into UE import PNGs."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "Source" / "森林科技塔罗牌阵列.png"
CARD_DIR = ROOT / "Cards"
PREVIEW = ROOT / "Preview" / "contact_sheet.png"
MANIFEST = ROOT / "major_arcana_manifest.json"

# The black gutters were measured on both rows. Boundaries lie inside gutters,
# so no border or printed title is trimmed from any card.
X_EDGES = [0, 139, 277, 417, 556, 698, 838, 978, 1120, 1260, 1400, 1536]
Y_EDGES = [0, 512, 1024]
CANVAS_SIZE = (142, 512)

# id is the authoritative FName used by UAlchemyGameplaySubsystem::MakeMajorCards.
CARDS = [
    ("0", "Fool", "愚者", "THE FOOL"),
    ("I", "Magician", "魔术师", "THE MAGICIAN"),
    ("II", "HighPriestess", "女祭司", "THE HIGH PRIESTESS"),
    ("III", "Empress", "女皇", "THE EMPRESS"),
    ("IV", "Emperor", "皇帝", "THE EMPEROR"),
    ("V", "Hierophant", "教皇", "THE HIEROPHANT"),
    ("VI", "Lovers", "恋人", "THE LOVERS"),
    ("VII", "Chariot", "战车", "THE CHARIOT"),
    ("VIII", "Adjustment", "调整", "ADJUSTMENT"),
    ("IX", "Hermit", "隐者", "THE HERMIT"),
    ("X", "Fortune", "命运之轮", "FORTUNE"),
    ("XI", "Lust", "欲望", "LUST"),
    ("XII", "HangedMan", "倒吊人", "THE HANGED MAN"),
    ("XIII", "Death", "死神", "DEATH"),
    ("XIV", "Art", "艺术", "ART"),
    ("XV", "Devil", "恶魔", "THE DEVIL"),
    ("XVI", "Tower", "塔", "THE TOWER"),
    ("XVII", "Star", "星", "THE STAR"),
    ("XVIII", "Moon", "月", "THE MOON"),
    ("XIX", "Sun", "太阳", "THE SUN"),
    ("XX", "Aeon", "永劫", "THE AEON"),
    ("XXI", "Universe", "宇宙", "THE UNIVERSE"),
]


def main() -> None:
    source_bytes = SOURCE.read_bytes()
    image = Image.open(SOURCE)
    if image.size != (1536, 1024) or image.mode != "RGB":
        raise ValueError(f"Unexpected source format: {image.size}, {image.mode}")
    if len(CARDS) != 22 or len({entry[0] for entry in CARDS}) != 22:
        raise ValueError("Expected 22 unique game card IDs")

    CARD_DIR.mkdir(parents=True, exist_ok=True)
    PREVIEW.parent.mkdir(parents=True, exist_ok=True)
    records = []
    contact = Image.new("RGB", (6 * 164, 4 * 552), (24, 23, 22))
    draw = ImageDraw.Draw(contact)
    font = ImageFont.load_default()

    for index, (game_id, slug, chinese, printed) in enumerate(CARDS):
        row, column = divmod(index, 11)
        box = (X_EDGES[column], Y_EDGES[row], X_EDGES[column + 1], Y_EDGES[row + 1])
        crop = image.crop(box)
        output = Image.new("RGB", CANVAS_SIZE, (0, 0, 0))
        output.paste(crop, ((CANVAS_SIZE[0] - crop.width) // 2, 0))
        filename = f"Major_{index:02d}_{slug}.png"
        path = CARD_DIR / filename
        output.save(path, format="PNG", optimize=True)
        if Image.open(path).size != CANVAS_SIZE:
            raise AssertionError(filename)

        px, py = (index % 6) * 164 + 11, (index // 6) * 552 + 25
        contact.paste(output, (px, py))
        draw.text((px, py - 17), f"{index:02d} / {game_id}", font=font, fill=(235, 220, 185))
        records.append({
            "index": index,
            "game_id": game_id,
            "name_zh": chinese,
            "printed_name_en": printed,
            "file": f"Cards/{filename}",
            "source_box_xyxy": list(box),
            "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
        })

    contact.save(PREVIEW, format="PNG", optimize=True)
    MANIFEST.write_text(
        json.dumps({
            "source_file": f"Source/{SOURCE.name}",
            "source_sha256": hashlib.sha256(source_bytes).hexdigest(),
            "source_size": list(image.size),
            "card_size": list(CANVAS_SIZE),
            "format": "RGB PNG, sRGB source, unchanged crop with black gutter padding",
            "game_id_reference": "UAlchemyGameplaySubsystem::MakeMajorCards",
            "cards": records,
        }, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
