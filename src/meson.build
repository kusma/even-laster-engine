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
	bass_dep,
	freeimage_dep,
	glfw_dep,
	glm_dep,
	vulkan_dep
]
executable(
	'demo',
	sources,
	dependencies : demo_deps,
	cpp_args : '-DSYNC_PLAYER',
	gui_app : true,
	install : true,
	install_dir : '/'
)

executable(
	'editor',
	sources,
	dependencies : demo_deps,
	gui_app : true,
	install : true,
	install_dir : '/'
)

gen_fbm = executable(
	'gen-fbm',
	files('gen-fbm.cpp', 'FastNoise.cpp', 'FastNoise.h')
)
