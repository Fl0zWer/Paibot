#!/usr/bin/env python3
"""CLI para conversar con Paibot utilizando memoria persistente."""

from __future__ import annotations

import argparse
import sys

from bot import PaibotChat


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Inicia una sesión de chat con Paibot")
    parser.add_argument("usuario", help="Identificador del usuario para la memoria persistente")
    parser.add_argument(
        "--modelo",
        default="gemini-pro",
        help="Nombre del modelo de Gemini a utilizar (por defecto gemini-pro)",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    chat = PaibotChat(model_name=args.modelo)

    print("Iniciando conversación con Paibot. Escribe 'salir' para terminar.")
    while True:
        try:
            mensaje = input("Tú: ").strip()
        except (KeyboardInterrupt, EOFError):
            print("\nCerrando sesión. ¡Hasta luego!")
            break

        if mensaje.lower() in {"salir", "exit", "quit"}:
            print("Cerrando sesión. ¡Hasta luego!")
            break

        respuesta = chat.respond(args.usuario, mensaje)
        print(f"Paibot: {respuesta}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
