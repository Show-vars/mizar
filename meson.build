project('mizar', 'c',
  version: '0.1'
)

mizar_sources = [
  'src/log.c',
  'src/decoder/mp3.c',
  'src/output/alsa.c',
  'src/ringbuffer.c',
  'src/commandqueue.c',
  'src/extend.c',
  'src/playback.c',
  'src/main.c',
  'src/util.c',
]

inc = include_directories(
  'src',
  '.',
)

conf = configuration_data()
conf.set_quoted('VERSION', meson.project_version())
conf.set('DEVEL_LOGGING_ENABLED', get_option('buildtype') == 'debug')

configure_file(output: 'config.h', configuration: conf)

libuv_dep = subproject('libuv')
lua_dep   = subproject('lua')

mizar_deps = [
  dependency('libuv', version: '>=1.18',  fallback: ['libuv', 'libuv_dep']),
  dependency('alsa',  version: '>=1.1.3'),
  dependency('lua',   version: '>=5.3.0', fallback: ['lua', 'lua_dep'])
]

executable('mizar', mizar_sources, dependencies: mizar_deps, include_directories: inc)
