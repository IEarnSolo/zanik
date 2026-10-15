import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools.quest_schema import objective_rows


legacy = {
    "ObjectiveTexts": [{"Key": "Find", "Value": {"SourceString": "Find the target"}}]
}
current = {
    "Objectives": [{"Key": "Find", "Value": {
        "Description": {"SourceString": "Find the target"},
        "Hint": {"SourceString": "Look nearby"},
    }}]
}
expected = [{"Key": "Find", "Value": {"SourceString": "Find the target"}}]
assert objective_rows(legacy) == expected
assert objective_rows(current) == expected
assert objective_rows({}) == []
assert objective_rows({"Objectives": [{"Key": "", "Value": {"Description": {}}}]}) == []
print("quest schema tests passed")
