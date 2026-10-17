from __future__ import annotations

from datetime import datetime


def format_activity_entry(message: str, timestamp: datetime | None = None) -> str:
    moment = timestamp or datetime.now().astimezone()
    prefix = f"[{moment.strftime('%Y-%m-%d %H:%M:%S')}] "
    lines = str(message).splitlines() or [""]
    return "\n".join(prefix + line for line in lines)
