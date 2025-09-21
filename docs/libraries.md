---
title: Libraries bundled with Azur
label: @libraries
template: article.html
---

# Libraries bundled with Azur

Azur relies on a number of third-party libraries at compile-time or runtime, and most of them are also exposed to applications to share code.

[TOC]

## List of libraries used in Azur

#table(rowsep: "\\\\", colsep: "&", head: "row"):
  Library & License & Description \\
  [gl3w](https://github.com/skaslev/gl3w) & Public domain & OpenGL core profile loader \\
  [glm](https://github.com/g-truc/glm) & MIT & OpenGL Mathematics \\
  [Dear ImGui](https://github.com/ocornut/imgui) & MIT & Awesome immediate-mode GUI framework \\
  [stb libraries](https://github.com/nothings/stb) & Public domain & Single-header general-purpose libraries \\

gl3w is used internally to load an OpenGL core profile.
Applications shouldn't need to access it as its only job is performed when initializing Azur.

## Using glm

Simply include the header file and be happy!

```c
#include <glm/glm.hpp>
glm::vec2 pos;
```

TODO: Azur has a numerical library called libnum whose main purpose is to provide fixed-point implementation of operations and algorithms for targets without an FPU.
libnum overlaps with glm significantly and might replace it in Azur in the future.

## Using Dear Imgui

[Dear Imgui](https://github.com/ocornut/imgui) is available for debugging tools or simply making standard GUI apps.
It's very much intended to be used directly; in fact, Azur doesn't wrap it and mostly offers integration and some API extensions with new functions.

Initialize Dear Imgui with `azur_init_imgui`.
It will be shut down properly when calling `azur_quit`.

```c++
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

if(azur_init("MQ", 1500, 850) != 0)
    return 1;
if(!azur_init_imgui())
    return 1;
```

TODO: Adjust return value convention of `azur_init`.

Render Dear Imgui stuff on the screen as usual.

```c++
ImGui_ImplOpenGL3_NewFrame();
ImGui_ImplSDL2_NewFrame();
ImGui::NewFrame();
// ...
ImGui::Render();
ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
```

## Using stb libraries

The following [stb](https://github.com/nothings/stb) libraries are bundled in Azur:

* [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h) (with JPG, PNG, BMP, and GIF)
* [stb_image_write](https://github.com/nothings/stb/blob/master/stb_image_write.h)

There are normally higher-level APIs for the associated tasks, through e.g. `azur::gl::Surface` or other abstractions.
If you want to use them directly for any reason, just include the header; Azur provides the implementation.

```c
#include <azur/stb_image.h>

int width, height, n;
u8 *data = stbi_load("image.png", &width, &height, &n, 0);
```

The headers are only accessible through `azur/` so that applications can still include stb libraries themselves (maybe with different configurations) without clashing.
The implementations in Azur are not namespaced but come from the library so they won't be pulled if another implementation is already available in the project's object files.

TODO: This means applications could override library code with user code, which isn't great.
