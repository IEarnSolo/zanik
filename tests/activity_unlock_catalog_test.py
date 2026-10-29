#!/usr/bin/env python3
import json
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def localized(value: str) -> dict[str, str]:
    return {"SourceString": value, "LocalizedString": value}


def perk(key: str, name: str, texture: str, spell: str = "") -> list[dict]:
    records = [{
        "Type": "SkillPerkData", "Name": key,
        "Properties": {
            "PerkName": localized(name),
            "SkillPerkImage": {"AssetPathName": f"/Game/UI/{texture}.{texture}"},
        },
    }]
    if spell:
        records.append({
            "Type": "SkillPerkModule_Spell", "Name": "SkillPerkModule_Spell_0",
            "Outer": {"ObjectName": f"SkillPerkData'{key}'"},
            "Properties": {"SpellData": {"ObjectName": f"UtilitySpellData'{spell}'"}},
        })
    return records


class ActivityUnlockCatalogTest(unittest.TestCase):
    def test_spell_link_is_canonicalized_without_losing_authored_metadata(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary); source = root / "assets"; source.mkdir()
            (source / "tempest.json").write_text(json.dumps(
                perk("PerkV2_Attack_TempestShield", "Tempest Shield",
                     "T_Skill_Attack_Spell_Tempestshield", "USD_TempestShield")), encoding="utf-8")
            (source / "ordinary.json").write_text(json.dumps(
                perk("PerkV2_Attack_Parry", "Perfect Parry", "T_Skill_Attack_Parry")), encoding="utf-8")
            output = root / "catalog.tsv"
            subprocess.run([
                "python3", str(ROOT / "tools/generate_activity_unlock_catalog.py"),
                "--source", str(source), "--output", str(output),
            ], check=True, capture_output=True, text=True)
            lines = output.read_text(encoding="utf-8").splitlines()
            self.assertIn("count=2", lines[0])
            self.assertIn(
                "PerkV2_Attack_TempestShield\tspell\tUSD_TempestShield\tTempest Shield\t"
                "/assets/spells-and-perks/PerkV2_Attack_TempestShield.png\tT_Skill_Attack_Spell_Tempestshield",
                lines,
            )
            self.assertIn(
                "PerkV2_Attack_Parry\tperk\tPerkV2_Attack_Parry\tPerfect Parry\t"
                "/assets/spells-and-perks/PerkV2_Attack_Parry.png\tT_Skill_Attack_Parry",
                lines,
            )


if __name__ == "__main__":
    unittest.main()
