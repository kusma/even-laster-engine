project(
	'engine',
	'cpp',
	'c',
	default_options: [
		'cpp_std=c++20',
		'default_library=static',
		'b_ndebug=if-release',
	],
	meson_version: '>=1.3',
)

meson.add_postconf_script(
	join_paths(meson.project_source_root(), 'bin/create_link.py'),
	join_paths(meson.project_source_root(), 'assets'),
	join_paths(meson.project_build_root(), 'assets'),
)

cxx = meson.get_compiler('cpp')

add_project_arguments(cxx.get_supported_arguments('-mf16c'), language: 'cpp')

add_project_arguments('-DNOMINMAX', language : 'cpp')

python = import('python').find_installation('python3')

vulkan_dep = dependency('vulkan')
glslc = find_program('glslc')
glfw_dep = dependency('glfw3', version: '>=3.2')
glm_dep = dependency('glm')

freeimage_dep = dependency('freeimage', required: false, fallback: 'freeimage')
if not freeimage_dep.found()
	freeimage_dep = cxx.find_library('freeimage')
endif

assimp_dep = dependency('assimp')

bass_dep = dependency('bass', fallback: 'bass-' + host_machine.system())

subdir('src')
subdir('tools')
subdir('data')

install_subdir('assets', install_dir: '')

devenv = environment()
devenv.set('VK_INSTANCE_LAYERS', 'VK_LAYER_KHRONOS_validation')
devenv.set('VK_LAYER_SETTINGS_PATH', meson.current_source_dir())
meson.add_devenv(devenv)
