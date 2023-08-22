project('engine', 'cpp', 'c',
	default_options : [
		'cpp_std=c++11',
		'default_library=static',
		'b_ndebug=if-release'
	],
	meson_version: '>=0.58')

meson.add_postconf_script(
	join_paths(meson.project_source_root(), 'bin/create_link.py'),
	join_paths(meson.project_source_root(), 'assets'),
	join_paths(meson.project_build_root(), 'assets'))

cxx = meson.get_compiler('cpp')

add_project_arguments(cxx.get_supported_arguments('-mf16c'), language : 'cpp')


python = import('python').find_installation('python3')

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
	cmake = import('cmake')
	glfw_dep = cmake.subproject('glfw').dependency('glfw')
endif

if not glm_dep.found()
	cmake = import('cmake')
	glm_dep = cmake.subproject('glm').dependency('glm')
endif

freeimage_dep = cxx.find_library('freeimage')
assimp_dep = cxx.find_library('assimp')

bass_dep = dependency('bass', fallback: 'bass-' + host_machine.system())

subdir('src')
subdir('tools')
subdir('data')

install_subdir('assets', install_dir : '')

devenv = environment()
devenv.set('VK_INSTANCE_LAYERS', 'VK_LAYER_KHRONOS_validation')
meson.add_devenv(devenv)
