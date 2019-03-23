project('urlnoteskeeper', 'c')
src = ['unkplugin.c', 'unkgui.c', 'unksidebar.c', 'unkmatcher.c', 'unkdb.c', 'unkkeys.c', 'gtk3compat.c']
gtk3 = dependency('gtk+-3.0', version: '>= 3.21.6')
geanydep = dependency('geany', version : '>=1.2.2', required : true)
sqlitedep = dependency('sqlite3', required : true)
cc = meson.get_compiler('c')
libm = cc.find_library('m', required: true)
add_project_arguments('-DLOCALEDIR="' + join_paths(get_option('prefix'), get_option('localedir')) +'"', language:'c')
shared_library('unk', src, version : '0.1.0', soversion : '0', dependencies : [gtk3, libm,  geanydep, sqlitedep])
