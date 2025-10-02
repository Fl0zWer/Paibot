"""User memory management for Paibot conversations."""

from __future__ import annotations

import json
import os
import re
from dataclasses import asdict, dataclass, field
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable, List

ISO_TIMESTAMP = "%Y-%m-%dT%H:%M:%S.%fZ"


@dataclass
class MemoryRecord:
    """Represents a single conversational memory record."""

    role: str
    content: str
    metadata: dict[str, str] | None = None
    timestamp: str = field(
        default_factory=lambda: datetime.now(timezone.utc).strftime(ISO_TIMESTAMP)
    )

    def to_json(self) -> dict[str, object]:
        """Serialize the record to a JSON-compatible dict."""
        payload = asdict(self)
        # Drop None metadata to keep files compact
        if payload.get("metadata") is None:
            payload.pop("metadata")
        return payload


class MemoryManager:
    """Stores conversational history per user scoped by repository metadata."""

    def __init__(self, base_directory: Path | None = None) -> None:
        repo_owner = os.environ.get("GITHUB_REPO_OWNER", "unknown-owner").strip() or "unknown-owner"
        repo_name = os.environ.get("GITHUB_REPO_NAME", "unknown-repo").strip() or "unknown-repo"
        branch = os.environ.get("GITHUB_BRANCH", "unknown-branch").strip() or "unknown-branch"

        if base_directory is None:
            base_directory = Path("memory") / repo_owner / repo_name / branch

        self._base_directory = base_directory
        self._base_directory.mkdir(parents=True, exist_ok=True)

    @property
    def base_directory(self) -> Path:
        """Return the directory root where memories are stored."""
        return self._base_directory

    def _sanitize_user_id(self, user_id: str) -> str:
        sanitized = re.sub(r"[^a-zA-Z0-9_.-]", "_", user_id)
        return sanitized or "anonymous"

    def _memory_file(self, user_id: str) -> Path:
        filename = f"{self._sanitize_user_id(user_id)}.json"
        return self._base_directory / filename

    def load_history(self, user_id: str) -> List[MemoryRecord]:
        """Load the stored history for a user."""
        path = self._memory_file(user_id)
        if not path.exists():
            return []

        with path.open("r", encoding="utf-8") as fp:
            payload = json.load(fp)

        history: List[MemoryRecord] = []
        for item in payload.get("history", []):
            history.append(
                MemoryRecord(
                    role=item.get("role", "assistant"),
                    content=item.get("content", ""),
                    metadata=item.get("metadata"),
                    timestamp=item.get("timestamp", datetime.now(timezone.utc).strftime(ISO_TIMESTAMP)),
                )
            )
        return history

    def save_history(self, user_id: str, history: Iterable[MemoryRecord]) -> None:
        """Persist the provided history for a user."""
        records = list(history)
        payload = {
            "user_id": user_id,
            "history": [record.to_json() for record in records],
            "updated_at": datetime.now(timezone.utc).strftime(ISO_TIMESTAMP),
        }
        path = self._memory_file(user_id)
        with path.open("w", encoding="utf-8") as fp:
            json.dump(payload, fp, ensure_ascii=False, indent=2)

    def append(self, user_id: str, record: MemoryRecord) -> None:
        """Append a single record to the user's history."""
        history = self.load_history(user_id)
        history.append(record)
        self.save_history(user_id, history)

    def extend(self, user_id: str, records: Iterable[MemoryRecord]) -> None:
        """Append multiple records to the user's history."""
        history = self.load_history(user_id)
        history.extend(records)
        self.save_history(user_id, history)
