# Azur: Lephe's game engine

Azur is a game engine for the fx-CG series of graphing calculators that also supports standard PC (SDL2 / OpenGL) and web (emscripten / SDL2 / WebGL) builds.

Azur aims to implement real-world game engine mechanics such as ECS, animation and visual effects, physics, and state serialization. It doesn't try to be super fancy with them, but it does try to get the structure right.

## Projects in this repository

The following are Azur libraries:

* **libnum** is a C++ fixed-point math library. It provides various sizes of fixed-point values with support for arithmetic, but also some math functions, geometry/linear algebra, and is planned to support numerical algorithms in the future.
* **libazrp** is C library that implements Azur's Rendering Pipeline. On fx-CG, Azur uses a very unique rendering method that sacrifices some generality for a significant boost in performance, and this system has uses beyond games. (TODO: Currently still in the `azur/` folder)
* **azur** is of course the engine itself.

The following external projects are Azur by some of Azur's features, and maintained as submodules in `3rdparty/`:

* [**gl3w**](https://github.com/skaslev/gl3w) is a minimalist OpenGL loader, which is used in Azur's OpenGL backend to load the core profile.
* [**GLM**](https://glm.g-truc.net/0.9.9/index.html) is math library for graphics, which is interoperable with OpenGL shaders. In the future I hope to use libnum everywhere, but GLM is still useful in OpenGL-related code.
* [**Dear ImGui**](https://github.com/ocornut/imgui) is a GUI library that integrates fantastically with OpenGL. In the future, I plan to provide debugging and introspection for Azur's OpenGL backend using ImGui.

## Building

**Building for fx-CG**

Use the [fxSDK toolchain](https://gitea.planet-casio.com/Lephenixnoir/fxsdk).

```
% fxsdk build-cg
```

**Building for Linux**

```
% cmake -B build-linux -DAZUR_PLATFORM=linux -DCMAKE_INSTALL_PREFIX="$HOME/.local"
% make -C build-linux
```

**Building for emscripten**

You will need the [emscripten toolchain](https://emscripten.org/).

```
% emcmake cmake -B build-emscripten -D AZUR_PLATFORM=emscripten
% make -C build-emscripten
```

## History note

Back in 2021 I made a single repository for this engine, along with other programs that I intended to write with it. At the time I didn't intend to distribute the engine in any serious fashion, but that changed after people expressed interest in playing with [After Burner](https://www.planet-casio.com/Fr/programmes/programme4238-1-after-burner-lephenixnoir-jeux-add-ins.html). I split the repository in May 2022 with `git filter-branch`, and edited the history so old commits can still be built, but don't expect that to work out-of-the-box.
