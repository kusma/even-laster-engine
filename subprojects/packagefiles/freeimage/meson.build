project('freeimage', 'c')

cc = meson.get_compiler('c')
distdir = meson.current_source_dir() / 'Dist' / 'x64'
lib = cc.find_library('freeimage', dirs : distdir)
freeimage_dep = declare_dependency(
  dependencies : lib,
  include_directories : include_directories(distdir))
