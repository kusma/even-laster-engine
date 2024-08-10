gen_fbm = executable(
	'gen-fbm',
	files('FastNoise.cpp', 'FastNoise.h', 'gen-fbm.cpp'),
	cpp_args: cxx.get_supported_arguments(
		[
			'-Wno-strict-aliasing',
			'-Wno-maybe-uninitialized',
			'-Wno-unused-function',
		],
	),
)
