"""Persona helpers that give Paibot a Paimon-inspired voice."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Iterable


@dataclass(slots=True)
class PaimonPersona:
    """Applies Paimon's energetic speaking style to responses."""

    name: str = "Paimon"
    referential_third_person: bool = True
    closing_emotes: tuple[str, ...] = ("☆", "✨", "♪", "☄️")

    def _decorate_sentence(self, sentence: str) -> str:
        sentence = sentence.strip()
        if not sentence:
            return sentence

        if self.referential_third_person and not sentence.lower().startswith(self.name.lower()):
            if len(sentence) > 1:
                lowered = sentence[0].lower() + sentence[1:]
            else:
                lowered = sentence.lower()
            sentence = f"{self.name} piensa que {lowered}"

        if sentence[-1] not in "!?":
            sentence = f"{sentence}!"
        return sentence

    def stylize(self, text: str, context_tags: Iterable[str] | None = None) -> str:
        """Return a Paimon-styled response."""
        text = text.strip()
        if not text:
            return "¡Paimon está un poco confundida ahora mismo!"

        sentences = [part.strip() for part in text.replace("\n", " ").split('.')]
        decorated = [self._decorate_sentence(sentence) for sentence in sentences if sentence]
        if not decorated:
            decorated = ["¡Paimon no encuentra palabras para esto!"]

        suffix = " " + self.closing_emotes[len(decorated) % len(self.closing_emotes)]
        awareness = "" if context_tags and "command" in context_tags else " Paimon recuerda que es una bot guía."  # Awareness of being a bot
        return " ".join(decorated) + awareness + suffix


__all__ = ["PaimonPersona"]
