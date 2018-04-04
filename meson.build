project('engine', 'cpp', 'c',
	default_options : [
		'cpp_std=c++11',
		'default_library=static'
	])

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

glfw_dep = dependency('glfw3', version : '>=3.2', required : false)
if not glfw_dep.found()
	glfw_dep = subproject('glfw').get_variable('glfw_dep')
endif

if cxx.has_header('glm/glm.hpp')
	glm_dep = declare_dependency()
else
	glm_dep = dependency('glm', required : false)
	if not glm_dep.found()
		glm_dep = subproject('glm').get_variable('glm_dep')
	endif
	glm_dep = subproject('glm').get_variable('glm_dep')
endif

freeimage_dep = cxx.find_library('freeimage')
assimp_dep = cxx.find_library('assimp')

bass_sdk = get_option('bass-sdk')
if bass_sdk == ''
	bass_dep = dependency('bass')
else
	bass_inc = include_directories(join_paths(bass_sdk))

	bass_libpath = bass_sdk
	if host_machine.cpu_family() == 'x86_64'
		bass_libpath = join_paths(bass_sdk, 'x64')
	endif

	bass_lib = cxx.find_library('bass', dirs : bass_libpath)
	bass_dep = declare_dependency(dependencies : bass_lib,
	                              include_directories : bass_inc)
endif

glslang = find_program('glslangValidator',
                       join_paths(vulkan_sdk, 'bin', 'glslangValidator'))

subdir('src')
subdir('data')

install_subdir('assets', install_dir : '/')
