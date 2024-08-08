sources = [
	'background.frag',
	'bartikkel.frag',
	'bartikkel.vert',
	'bloom_downscale.frag',
	'bloom_upscale.frag',
	'fullscreenquad.vert',
	'particle.geom',
	'postprocess.comp',
	'refraction.frag',
	'refraction.vert',
	'smoke.frag',
	'smoke.vert',
]

foreach s : sources
	infile = files(join_paths('..', '..', 'src', 'shaders', s))
	outfile = s + '.spv'
	depfile = s + '.d'
	custom_target(
		outfile,
		input: infile,
		output: outfile,
		depfile: depfile,
		command: [
			glslc,
			'-O',
			'--target-env=vulkan1.1',
			'@INPUT@',
			'-o',
			'@OUTPUT@',
			'-MD', '-MF', '@DEPFILE@',
		],
		build_by_default: true,
		install: true,
		install_dir: 'data/shaders/',
	)
endforeach
