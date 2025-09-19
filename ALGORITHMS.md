# Algorithm Implementation Guide

This document explains the core algorithms implemented in the Paibot Drawing Tool, providing both theoretical background and implementation details.

## Gradient Paint Bucket

### Flood Fill Algorithm
The gradient bucket uses a modified flood-fill algorithm to identify enclosed areas:

1. **Seed Point Selection**: User clicks to provide initial point
2. **Grid Rasterization**: Level geometry is rasterized to a binary grid
3. **Boundary Detection**: Use 4-connected flood fill to find enclosed region
4. **Marching Squares**: Extract polygon boundary from rasterized region
5. **Douglas-Peucker**: Simplify polygon to reduce vertex count

```cpp
void performFloodFill(CCPoint seedPoint) {
    // 1. Initialize visited grid
    std::vector<std::vector<bool>> visited(gridHeight, 
        std::vector<bool>(gridWidth, false));
    
    // 2. Flood fill with boundary detection
    std::queue<CCPoint> queue;
    queue.push(seedPoint);
    
    while (!queue.empty()) {
        auto current = queue.front();
        queue.pop();
        
        // Check 4-connected neighbors
        for (auto& direction : {CCPoint{1,0}, {-1,0}, {0,1}, {0,-1}}) {
            auto neighbor = current + direction;
            if (isValidAndNotVisited(neighbor) && !hasGeometry(neighbor)) {
                visited[neighbor.y][neighbor.x] = true;
                queue.push(neighbor);
            }
        }
    }
}
```

### Gradient Generation
Three gradient types are supported with different mathematical approaches:

#### Linear Gradients
- **Direction Vector**: Calculated from start→end points
- **Projection**: Each point projected onto gradient line
- **Band Generation**: Create parallel strips perpendicular to direction

```cpp
float calculateGradientPosition(CCPoint point, CCPoint start, CCPoint end) {
    auto direction = normalize(end - start);
    auto offset = point - start;
    return dot(offset, direction) / length(end - start);
}
```

#### Radial Gradients  
- **Distance Calculation**: Euclidean distance from center
- **Ring Generation**: Concentric circles with varying radii
- **Normalization**: Distance normalized by maximum radius

```cpp
float calculateRadialPosition(CCPoint point, CCPoint center, float maxRadius) {
    return length(point - center) / maxRadius;
}
```

#### Angular Gradients
- **Angle Calculation**: atan2 from center to point
- **Sector Generation**: Pie slice regions based on angle ranges
- **Wraparound**: Handle 2π wraparound for seamless transition

### Color Interpolation
HSV color space provides more natural gradients than RGB:

```cpp
ccColor3B interpolateHSV(ccColor3B color1, ccColor3B color2, float t) {
    auto hsv1 = rgbToHsv(color1);
    auto hsv2 = rgbToHsv(color2);
    
    // Handle hue wraparound (shortest path)
    float hueDiff = hsv2.h - hsv1.h;
    if (hueDiff > 180) hueDiff -= 360;
    if (hueDiff < -180) hueDiff += 360;
    
    HSV result = {
        hsv1.h + t * hueDiff,
        hsv1.s + t * (hsv2.s - hsv1.s),
        hsv1.v + t * (hsv2.v - hsv1.v)
    };
    
    return hsvToRgb(result);
}
```

## Structure Optimizer

### Geometric Merging Pipeline
The optimizer follows a multi-stage pipeline for maximum reduction:

1. **Normalization**: Standardize transforms, colors, scales
2. **Geometric Merging**: Combine compatible geometric elements
3. **Pattern Recognition**: Identify and instance repeated patterns
4. **Polygonization**: Convert small objects to polygon representations
5. **Trigger Coalescing**: Merge compatible trigger objects
6. **Validation**: Ensure visual fidelity within ΔE threshold

### Line Merging Algorithm
Combines collinear and adjacent line segments:

```cpp
std::vector<LineSegment> mergeLines(const std::vector<LineSegment>& lines) {
    std::vector<LineSegment> merged;
    
    for (const auto& line : lines) {
        bool wasMerged = false;
        
        for (auto& existing : merged) {
            if (areCollinear(line, existing) && 
                colorsMatch(line, existing) &&
                canMerge(line, existing)) {
                
                // Extend existing line to encompass both
                existing = extendLine(existing, line);
                wasMerged = true;
                break;
            }
        }
        
        if (!wasMerged) {
            merged.push_back(line);
        }
    }
    
    return merged;
}
```

### Pattern Recognition
Uses spatial hashing and transform comparison:

```cpp
struct Pattern {
    std::vector<GameObject*> objects;
    CCPoint centroid;
    float rotation;
    CCPoint scale;
    int frequency;
};

std::vector<Pattern> findPatterns(const std::vector<GameObject*>& objects) {
    // 1. Group objects by spatial proximity
    auto spatialGroups = spatialHash(objects, gridSize);
    
    // 2. For each group, find transform-similar clusters
    std::vector<Pattern> patterns;
    
    for (const auto& group : spatialGroups) {
        auto clusters = clusterByTransform(group);
        
        for (const auto& cluster : clusters) {
            if (cluster.size() >= minPatternSize) {
                patterns.push_back(createPattern(cluster));
            }
        }
    }
    
    return patterns;
}
```

### Delta E Validation
Ensures visual fidelity using perceptual color difference:

