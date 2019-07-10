mizar_sources = [
  'decoder/mp3.c',
  'output/alsa.c',
  'ringbuffer.c',
  'extend.c',
  'playback.c',
  'main.c',
  'util.c',
]

mizar_deps = [
  dependency('libuv', version: '>=1.23'),
  dependency('alsa', version:  '>=1.1.8'),
  dependency('luajit', version:  '>=2.0.0')
]

executable('mizar', mizar_sources,
  dependencies: mizar_deps
)