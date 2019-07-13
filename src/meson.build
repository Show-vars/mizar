mizar_sources = [
  'log.c',
  'decoder/mp3.c',
  'output/alsa.c',
  'ringbuffer.c',
  'extend.c',
  'playback.c',
  'main.c',
  'util.c',
]

conf = configuration_data()
conf.set_quoted('VERSION', meson.project_version())
conf.set('DEVEL_LOGGING', get_option('buildtype') == 'debug')

configure_file(output: 'config.h', configuration: conf)

mizar_deps = [
  dependency('libuv',  version: '>=1.18'),
  dependency('alsa',   version: '>=1.1.3'),
  dependency('luajit', version: '>=2.0.0')
]

executable('mizar', mizar_sources, dependencies: mizar_deps)
