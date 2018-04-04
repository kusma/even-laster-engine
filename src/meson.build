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

executable(
	'demo',
	sources,
	dependencies : [
		assimp_dep,
		bass_dep,
		freeimage_dep,
		glfw_dep,
		glm_dep,
		thread_dep,
		vulkan_dep
	],
	cpp_args : '-DSYNC_PLAYER',
	gui_app : true,
	install : true,
	install_dir : '/'
)
