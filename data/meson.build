subdir('shaders')

configure_file(
   input : files('soundtrack.mp3'),
   output : '@PLAINNAME@',
   install_dir : '/data',
   copy : true)

custom_target('fbm.raw', output : 'fbm.raw',
	command : [gen_fbm, '@OUTPUT@'],
	build_by_default : true,
	install : true,
	install_dir : '/data/')
