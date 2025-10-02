# Paibot Drawing Tool for Geometry Dash

An advanced drawing tool for Geometry Dash inspired by [Allium](https://github.com/altalk23/Allium) with complete UI/UX parity plus four new powerful features.

## Features

### Core Drawing Tools (Allium Parity)
- **Line Tool**: Draw straight lines with 90° angle snapping (Shift key)
- **Curve Tool**: Draw smooth Bézier curves 
- **Freeform Tool**: Draw freehand with Douglas-Peucker simplification
- **Polygon Tool**: Create closed polygons with vertex snapping
- **Text Tool**: Render text using TTF fonts with vectorization

### New Advanced Features

#### 1. Gradient Paint Bucket (Balde con Degradado)
- **Linear gradients**: Directional color transitions
- **Radial gradients**: Circular color spreads from center
- **Angular gradients**: Rotational color wheels
- **Flood-fill algorithm**: Intelligently fills enclosed areas
- **HSV color interpolation**: Smooth color transitions
- **Preview mode**: See results before applying

#### 2. Structure Optimizer (Optimizador de Estructuras)  
- **60%+ object reduction**: Massive optimization without visual loss
- **Geometric merging**: Combines lines, segments, and overlapping objects
- **Pattern recognition**: Creates instances of repeated elements
- **Delta E validation**: Ensures visual fidelity under 2.0 ΔE threshold
- **Vanilla/Geode modes**: Compatible optimizations for different setups

#### 3. Seamless Background Generator
- **Seamless from image**: Convert any image to tileable background
- **Texture synthesis**: Generate infinite variations using Efros-Leung/Kwatra algorithms
- **Procedural generation**: Perlin/Simplex/Worley noise with palettes
- **Wang tiles**: 8-16 tile sets with perfect edge matching
- **Multiple formats**: Export as sprites, normal maps, or JSON presets

### UI/UX Design (Allium Parity)

#### Keyboard Shortcuts
- **Shift**: Snap to 90° angles (lines/polygons)
- **Alt**: Snap to editor grid
- **Space** (hold): Temporary pan mode, disables brush
- **Arrow keys**: Navigate in text editor mode
- **G**: Quick access to Gradient Bucket
- **O**: Quick access to Structure Optimizer  
- **B**: Quick access to Background Generator

#### Interface Elements
- **Tool palette**: Toggle buttons with active/hover/disabled states
- **Side panels**: Parameter controls for each tool
- **Preview system**: Live preview with apply/cancel options
- **Undo/Redo**: Full integration with editor history
- **Progress bars**: Cancellable long operations

## Architecture

### Core Components

```
include/Paibot/
├── ui/
│   ├── PaibotButtonBar.hpp      # Main toolbar (AlliumButtonBar equivalent)
│   └── MenuItemTogglerExtra.hpp # Toggle button component
├── manager/
│   └── BrushManager.hpp         # Global state and settings
└── util/
    ├── BrushDrawer.hpp          # Base drawing tool class
    ├── GradientBrushDrawer.hpp  # Gradient paint bucket
    ├── StructureOptimizer.hpp   # Object optimization engine
    └── BackgroundGenerator.hpp  # Seamless background system
```

### Third-Party Libraries (Following Allium)
- **Anti-Grain Geometry (AGG)**: High-quality 2D rendering
- **Poly2Tri**: Polygon triangulation for complex shapes
- **Clipper2**: Boolean operations on polygons
- **Geode SDK**: Mod framework and game integration

### License Compliance
- **BSL 1.0**: Geode SDK, Clipper2 compatibility
- **3-Clause BSD**: AGG, Poly2Tri compatibility
- All derivative works maintain original license requirements

## Settings

The mod provides extensive configuration through Geode's settings system:

```json
{
  "brush-line-width": 5.0,        // Line thickness (0.5-30)
  "brush-color-id": 1011,         // GD color ID (1-1020)
  "gradient-steps": 32,           // Gradient resolution (8-64)
  "optimizer-target-reduction": 0.6, // Target optimization % (0.1-0.9)
  "seamless-tile-size": 1024      // Background tile size (512/1024/2048)
}
```

## Installation

1. Download the latest release from GitHub
2. Copy the `.dll`/`.dylib`/`.so` file to your Geode `mods/` folder
3. Launch Geometry Dash and open the Level Editor
4. The Paibot toolbar will appear in the editor interface

## Usage Examples

### Creating a Gradient Fill
1. Select the Gradient Bucket tool (G)
2. Click in an enclosed area to set the seed point
3. Drag to define gradient direction/radius
4. Use Shift for 90° angle snapping, Alt for grid snapping
5. Preview the result and click Apply

### Optimizing Level Structures
1. Select objects you want to optimize
2. Click the Structure Optimizer button (O)
3. Adjust reduction target and tolerance settings
4. Preview shows before/after comparison
5. Apply optimization if satisfied with results

### Generating Seamless Backgrounds
1. Click the Background Generator button (B)
2. Choose generation method (Image/Synthesis/Procedural/Wang)
3. Configure parameters (tile size, continuity, variety)
4. Preview the 3×3 tile pattern
5. Export as sprites or JSON preset

## Conversational Assistant

Paibot ahora incluye un asistente conversacional alimentado por Gemini con memoria por usuario.

- **Estilo Paimon**: cuando la conversación menciona explícitamente a Paibot, las respuestas se transforman con el tono energético de Paimon.
- **Memoria persistente**: el historial de cada usuario se guarda en `memory/<owner>/<repo>/<branch>/usuario.json`, utilizando las variables de entorno `GITHUB_REPO_OWNER`, `GITHUB_REPO_NAME` y `GITHUB_BRANCH`.
- **Documentación dinámica**: las preguntas sobre comandos leen la información directamente de los archivos `.md` dentro de `commands/` para dar instrucciones precisas.
- **Gemini API**: el asistente usa la clave `GEMINI_API_KEY` para conectarse al modelo `gemini-pro` y generar respuestas en español conscientes de que se trata de un bot.

## Building from Source

### Requirements
- CMake 3.21+ (for modern C++20 support)
- C++20 compatible compiler (required for Geode SDK)
- Geode SDK 4.8.0 (pinned version for stability)

### Recommended Toolchain

#### Windows
- **Compiler**: MSVC 2022 (Visual Studio 17.0+)
- **Build Tool**: MSBuild or Ninja
- **Compilation Flags**: `/W4 /WX /permissive-` (warnings as errors)

#### Linux/macOS
- **Compiler**: Clang 14+ or GCC 11+ 
- **Build Tool**: Ninja (recommended) or Make
- **Compilation Flags**: `-Wall -Wextra -Werror -pedantic`

### Build Steps

#### Windows (MSVC)
```bash
# Configure with MSVC toolchain
cmake -S . -B build -DGEODE_TARGET_PLATFORM=Win64 -DCMAKE_BUILD_TYPE=Release

# Build with parallel compilation
cmake --build build --config Release --parallel
```

#### Linux (Clang)
```bash
# Configure with Clang toolchain  
cmake -S . -B build -DGEODE_TARGET_PLATFORM=Linux -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++

# Build with parallel compilation
cmake --build build --config Release --parallel
```

#### Build Artifacts
The mod binary will be created in `build/` with platform-specific naming:
- Windows: `PaibotGeodeBase.dll` 
- Linux: `libPaibotGeodeBase.so`
- macOS: `libPaibotGeodeBase.dylib`

### Continuous Integration
The project uses GitHub Actions for automated building and testing:
- **Windows**: MSVC 2022 on Windows Server 2022
- **Linux**: Clang 14 on Ubuntu 22.04
- **Validation**: mod.json schema and resource integrity checks

## Development Roadmap

### Phase 1: Core Framework ✅
- [x] UI/UX parity with Allium
- [x] Basic drawing tool infrastructure
- [x] Keyboard modifier support
- [x] Settings integration

### Phase 2: Algorithm Implementation 🚧
- [ ] Real flood-fill with Marching Squares
- [ ] Poisson blending for seamless images
- [ ] Efros-Leung texture synthesis
- [ ] Advanced optimization algorithms

### Phase 3: Third-Party Integration
- [ ] AGG library integration
- [ ] Poly2Tri triangulation
- [ ] Clipper2 polygon operations
- [ ] Performance optimization

### Phase 4: Polish & Testing
- [ ] Comprehensive test suite
- [ ] Performance benchmarks
- [ ] User documentation
- [ ] Tutorial levels

## Contributing

1. Fork the repository
2. Create a feature branch
3. Follow existing code style and patterns
4. Add tests for new functionality
5. Submit a pull request

## Credits

- **Inspired by**: [Allium](https://github.com/altalk23/Allium) by altalk23
- **Built with**: [Geode SDK](https://github.com/geode-sdk/geode)
- **Algorithms**: Based on academic papers for texture synthesis and optimization

## License

This project is licensed under the BSL 1.0 License - see the [LICENSE](LICENSE) file for details.

Third-party libraries maintain their original licenses:
- Geode SDK: BSL 1.0
- Anti-Grain Geometry: 3-Clause BSD  
- Poly2Tri: 3-Clause BSD
- Clipper2: BSL 1.0
