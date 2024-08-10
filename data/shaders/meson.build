sources = [
	'bartikkel.frag',
	'bartikkel.geom',
	'bartikkel.vert',
	'bloom_downscale.frag',
	'bloom_upscale.frag',
	'fullscreenquad.vert',
	'postprocess.comp',
	'refraction.frag',
	'refraction.vert',
]

foreach s : sources
	infile = files(join_paths('..', '..', 'src', 'shaders', s))
	outfile = s + '.spv'
	custom_target(
		outfile,
		input: infile,
		output: outfile,
		command: [
			glslang,
			'-V',
			'--target-env',
			'vulkan1.3',
			'@INPUT@',
			'-o',
			'@OUTPUT@',
		],
		build_by_default: true,
		install: true,
		install_dir: 'data/shaders/',
	)
endforeach
