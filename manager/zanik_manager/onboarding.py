from __future__ import annotations

from dataclasses import dataclass
from typing import Callable

from .profiles import is_local_origin, normalize_origin
from .pterodactyl import PterodactylClient


def is_remote_http(origin: str) -> bool:
    normalized = normalize_origin(origin)
    return normalized.startswith("http://") and not is_local_origin(normalized)


@dataclass(frozen=True)
class PanelDiscovery:
    origin: str
    account: dict
    servers: list[dict]
    allow_insecure_http: bool


def discover_panel(origin: str, token: str, confirm_http: Callable[[], bool],
                   client_factory: Callable[[str, str], PterodactylClient] = PterodactylClient
                   ) -> PanelDiscovery | None:
    normalized = normalize_origin(origin)
    insecure = is_remote_http(normalized)
    if insecure and not confirm_http():
        return None
    client = client_factory(normalized, token)
    return PanelDiscovery(normalized, client.account(), client.servers(), insecure)
