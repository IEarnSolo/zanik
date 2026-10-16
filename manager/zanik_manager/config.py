from __future__ import annotations

import hashlib
import json
import re
from pathlib import Path
from urllib.parse import urlsplit

TRUE_VALUES = {"1", "true", "yes", "on"}
FALSE_VALUES = {"0", "false", "no", "off"}


class ConfigError(ValueError):
    pass


class ConfigModel:
    def __init__(self, schema_path: Path):
        self.schema = json.loads(schema_path.read_text(encoding="utf-8"))
        self.properties = self.schema["properties"]

    def defaults(self) -> dict[str, object]:
        return {key: spec["default"] for key, spec in self.properties.items()}

    def parse(self, text: str) -> dict[str, object]:
        values = self.defaults()
        seen: set[str] = set()
        for number, raw in enumerate(text.splitlines(), 1):
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            if "=" not in line:
                raise ConfigError(f"Invalid configuration line {number}")
            key, value = (part.strip() for part in line.split("=", 1))
            if key not in self.properties:
                raise ConfigError(f"Unknown configuration key '{key}' at line {number}")
            if key in seen:
                raise ConfigError(f"Duplicate configuration key '{key}' at line {number}")
            seen.add(key)
            values[key] = self._parse_value(key, value, number)
        return values

    def _parse_value(self, key: str, value: str, line: int) -> object:
        spec = self.properties[key]
        if spec["type"] == "boolean":
            normalized = value.lower()
            if normalized in TRUE_VALUES:
                return True
            if normalized in FALSE_VALUES:
                return False
            raise ConfigError(f"Invalid boolean for '{key}' at line {line}")
        if spec["type"] == "integer":
            if not value.isdigit():
                raise ConfigError(f"Invalid integer for '{key}' at line {line}")
            parsed = int(value)
            if parsed < spec.get("minimum", parsed) or parsed > spec.get("maximum", parsed):
                raise ConfigError(f"Value for '{key}' is outside the allowed range at line {line}")
            return parsed
        pattern = spec.get("pattern")
        if pattern and not re.search(pattern, value):
            raise ConfigError(f"Invalid value for '{key}' at line {line}")
        return value

    def render(self, values: dict[str, object]) -> str:
        unknown = set(values) - set(self.properties)
        if unknown:
            raise ConfigError(f"Unknown configuration keys: {', '.join(sorted(unknown))}")
        rows = ["# Managed by Zanik Manager. Values are data and are never sourced."]
        category = None
        for key, spec in self.properties.items():
            if spec["category"] != category:
                category = spec["category"]
                rows.extend(["", f"# {category.replace('-', ' ').title()}"])
            value = values.get(key, spec["default"])
            if spec["type"] == "boolean":
                value = "true" if value is True else "false" if value is False else value
            self._parse_value(key, str(value), len(rows) + 1)
            rows.append(f"{key}={value}")
        return "\n".join(rows) + "\n"


def content_hash(data: bytes | str) -> str:
    if isinstance(data, str):
        data = data.encode()
    return hashlib.sha256(data).hexdigest()


def update_website_settings(model: ConfigModel, text: str, port: int, public_url: str) -> str:
    values = model.parse(text)
    values["STATUS_PORT"] = port
    values["PUBLIC_BASE_URL"] = public_url.strip()
    return model.render(values)


def website_configuration_ready(values: dict[str, object]) -> bool:
    """Return true only when saved networking values are no longer local placeholders."""
    try:
        port = int(values["STATUS_PORT"])
        host = (urlsplit(str(values["PUBLIC_BASE_URL"])).hostname or "").lower()
    except (KeyError, TypeError, ValueError):
        return False
    return 1 <= port <= 65535 and host not in {"", "localhost", "127.0.0.1", "::1"}
