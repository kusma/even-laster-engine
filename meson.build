project('engine', 'cpp', 'c',
	default_options : [
		'cpp_std=c++11',
		'default_library=static',
		'b_ndebug=if-release'
	])

add_global_arguments('-mf16c', language : 'cpp')

meson.add_postconf_script(
	join_paths(meson.source_root(), 'bin/create_link.py'),
	join_paths(meson.source_root(), 'assets'),
	join_paths(meson.build_root(), 'assets'))

cxx = meson.get_compiler('cpp')

vulkan_sdk = get_option('vulkan-sdk')
if vulkan_sdk != ''
	vulkan_inc = include_directories(join_paths(vulkan_sdk, 'include'))
	vulkan_lib = cxx.find_library('vulkan', dirs : join_paths(vulkan_sdk, 'lib'), required : false)
	if not vulkan_lib.found()
		vulkan_lib = cxx.find_library('vulkan-1', dirs : join_paths(vulkan_sdk, 'lib'), required : false)
	endif
	vulkan_dep = declare_dependency(dependencies : vulkan_lib,
	                                include_directories : vulkan_inc)

	glslang = find_program('glslangValidator',
	                       join_paths(vulkan_sdk, 'bin', 'glslangValidator'))
else
	vulkan_dep = dependency('vulkan')
	glslang = find_program('glslangValidator')
endif


glfw_dep = dependency('glfw3', version : '>=3.2', required : false)
glm_dep = dependency('glm', required : false)

if not glfw_dep.found()
	glfw_dep = subproject('glfw').get_variable('glfw_dep')
endif

if not glm_dep.found()
	glm_dep = subproject('glm').get_variable('glm_dep')
endif

freeimage_dep = cxx.find_library('freeimage')
assimp_dep = cxx.find_library('assimp')

bass_sdk = get_option('bass-sdk')
if bass_sdk == ''
	bass_dep = dependency('bass')
else
	bass_inc = include_directories(join_paths(bass_sdk))
	bass_lib = cxx.find_library('bass', dirs : [ join_paths(bass_sdk, 'x64'), bass_sdk ] )
	bass_dep = declare_dependency(dependencies : bass_lib,
	                              include_directories : bass_inc)
endif

subdir('src')
subdir('data')

install_subdir('assets', install_dir : '')
