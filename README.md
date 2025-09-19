# Base para mods de Geode

Esta es una plantilla mínima inspirada en [Allium](https://github.com/altalk23/Allium) para comenzar un mod de [Geode](https://github.com/geode-sdk) escrito en C++. Incluye una modificación simple de `MenuLayer` que muestra un texto en el menú principal para comprobar que el hook funciona y un ajuste configurable para activarlo o desactivarlo.

## Estructura del proyecto

```
.
├── CMakeLists.txt        # Configuración de CMake con descarga opcional del SDK
├── mod.json              # Metadatos del mod y configuración
├── src/
│   └── main.cpp          # Punto de entrada del mod
└── .gitignore
```

## Requisitos

- [CMake](https://cmake.org/) 3.21 o superior
- Compilador C++ con soporte para C++20
- El SDK de Geode (`geode-sdk`). El script de CMake lo descargará automáticamente a `.geode-sdk/` si no existe.

Si prefieres usar una copia local del SDK, invoca CMake con `-DGEODE_SDK_DIR=/ruta/a/geode-sdk`.

## Compilación

```bash
cmake -S . -B build
cmake --build build --config Release
```

El binario resultante se ubicará en `build/` con el nombre correspondiente a cada plataforma. Copia el archivo y `mod.json` dentro de la carpeta `mods/` de Geode para probarlo.

## Personalización

- Cambia el identificador (`id`) y los metadatos en `mod.json` para tu proyecto.
- Añade nuevas clases modificadas en `src/` siguiendo la macro `$modify` de Geode.
- Usa `Mod::get()->getSettingValue<tipo>("clave")` para leer ajustes definidos en `mod.json`.

## Próximos pasos sugeridos

- Añadir más hooks (por ejemplo, a `PlayLayer` o `LevelEditorLayer`).
- Incorporar assets personalizados dentro de `resources/` y cargarlos desde el código.
- Configurar acciones en respuesta a eventos de Geode empleando `geode::NotificationCenter`.
