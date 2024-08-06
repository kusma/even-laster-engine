sources = files('main.cpp', 'renderer/buffer.cpp', 'renderer/import-texture.cpp', 'renderer/pipeline.cpp', 'renderer/scenerenderer.cpp', 'renderer/shader.cpp', 'renderer/texture.cpp', 'renderer/vkinstance.cpp'
, 'scene/sceneimporter.cpp', 'swapchain.cpp', 'sync/device.c', 'sync/track.c')

demo_deps = [
	assimp_dep,
	bass_dep,
	freeimage_dep,
	glfw_dep,
	glm_dep,
	vulkan_dep,
	vk_mem_alloc_dep,
]

executable(
	'demo',
	sources,
	dependencies: demo_deps,
	include_directories: [inc_volk],
	c_args: '-DSYNC_PLAYER',
	cpp_args: '-DSYNC_PLAYER',
	win_subsystem: 'windows',
	install: true,
	install_dir: 'bin',
)

executable(
	'editor',
	sources,
	dependencies : demo_deps,
	include_directories: [inc_volk],
	win_subsystem : 'windows'
)
