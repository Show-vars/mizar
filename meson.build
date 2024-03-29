project('mizar', 'c',
  version: '0.1',
  default_options: 'c_std=c11'
)

mizar_sources = [
  'src/logging.c',
  'src/decoder/mp3.c',
  'src/output/alsa.c',
  'src/audiobuffer.c',
  'src/pcm_conv.c',
  'src/audio_io.c',
  'src/audio_ctrl.c',
  #'src/commandqueue.c',
  #'src/extend.c',
  #'src/osc.c',
  #'src/osc_ctrl.c',
  #'src/playback.c',
  'src/main.c',
]

inc = include_directories(
  'src',
  '.',
)

conf = configuration_data()
conf.set_quoted('VERSION', meson.project_version())
conf.set('DEVEL_LOGGING_ENABLED', get_option('buildtype') == 'debug')
conf.set('WORDS_BIGENDIAN', build_machine.endian() == 'big')

configure_file(output: 'config.h', configuration: conf)

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)
atomic_dep = cc.find_library('atomic', required : false)
thread_dep = dependency('threads')
libuv_dep = subproject('libuv')
lua_dep   = subproject('lua')


mizar_deps = [
  m_dep, atomic_dep, thread_dep,
  dependency('alsa',  version: '>=1.1.3'),
  dependency('lua',   version: '>=5.3.0', fallback: ['lua', 'lua_dep'])
]

executable('mizar', mizar_sources, dependencies: mizar_deps, include_directories: inc)
