gen_fbm = executable(
	'gen-fbm',
	files('gen-fbm.cpp', 'FastNoise.cpp', 'FastNoise.h'),
	cpp_args: cxx.get_supported_arguments([
		'-Wno-strict-aliasing',
		'-Wno-maybe-uninitialized',
		'-Wno-unused-function'])
)
