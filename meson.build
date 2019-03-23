project('urlnoteskeeper', 'c')
src = ['unkplugin.c', 'unkgui.c', 'unksidebar.c', 'unkmatcher.c', 'unkdb.c', 'unkkeys.c', 'gtk3compat.c']

cc = meson.get_compiler('c')
libm = cc.find_library('m', required: true)
geanydep = dependency('geany', version : '>=1.2.2', required : true)
sqlitedep = dependency('sqlite3', required : true)

dependencies = [libm,  geanydep, sqlitedep]

build_os = build_machine.system()
message('Build OS: ' + build_os)
build_platform_win32 = (build_os.startswith('mingw') or 
                        build_os.startswith('cygwin') or 
                        build_os.startswith('windows'))
if build_platform_win32                        
    #gtk = dependency('gtk+-2.0', version: '>= 2.12')
else
    gtk = dependency('gtk+-3.0', version: '>= 3.21.6')
    dependencies = [gtk] + dependencies
endif

add_project_arguments('-DLOCALEDIR="' + join_paths(get_option('prefix'), get_option('localedir')) +'"', language:'c')
shared_library('unk', src, version : '0.1.0', soversion : '0', dependencies : dependencies)
