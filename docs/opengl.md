---
title: OpenGL wrappers and utilities
label: @opengl
template: article.html
---

# OpenGL wrappers and utilities

```c++
#include <azur/opengl.h>
```

This header provides low-level access to OpenGL as well as class-based wrappers.
The wrappers are intended to be used in conjunction with direct OpenGL access whenever needed.
Their primary purpose is to facilitate routine tasks like loading textures, compiling, shaders, and reporting errors.
They also make OpenGL state more explicit by attaching it to classes' API instead of leaving it hidden in the OpenGL context.

Whenever applicable, compatibility between OpenGL 3.3 and OpenGL ES 3.0 is provided, but this is rarely needed.

TODO

## Surfaces (CPU images)

TODO

## Textures (GPU images)

TODO

## Shaders

The class `Program` is a lightweight abstraction for OpenGL shaders.
It encapsulates all of a program's OpenGL state and automates typical tasks with error checking and (hopefully) good error reporting.
Instances of `Program` are expected to override methods and interact directly with OpenGL whenever the patterns supported by the base class are not sufficient.

### Handling buffers

A base program doesn't hold any buffer by default.
The only thing it needs is a structure description of vertex attributes for the Vertex Array Object (VAO).
Consider the two extreme cases:

- A program might have its own VAO, and fill a single VBO with dynamically-generated small meshes at every frame.
  Then it owns both the VAO and VBO.
- Or it may rely on a shared format precomputed on disk for evrery mesh; then it shares the VAO with other shaders and reads one VBO per mesh.

TODO: Runtime type reflection

### Shader includes

TODO
