subdir('shaders')

install_data(sources : [ 'soundtrack.mp3' ], install_dir : '/data')

custom_target('fbm.raw', output : 'fbm.raw',
	command : [gen_fbm, '@OUTPUT@'],
	build_by_default : true,
	install : true,
	install_dir : '/data/')
