#!/usr/bin/env python3
import json
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class MaterialActivityCatalogTest(unittest.TestCase):
    def test_only_first_pickup_items_are_emitted(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "assets"
            source.mkdir()
            (source / "items.json").write_text(json.dumps([
                {"Name": "ITEM_Fang", "Properties": {
                    "Name": {"LocalizedString": "Small Animal Fang"},
                    "bShowNotificationOnFirstPickup": True}},
                {"Name": "ITEM_Ordinary", "Properties": {
                    "Name": {"LocalizedString": "Ordinary"},
                    "bShowNotificationOnFirstPickup": False}},
            ]), encoding="utf-8")
            output = root / "catalog.tsv"
            subprocess.run([
                "python3", str(ROOT / "tools/generate_material_activity_catalog.py"),
                "--source", str(source), "--output", str(output),
            ], check=True, capture_output=True, text=True)
            lines = output.read_text(encoding="utf-8").splitlines()
            self.assertIn("count=1", lines[0])
            self.assertEqual(lines[1],
                             "ITEM_Fang\tSmall Animal Fang\t/assets/items/ITEM_Fang.png")
            self.assertEqual(len(lines), 2)


if __name__ == "__main__":
    unittest.main()
