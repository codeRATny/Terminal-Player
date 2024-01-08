# Terminal-Player

## Requirements
```
libavcodec
libavformat
libavutil
libswscale
libswresample
```

## Player options
* --input / -i (str) : input file or url
* --realtime / -r (bool) : on / off realtime (default on)
* --color / -c (rgb/gray) : set color space (default rgb)
* --decoder-threads / -d : threads per decoder (default 4)
* --painter-threads / -p : threads per painter (default 8)