```cpp
float calculateDeltaE(ccColor3B color1, ccColor3B color2) {
    // Convert to LAB color space for perceptual accuracy
    auto lab1 = rgbToLab(color1);
    auto lab2 = rgbToLab(color2);
    
    // Delta E CIE76 formula
    float deltaL = lab1.L - lab2.L;
    float deltaA = lab1.a - lab2.a;
    float deltaB = lab1.b - lab2.b;
    
    return sqrt(deltaL*deltaL + deltaA*deltaA + deltaB*deltaB);
}
```

## Seamless Background Generator

### Seamless From Image Algorithm
Converts arbitrary images to tileable textures:

1. **Offset Method**: Split image and swap quadrants
2. **Seam Detection**: Identify discontinuities at new edges
3. **Poisson Blending**: Seamlessly blend seam regions
4. **Edge Enhancement**: Apply Hann window for smooth transitions

```cpp
CCImage* makeSeamless(CCImage* source) {
    int width = source->getWidth();
    int height = source->getHeight();
    
    // 1. Create offset version (swap quadrants)
    auto offset = createOffsetImage(source, width/2, height/2);
    
    // 2. Identify seam regions (cross pattern)
    auto seamMask = createCrossMask(width, height, seamWidth);
    
    // 3. Apply Poisson blending at seams
    auto blended = poissonBlend(source, offset, seamMask);
    
    // 4. Apply Hann window for edge smoothing
    return applyHannWindow(blended);
}
```

### Texture Synthesis (Efros-Leung)
Generates new texture content based on sample:

```cpp
CCImage* efrosLeungSynthesis(CCImage* sample, int outputSize) {
    CCImage* output = createEmptyImage(outputSize, outputSize);
    
    // Start with random seed pixels
    seedRandomPixels(output, sample);
    
    while (!allPixelsFilled(output)) {
        auto unfilled = getUnfilledPixels(output);
        
        for (const auto& pixel : unfilled) {
            // Find best matching neighborhood in sample
            auto neighborhood = getNeighborhood(output, pixel);
            auto bestMatch = findBestMatch(sample, neighborhood);
            
            if (bestMatch.isValid()) {
                setPixel(output, pixel, bestMatch.color);
            }
        }
    }
    
    return output;
}
```

### Procedural Noise Generation
Implements tileable Perlin noise:

```cpp
float tileablePerlinNoise(float x, float y, float tileSize) {
    // Use tileable gradient vectors
    auto p00 = gradientAt(floor(x), floor(y));
    auto p01 = gradientAt(floor(x), floor(y) + tileSize);
    auto p10 = gradientAt(floor(x) + tileSize, floor(y));
    auto p11 = gradientAt(floor(x) + tileSize, floor(y) + tileSize);
    
    // Bilinear interpolation with smooth step
    float fx = smoothstep(x - floor(x));
    float fy = smoothstep(y - floor(y));
    
    float n0 = lerp(dotProduct(p00, x, y), dotProduct(p10, x, y), fx);
    float n1 = lerp(dotProduct(p01, x, y), dotProduct(p11, x, y), fx);
    
    return lerp(n0, n1, fy);
}
```

### Wang Tiles Generation
Creates edge-compatible tile sets:

```cpp
struct WangTile {
    CCImage* image;
    EdgePattern top, right, bottom, left;
};

std::vector<WangTile> generateWangTiles(int count) {
    // Generate edge patterns (colors/textures)
    auto edgePatterns = generateEdgePatterns(sqrt(count));
    
    std::vector<WangTile> tiles;
    
    for (int i = 0; i < count; ++i) {
        WangTile tile;
        
        // Assign edge patterns ensuring some compatibility
        tile.top = edgePatterns[i % edgePatterns.size()];
        tile.right = edgePatterns[(i + 1) % edgePatterns.size()];
        tile.bottom = edgePatterns[(i + 2) % edgePatterns.size()];
        tile.left = edgePatterns[(i + 3) % edgePatterns.size()];
        
        // Generate tile content respecting edge constraints
        tile.image = generateTileContent(tile.top, tile.right, 
                                       tile.bottom, tile.left);
        
        tiles.push_back(tile);
    }
    
    return tiles;
}
```

## Performance Optimizations

### Spatial Indexing
Use quadtrees for efficient spatial queries:

```cpp
class QuadTree {
    struct Node {
        CCRect bounds;
        std::vector<GameObject*> objects;
        std::array<std::unique_ptr<Node>, 4> children;
        bool isLeaf = true;
    };
    
    void insert(GameObject* obj) {
        insertRecursive(root.get(), obj);
    }
    
    std::vector<GameObject*> query(CCRect region) {
        std::vector<GameObject*> result;
        queryRecursive(root.get(), region, result);
        return result;
    }
};
```

### Memory Management
Use object pooling for frequent allocations:

```cpp
template<typename T>
class ObjectPool {
    std::vector<std::unique_ptr<T>> pool;
    std::queue<T*> available;
    
public:
    T* acquire() {
        if (available.empty()) {
            pool.push_back(std::make_unique<T>());
            return pool.back().get();
        }
        
        T* obj = available.front();
        available.pop();
        return obj;
    }
    
    void release(T* obj) {
        obj->reset();
        available.push(obj);
    }
};
```

### Parallel Processing
Use thread pools for CPU-intensive operations:

```cpp
class ThreadPool {
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop = false;
    
public:
    template<typename F>
    auto enqueue(F&& f) -> std::future<decltype(f())> {
        auto task = std::make_shared<std::packaged_task<decltype(f())()>>(
            std::forward<F>(f)
        );
        
        auto result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            tasks.emplace([task]() { (*task)(); });
        }
        
        condition.notify_one();
        return result;
    }
};
```

This implementation guide provides the foundation for understanding and extending the Paibot Drawing Tool's algorithms.