sources = [
	'bloom.frag',
	'bloom_upscale.frag',
	'fullscreenquad.vert',
	'plane.frag',
	'plane.vert',
	'postprocess.comp',
	'refraction.frag',
	'refraction.vert'
]

foreach s : sources
	infile = files(join_paths('..', '..', 'src', 'shaders', s))
	outfile = s + '.spv'
	custom_target(outfile, input : infile, output : outfile,
		command : [glslang, '-V', '@INPUT@', '-o', '@OUTPUT@'],
		build_by_default : true,
		install : true,
		install_dir : '/data/shaders/')
endforeach
