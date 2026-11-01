# How to build Otto Matic

## The easy way: build.py (automated build script)

`build.py` can produce a game executable from a fresh clone of the repo in a single command. It will work on macOS, Windows and Linux, provided that your system has Python 3, CMake, and an adequate C++ compiler.

```
git clone --recurse-submodules https://github.com/jorio/OttoMatic
cd OttoMatic
python3 build.py
```

If you want to build the game **manually** instead, the rest of this document describes how to do just that on each of the big 3 desktop operating systems.

## Build and run in a browser

Requires [Emscripten 4.0.23](https://emscripten.org/docs/getting_started/downloads.html),
CMake 3.24+, Python 3, and Git. CMake fetches SDL 3.2.30 and gl4es.
The pinned [Pomme fork](https://github.com/Rage997/Pomme) reuses the game's SDL target.

Run from the repository root, keeping web and native builds separate:

```sh
source /path/to/emsdk/emsdk_env.sh
git submodule sync
git submodule update --init --recursive
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web --parallel 2
python3 -m http.server 8073 --bind 127.0.0.1 --directory build-web
```

Open <http://127.0.0.1:8073/OttoMatic.html> and click **Play**. Do not open the HTML via `file://`.
The initial download is about 200 MB across six asset packages.

### Browser behavior

- Requires WebAssembly, WebGL 2, browser storage, and a keyboard or gamepad. No touch controls.
- **J** shoots; **K** punches or picks up items. Other controls are configurable.
- Play enables audio. Losing focus pauses the game; click the canvas or **Resume game** to continue.
- Rendering follows the page size at full device-pixel resolution, without resolution
  presets or caps. Resizing the window, changing display density, and entering or
  leaving fullscreen update the drawable while preserving logical mouse coordinates.
  Press **Esc** to release the captured mouse before using page controls.
- Saves, preferences, and scores stay in this browser. Wait for **Saved in this browser**
  before closing the tab. Clearing site data deletes progress.
- Storage errors offer a retry; failed save restoration blocks startup.

Artwork is in `packaging/web-assets/`; the launcher is `packaging/web.html`.
Browser build settings live in the root `CMakeLists.txt`, matching the sibling ports.
The gl4es compatibility patch and content-versioning helper remain in `web/`.


## How to build the game manually on macOS

1. Install the prerequisites:
    - Xcode (preferably the latest version)
    - [CMake](https://formulae.brew.sh/formula/cmake) 3.21+ (installing via Homebrew is recommended)
1. Clone the repo **recursively**:
    ```
    git clone --recurse-submodules https://github.com/jorio/OttoMatic
    cd OttoMatic
    ```
1. Download [SDL3-3.2.4.dmg](https://libsdl.org/release/SDL3-3.2.4.dmg), open it, then browse to SDL3.xcframework/macos-arm64_x86_64. In that folder, copy **SDL3.framework** to the game's **extern** folder.
1. Prep the Xcode project:
    ```
    cmake -G Xcode -S . -B build
    ```
1. Now you can open `build/OttoMatic.xcodeproj` in Xcode, or you can just go ahead and build the game:
    ```
    cmake --build build --config RelWithDebInfo
    ```
1. The game gets built in `build/RelWithDebInfo/OttoMatic.app`. Enjoy!

## How to build the game manually on Windows

1. Install the prerequisites:
    - Visual Studio 2022 with the C++ toolchain
    - [CMake](https://cmake.org/download/) 3.21+
1. Clone the repo **recursively**:
    ```
    git clone --recurse-submodules https://github.com/jorio/OttoMatic
    cd OttoMatic
    ```
1. Download [SDL3-devel-3.2.4-VC.zip](https://libsdl.org/release/SDL3-devel-3.2.4-VC.zip), extract it, and copy **SDL3-3.2.4** to the **extern** folder. Rename **SDL3-3.2.4** to just **SDL3**.
1. Prep the Visual Studio solution:
    ```
    cmake -G "Visual Studio 17 2022" -A x64 -S . -B build
    ```
1. Now you can open `build/OttoMatic.sln` in Visual Studio, or you can just go ahead and build the game:
    ```
    cmake --build build --config Release
    ```
1. The game gets built in `build/Release/OttoMatic.exe`. Enjoy!

## How to build the game manually on Linux et al.

1. Install the prerequisites from your package manager:
    - Any C++20 compiler
    - CMake 3.21+
    - SDL3 development library (e.g. "libsdl3-dev" on Ubuntu, "sdl3" on Arch, "SDL3-devel" on Fedora)
    - OpenGL development libraries (e.g. "libgl1-mesa-dev" on Ubuntu)
1. Clone the repo **recursively**:
    ```
    git clone --recurse-submodules https://github.com/jorio/OttoMatic
    cd OttoMatic
    ```
1. Build the game:
    ```
    cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
    cmake --build build
    ```
    If you'd like to enable runtime sanitizers, append `-DSANITIZE=1` to the **first** `cmake` call above.
1. The game gets built in `build/OttoMatic`. Enjoy!
