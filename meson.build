project('engine', 'cpp', 'c',
	default_options : [
		'cpp_std=c++11',
		'default_library=static',
		'b_ndebug=if-release'
	],
	meson_version: '>=0.56')

meson.add_postconf_script(
	join_paths(meson.project_source_root(), 'bin/create_link.py'),
	join_paths(meson.project_source_root(), 'assets'),
	join_paths(meson.project_build_root(), 'assets'))

cxx = meson.get_compiler('cpp')

add_project_arguments(cxx.get_supported_arguments('-mf16c'), language : 'cpp')


python = import('python').find_installation('python3')

if host_machine.system() == 'windows'
	add_project_arguments('-DNOMINMAX', language : 'cpp')
endif

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

wsock2_dep = cxx.find_library('ws2_32', required: false)

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

freeimage_dep = cxx.find_library('freeimage', required: false)
if not freeimage_dep.found()
	freeimage_proj = subproject('freeimage')
	freeimage_dep = freeimage_proj.get_variable('freeimage_dep')
endif

assimp_dep = cxx.find_library('assimp', required: false)
if not assimp_dep.found()
	cmake = import('cmake')
	opt_var = cmake.subproject_options()
	opt_var.add_cmake_defines({
		'ASSIMP_BUILD_ASSIMP_TOOLS': false,
		'ASSIMP_BUILD_TESTS': false
	})
	assimp_dep = cmake.subproject('assimp', options: opt_var).dependency('assimp')
endif

if host_machine.system() == 'windows'
	bass_sdk = get_option('bass-sdk')
	libdirs = [
		join_paths(bass_sdk, 'c', 'x64'),
		join_paths(bass_sdk, 'c')
	]
	bass_inc = include_directories(join_paths(bass_sdk, 'c'))
	bass_lib = cxx.find_library('bass', dirs : libdirs)
	bass_dep = declare_dependency(dependencies : bass_lib,
								include_directories : bass_inc)
	add_project_arguments('-DUSE_BASS', language : 'cpp')
	gst_dep = dependency('', required: false)
else
	gst_dep = dependency('gstreamer-1.0')
	bass_dep = dependency('', required: false)
endif

subdir('src')
subdir('data')

install_subdir('assets', install_dir : '')
