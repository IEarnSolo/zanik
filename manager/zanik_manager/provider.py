from __future__ import annotations

from abc import ABC, abstractmethod


class ServerProvider(ABC):
    """Hosting-provider boundary used by the desktop manager."""

    @abstractmethod
    def list_servers(self) -> list[dict]: ...

    @abstractmethod
    def diagnose(self, server_id: str, server_root: str, deep: bool = False) -> dict: ...
