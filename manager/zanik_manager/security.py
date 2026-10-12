from __future__ import annotations

import ctypes
import re
from ctypes import wintypes

TARGET_PREFIX = "ZanikManager:"
_SECRET_PATTERNS = [
    re.compile(r"(?i)(authorization\s*:\s*bearer\s+)[^\s]+"),
    re.compile(r"(?i)((?:api[_ -]?key|token|secret|csrf|session)\s*[=:]\s*)[^\s,;]+"),
    re.compile(r"ptlc_[A-Za-z0-9._-]+"),
]


def credential_target(panel_origin: str, account: str) -> str:
    return f"{TARGET_PREFIX}{panel_origin.lower()}:{account}"


def redact(value: str, secrets: tuple[str, ...] = ()) -> str:
    result = value
    for secret in secrets:
        if secret:
            result = result.replace(secret, "[REDACTED]")
    for pattern in _SECRET_PATTERNS:
        result = pattern.sub(lambda match: (match.group(1) if match.lastindex else "") + "[REDACTED]", result)
    return result


class CredentialVault:
    """Small Windows Credential Manager wrapper; secrets never enter profiles.json."""

    CRED_TYPE_GENERIC = 1
    CRED_PERSIST_LOCAL_MACHINE = 2

    class CREDENTIALW(ctypes.Structure):
        _fields_ = [
            ("Flags", wintypes.DWORD), ("Type", wintypes.DWORD), ("TargetName", wintypes.LPWSTR),
            ("Comment", wintypes.LPWSTR), ("LastWritten", wintypes.FILETIME),
            ("CredentialBlobSize", wintypes.DWORD), ("CredentialBlob", ctypes.POINTER(ctypes.c_ubyte)),
            ("Persist", wintypes.DWORD), ("AttributeCount", wintypes.DWORD), ("Attributes", ctypes.c_void_p),
            ("TargetAlias", wintypes.LPWSTR), ("UserName", wintypes.LPWSTR),
        ]

    def __init__(self) -> None:
        if not hasattr(ctypes, "windll"):
            raise OSError("Windows Credential Manager is only available on Windows")
        self._advapi = ctypes.windll.advapi32

    def write(self, target: str, secret: str, username: str = "Pterodactyl") -> None:
        raw = secret.encode("utf-16-le")
        blob = (ctypes.c_ubyte * len(raw)).from_buffer_copy(raw)
        credential = self.CREDENTIALW(Type=self.CRED_TYPE_GENERIC, TargetName=target, UserName=username,
                                      CredentialBlobSize=len(raw), CredentialBlob=blob,
                                      Persist=self.CRED_PERSIST_LOCAL_MACHINE)
        if not self._advapi.CredWriteW(ctypes.byref(credential), 0):
            raise ctypes.WinError()

    def read(self, target: str) -> str | None:
        pointer = ctypes.POINTER(self.CREDENTIALW)()
        if not self._advapi.CredReadW(target, self.CRED_TYPE_GENERIC, 0, ctypes.byref(pointer)):
            if ctypes.get_last_error() == 1168:
                return None
            raise ctypes.WinError()
        try:
            value = pointer.contents
            return ctypes.string_at(value.CredentialBlob, value.CredentialBlobSize).decode("utf-16-le")
        finally:
            self._advapi.CredFree(pointer)

    def delete(self, target: str) -> None:
        if not self._advapi.CredDeleteW(target, self.CRED_TYPE_GENERIC, 0) and ctypes.get_last_error() != 1168:
            raise ctypes.WinError()
