"""Load and search command documentation markdown files."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable


@dataclass(slots=True)
class CommandDocument:
    name: str
    content: str
    path: Path

    def summary(self) -> str:
        """Return a concise summary (first paragraph) of the document."""
        sections = [section.strip() for section in self.content.split('\n\n') if section.strip()]
        return sections[0] if sections else self.content.strip()


class CommandDocumentation:
    """Provides access to command documentation stored as markdown files."""

    def __init__(self, docs_directory: Path | None = None) -> None:
        self._docs_directory = docs_directory or Path("commands")
        self._documents: Dict[str, CommandDocument] = {}
        self._load_documents()

    def _load_documents(self) -> None:
        if not self._docs_directory.exists():
            return
        for md_file in self._docs_directory.glob("*.md"):
            content = md_file.read_text(encoding="utf-8")
            key = md_file.stem.lower()
            self._documents[key] = CommandDocument(name=key, content=content, path=md_file)

    def refresh(self) -> None:
        """Reload documents from disk."""
        self._documents.clear()
        self._load_documents()

    def available_commands(self) -> Iterable[str]:
        return self._documents.keys()

    def get(self, command_name: str) -> CommandDocument | None:
        return self._documents.get(command_name.lower())

    def find_best_match(self, query: str) -> CommandDocument | None:
        normalized = query.lower()
        if normalized in self._documents:
            return self._documents[normalized]
        for name, document in self._documents.items():
            if name in normalized or normalized in name:
                return document
        return None


__all__ = ["CommandDocument", "CommandDocumentation"]
