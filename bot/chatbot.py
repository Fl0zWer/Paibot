"""Conversational Paibot powered by Gemini with per-user memory."""

from __future__ import annotations

import os
import re
from typing import Iterable, List

import google.generativeai as genai

from .command_docs import CommandDocument, CommandDocumentation
from .memory import MemoryManager, MemoryRecord
from .persona import PaimonPersona

COMMAND_PATTERN = re.compile(r"(?:!|/)?(?:comando|command|cmd)[:\s]+(?P<name>[\w-]+)", re.IGNORECASE)


class PaibotChat:
    """High-level chat interface wrapping Gemini with custom behaviour."""

    def __init__(
        self,
        *,
        memory_manager: MemoryManager | None = None,
        persona: PaimonPersona | None = None,
        command_docs: CommandDocumentation | None = None,
        model_name: str = "gemini-pro",
        model: genai.GenerativeModel | None = None,
        history_window: int = 12,
        mention_aliases: Iterable[str] | None = None,
    ) -> None:
        self.memory = memory_manager or MemoryManager()
        self.persona = persona or PaimonPersona()
        self.command_docs = command_docs or CommandDocumentation()
        self._history_window = max(history_window, 1)
        self._mention_aliases = tuple(alias.lower() for alias in (mention_aliases or ("paibot", "@paibot", "paimon")))

        self._system_instruction = (
            "Eres Paibot, un asistente del mod Paibot para Geometry Dash. "
            "Siempre recuerda que eres un bot amistoso y habla en español. "
            "Usa la memoria proporcionada para mantener el contexto y apóyate en la documentación "
            ".md cuando el usuario pregunte por comandos específicos."
        )
        self._generation_config = genai.types.GenerationConfig(
            temperature=0.75,
            top_p=0.9,
            top_k=40,
            max_output_tokens=512,
        )

        if model is not None:
            self._model = model
        else:
            api_key = os.environ.get("GEMINI_API_KEY")
            if not api_key:
                raise EnvironmentError(
                    "La variable de entorno GEMINI_API_KEY es necesaria para ejecutar Paibot con Gemini."
                )
            genai.configure(api_key=api_key)
            self._model = genai.GenerativeModel(
                model_name,
                system_instruction=self._system_instruction,
            )

    def respond(self, user_id: str, message: str) -> str:
        """Generate a response for the provided user message."""
        message = message.strip()
        if not message:
            return "Paimon no escuchó nada, ¡intenta decir algo de nuevo!"

        history = self.memory.load_history(user_id)
        mention = self._is_mention(message)
        command_document = self._resolve_command_query(message)

        if command_document:
            base_response = self._format_command_response(command_document)
        else:
            base_response = self._generate_model_reply(history, message, mention)

        final_response = self.persona.stylize(base_response) if mention else base_response

        updated_history = history + [
            MemoryRecord(role="user", content=message),
            MemoryRecord(role="assistant", content=final_response, metadata={"mention": str(mention).lower()}),
        ]
        trimmed_history = self._trim_history(updated_history)
        self.memory.save_history(user_id, trimmed_history)

        return final_response

    def _is_mention(self, message: str) -> bool:
        normalized = message.lower()
        return any(alias in normalized for alias in self._mention_aliases)

    def _resolve_command_query(self, message: str) -> CommandDocument | None:
        match = COMMAND_PATTERN.search(message)
        if match:
            candidate = match.group("name")
            document = self.command_docs.get(candidate)
            if document:
                return document
            return self.command_docs.find_best_match(candidate)

        normalized = message.lower()
        for name in self.command_docs.available_commands():
            if name in normalized:
                return self.command_docs.get(name)
        return None

    def _format_command_response(self, document: CommandDocument) -> str:
        summary = document.summary()
        return (
            f"El comando `{document.name}` funciona así:\n{summary}\n\n"
            f"Puedes revisar el archivo `{document.path.as_posix()}` para más ejemplos detallados."
        )

    def _generate_model_reply(self, history: List[MemoryRecord], message: str, mention: bool) -> str:
        contents: List[dict[str, object]] = []
        for record in self._recent_history(history):
            role = "user" if record.role == "user" else "model"
            contents.append({"role": role, "parts": [record.content]})

        if mention:
            contents.append(
                {
                    "role": "user",
                    "parts": [
                        "El usuario acaba de mencionar a Paibot. Responde con un tono alegre inspirado en Paimon, sin olvidar"
                        " que eres una bot."
                    ],
                }
            )

        contents.append({"role": "user", "parts": [message]})

        response = self._model.generate_content(
            contents,
            generation_config=self._generation_config,
        )
        text = getattr(response, "text", "")
        return text.strip() or "Paimon todavía está pensando en la respuesta."

    def _recent_history(self, history: List[MemoryRecord]) -> List[MemoryRecord]:
        if len(history) <= self._history_window:
            return history
        return history[-self._history_window :]

    def _trim_history(self, history: List[MemoryRecord]) -> List[MemoryRecord]:
        max_records = self._history_window * 2
        if len(history) <= max_records:
            return history
        return history[-max_records:]


__all__ = ["PaibotChat"]
