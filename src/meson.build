sources = files(
	'main.cpp',
	'scene/buffer.cpp',
	'scene/import-texture.cpp',
	'scene/scene.cpp',
	'scene/sceneimporter.cpp',
	'scene/texture.cpp',
	'sync/device.c',
	'sync/track.c',
	'scenerenderer.cpp',
	'shader.cpp',
	'swapchain.cpp',
	'vkInstance.cpp'
)

demo_deps = [
	assimp_dep,
	gst_dep,
	freeimage_dep,
	glfw_dep,
	glm_dep,
	vulkan_dep
]
executable(
	'demo',
	sources,
	dependencies : demo_deps,
	c_args : '-DSYNC_PLAYER',
	cpp_args : '-DSYNC_PLAYER',
	win_subsystem : 'windows',
	install : true,
	install_dir : 'bin'
)

executable(
	'editor',
	sources,
	dependencies : demo_deps,
	win_subsystem : 'windows'
)

gen_fbm = executable(
	'gen-fbm',
	files('gen-fbm.cpp', 'FastNoise.cpp', 'FastNoise.h')
)
