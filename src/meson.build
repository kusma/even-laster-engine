sources = files(
	'main.cpp',
	'renderer/import-texture.cpp',
	'renderer/buffer.cpp',
	'scene/sceneimporter.cpp',
	'renderer/texture.cpp',
	'sync/device.c',
	'sync/track.c',
	'renderer/scenerenderer.cpp',
	'renderer/shader.cpp',
	'swapchain.cpp',
	'renderer/vkinstance.cpp'
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
