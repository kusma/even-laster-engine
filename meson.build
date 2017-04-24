project('engine', 'cpp', 'c', default_options : [ 'cpp_std=c++11' ])

add_global_arguments('-mf16c', language : 'cpp')

cxx = meson.get_compiler('cpp')

vulkan_sdk = get_option('vulkan-sdk')
vulkan_inc = include_directories(join_paths(vulkan_sdk, 'include'))
vulkan_lib = cxx.find_library('vulkan', dirs : join_paths(vulkan_sdk, 'lib'), required : false)
if not vulkan_lib.found()
	vulkan_lib = cxx.find_library('vulkan-1', dirs : join_paths(vulkan_sdk, 'lib'), required : false)
endif

vulkan_dep = declare_dependency(dependencies : vulkan_lib,
                                include_directories : vulkan_inc)

glfw_dep = dependency('glfw3', version : '>=3.2')
glm_dep = dependency('glm')

freeimage_dep = cxx.find_library('freeimage')
assimp_dep = cxx.find_library('assimp')

bass_sdk = get_option('bass-sdk')
bass_inc = include_directories(join_paths(bass_sdk))
bass_lib = cxx.find_library('bass', dirs : join_paths(bass_sdk, 'x64'))
bass_dep = declare_dependency(dependencies : bass_lib,
                                include_directories : bass_inc)

glslang = find_program('glslangValidator',
                       join_paths(vulkan_sdk, 'bin', 'glslangValidator'))

subdir('src')
subdir('data')

install_subdir('assets', install_dir : '/')
