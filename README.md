# Azur: Lephe's game engine

Azur is a game engine for the fx-CG series of graphing calculators that also supports standard PC (SDL2 / OpenGL) and web (emscripten / SDL2 / WebGL) builds.

Azur aims to implement real-world game engine mechanics such as ECS, animation and visual effects, physics, and state serialization. It doesn't try to be super fancy with them, but it does try to get the structure right.

Current status:
- [x] Build system supporting gint (fxSDK), Linux (SDL 2/OpenGL 3.3) and emscripten (SDL 2/OpenGL ES 3.0)
- [x] Platform-independent initialization/main loop/etc management.
- [x] Specialized fragmented-rendering pipeline for gint, providing high-performance graphics.
- [ ] Bare-bones fixed-point computation library.
- [ ] Experimental ECS system under design.

Currently Azur is barely more than the native systems it uses (gint/OpenGL), with no solid unified APIs. It is mostly useful for writing high-performance gint games.

## Projects in this repository

The following are Azur libraries:

* **libnum** is a C++ fixed-point math library. It provides various sizes of fixed-point values with support for arithmetic, but also some math functions, geometry/linear algebra, and is planned to support numerical algorithms in the future.
* **libazrp** is C library that implements Azur's Rendering Pipeline. On fx-CG, Azur uses a very unique rendering method that sacrifices some generality for a significant boost in performance, and this system has uses beyond games. (TODO: Currently still in the `azur/` folder)
* **azur** is of course the engine itself.

A number of external projects are used by some of Azur's features, and maintained as submodules in `3rdparty/`. See Azur's docs for a full breakdown and licensing considerations.

## Building

Building Azur takes two steps: first we build and install the third-party dependencies, and then only we build and install Azur itself (relying on the previously-intalled dependencies).

You *must* install into a different folder for each platform, as the headers and libraries would otherwise override each other. For all non-native builds you should have or make a sysroot folder.

**Building for CASIO calculators**

Use the [fxSDK toolchain](https://gitea.planet-casio.com/Lephenixnoir/fxsdk). When building with the fxSDK, `AZUR_PLATFORM=gint` is assumed and the fxSDK sysroot is used to install the library.

```bash
fxsdk build-cg install
```

**Building for Linux**

Choose an install prefix such as `~/.local` and export it in the environment variable `AZUR_PATH_linux`, ideally globally like in your `.profile`, Docker config, etc.
The environment variable will be used in step 2 to find the dependencies installed in step 1.
More importantly, it will be used to find Azur when you configure your own projects.

```bash
# Set and export it globally (e.g. in ~/.profile)
export AZUR_PATH_linux="$HOME/.local"

# 1. Build and install dependencies
cmake -B build-linux-3rdparty -S 3rdparty \
  -DAZUR_PLATFORM=linux \
  -DCMAKE_INSTALL_PREFIX="$AZUR_PATH_linux"
make -C build-linux-3rdparty install -j$(nproc)

# 2. Build and install Azur
cmake -B build-linux \
  -DAZUR_PLATFORM=linux \
  -DCMAKE_INSTALL_PREFIX="$AZUR_PATH_linux"
make -C build-linux install -j$(nproc)
```

**Building for emscripten**

You will need the [emscripten toolchain](https://emscripten.org/). I recommend to use a dedicated install folder since this is essentially a cross-compiling target.

```bash
# Set and export it globally (e.g. in ~/.profile)
export AZUR_PATH_emscripten="$HOME/.prefix-emscripten"

# 1. Build and install dependencies
emcmake cmake -B build-emscripten-3rdparty -S 3rdparty \
  -DAZUR_PLATFORM=emscripten \
  -DCMAKE_INSTALL_PREFIX="$AZUR_PATH_emscripten"
make -C build-emscripten-3rdparty install -j$(nproc)

# 2. Build and install Azur
emcmake cmake -B build-emscripten \
  -DAZUR_PLATFORM=emscripten \
  -DCMAKE_INSTALL_PREFIX="$AZUR_PATH_emscripten"
make -C build-emscripten install -j$(nproc)
```

## Using in an application

Users building programs with Azur for Linux or emscripten should export the environment variables `AZUR_PATH_linux` or `AZUR_PATH_emscripten` as described above. The variable `AZUR_PLATFORM` should also be set on the command-line with `-D` during configuration, or hardcoded if the application only supports one target.

The application's `CMakeLists.txt` can then do:

```cmake
list(APPEND CMAKE_MODULE_PATH "$ENV{AZUR_PATH_${AZUR_PLATFORM}}/lib/cmake")
find_package(Azur 0.1 REQUIRED)
```

When using gint, only the `find_package()` is necessary. The fxSDK handles the rest.

The find module then exports up to three targets that can be linked against: `Azur::Azur`, `Azur::ImGui`, and others. See the docs for a full list of the supported targets. The result might look like this:

```cmake
if(AZUR_PLATFORM STREQUAL gint)
  target_link_libraries(myprogram Azur::Azur)
else() # Linux and emscripten
  target_link_libraries(myprogram Azur::Azur Azur::GLM Azur::ImGui)
endif()
```

## History note

Back in 2021 I made a single repository for this engine, along with other programs that I intended to write with it. At the time I didn't intend to distribute the engine in any serious fashion, but that changed after people expressed interest in playing with [After Burner](https://www.planet-casio.com/Fr/programmes/programme4238-1-after-burner-lephenixnoir-jeux-add-ins.html).

I split the original repository in May 2022 with `git filter-branch`, extracting Azur-related contents to this repository. I patched old commits so they can still be built, but mostly for the sake of preserving history; don't expect to be able to use these early commits out-of-the-box.
