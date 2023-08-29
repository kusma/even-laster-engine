# Even Laster Engine

This is a demo-engine, using Vulkan for rendering.

## Building on Linux

OK, so some people have complained about difficulties building this on Linux,
so here's a small guide:

1. Install Meson (and Ninja): http://mesonbuild.com
2. Install these dependencies (most distros have them all):
   - GLFW: http://www.glfw.org/
   - GLM: https://glm.g-truc.net/
   - Assimp: http://www.assimp.org/
   - FreeImage: http://freeimage.sourceforge.net/
   - Vulkan Loader and Validation Layers: https://github.com/KhronosGroup/Vulkan-Loader
   - glslang: https://github.com/KhronosGroup/glslang

   In case you're using Fedora or Debian, here's the exact commands:
   - Fedora:
     ```console
     dnf install glfw-devel assimp-devel freeimage-devel vulkan-devel \
                 vulkan-validation-layers glslang
     ```
   - Debian:
     ```console
     apt install libglfw3-dev libglm-dev libassimp-dev libfreeimage-dev \
                 libvulkan-dev vulkan-validationlayers glslang-tools
     ```

3. Run meson. Something like `meson setup build` should do.
4. Compile. I usually do `meson compile -C build`
5. Run the demo. Something like this: `meson devenv -C build ./src/demo`

There's also an install-step. It behaves a bit differently than most other
Unix-build systems, though; it doesn't install things globally, it's only
really there to help staging a portable installation directory. Use the
environment variable $DESTDIR to control where the staging-directory is.
