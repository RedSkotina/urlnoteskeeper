project('urlnoteskeeper', 'c')
src = ['unkplugin.c', 'unkgui.c', 'unksidebar.c', 'unkmatcher.c', 'unkdb.c']
geanydep = dependency('geany', version : '>=1.2.2', required : true)
sqlitedep = dependency('sqlite3', required : true)
add_project_arguments('-DLOCALEDIR="' + join_paths(get_option('prefix'), get_option('localedir')) +'"', language:'c')
shared_library('unk', src, version : '0.1.0', soversion : '0', dependencies : [geanydep, sqlitedep])
