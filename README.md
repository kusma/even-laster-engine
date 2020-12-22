# Even Laster Engine

This is a demo-engine, using Vulkan for rendering.

## Building on Linux

OK, so some people have complained about difficulties building this on Linux,
so here's a small guide:

1. Install Meson (and Ninja): http://mesonbuild.com
2. Install these dependencies (most distros have them all):
  - GLFW: http://www.glfw.org/
    - fedora: dnf install glfw-devel
    - ubuntu: apt install libglfw3-dev
  - GLM: https://glm.g-truc.net/
    - fedora: dnf install glm-devel
    - ubuntu: apt install libglm-dev
  - Assimp: http://www.assimp.org/
    - fedora: dnf install assimp-devel
    - ubuntu: apt install libassimp-dev
  - FreeImage: http://freeimage.sourceforge.net/
    - fedora: dnf install freeimage-devel
    - ubuntu: apt install libfreeimage-dev
  - Vulkan Loader and Validation Layers: https://github.com/KhronosGroup/Vulkan-Loader
    - fedora: dnf install vulkan-devel vulkan-validation-layers
    - ubuntu: apt install libvulkan-dev vulkan-validationlayers
  - glslang: https://github.com/KhronosGroup/glslang
    - fedora: dnf install glslang
    - ubuntu: apt install glslang-tools
3. Download and uncompress BASS somewhere convenient
4. Run meson. Something like `meson -Dbass-sdk=<path-to-bass> build` should do.
5. Compile. I usually do `ninja -C build`
6. Run the demo. Something like this: `$(cd build && ./src/demo

There's also an install-step. It behaves a bit differently than most other
Unix-build systems, though; it doesn't install things globally, it's only
really there to help staging a portable installation directory. Use the
environment variable $DESTDIR to control where the staging-directory is.
