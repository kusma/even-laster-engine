subdir('shaders')

import('fs').copyfile(
	'soundtrack.mp3',
	'soundtrack.mp3',
	install : true,
	install_dir : 'data/')

custom_target('fbm.raw', output : 'fbm.raw',
	command : [gen_fbm, '@OUTPUT@'],
	build_by_default : true,
	install : true,
	install_dir : 'data/')

dump_tracks = files(join_paths(meson.project_source_root(), 'bin', 'dump_tracks.py'))
outdir = meson.current_build_dir()

custom_target('sync.rocket',
	input : files(join_paths('..', 'sync.rocket')),
	output : '.sync.rocket.timestamp',
	command : [
		python, dump_tracks, '@INPUT@',
		'--timestamp', '@OUTPUT@',
		'--outdir', meson.current_build_dir() ],
	build_by_default : true)

meson.add_install_script(
	join_paths(meson.project_source_root(), 'bin', 'install_tracks.py'),
	meson.current_build_dir())
