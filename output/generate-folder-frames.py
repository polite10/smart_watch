"""Pre-rasterize transparent folder motion independently of wallpaper and carousel.

17 RGB565A8 frames use independent LZ4 blocks. A single 270 KB
buffer is decoded in place, avoiding runtime icon scaling and baked backdrops.
"""
from pathlib import Path
from PIL import Image, ImageDraw
import struct
ROOT=Path(__file__).resolve().parents[1]
SIZE=300
SCALE=3

# Use the project's bundled LZ4 compressor; no pip or network dependency.
import ctypes, subprocess, tempfile
native=tempfile.TemporaryDirectory(prefix='watch-folder-lz4-')
libpath=Path(native.name)/'lz4.so'
shim=Path(native.name)/'memory.c'
shim.write_text('#include <string.h>\nvoid *lv_memcpy(void *d,const void *s,size_t n){return memcpy(d,s,n);}'
                '\nvoid *lv_memmove(void *d,const void *s,size_t n){return memmove(d,s,n);}'
                '\nvoid *lv_memset(void *d,int c,size_t n){return memset(d,c,n);}')
subprocess.run(['cc','-shared','-fPIC','-O2','-DLV_CONF_INCLUDE_SIMPLE',
                '-I'+str(ROOT/'Middlewares'),'-I'+str(ROOT/'Middlewares/lvgl'),
                '-I'+str(ROOT/'App'),str(ROOT/'Middlewares/lvgl/src/libs/lz4/lz4.c'),
                str(shim),'-o',str(libpath)],check=True)
lz4=ctypes.CDLL(str(libpath))
lz4.LZ4_compressBound.argtypes=[ctypes.c_int]
lz4.LZ4_compress_default.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int]
lz4.LZ4_decompress_safe.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_int,ctypes.c_int]
def compress(raw):
    buffer=ctypes.create_string_buffer(lz4.LZ4_compressBound(len(raw)))
    size=lz4.LZ4_compress_default(raw,buffer,len(raw),len(buffer))
    assert size>0
    packed=buffer.raw[:size];decoded=ctypes.create_string_buffer(len(raw))
    assert lz4.LZ4_decompress_safe(packed,decoded,len(packed),len(raw))==len(raw)
    assert decoded.raw==raw
    return packed

def frame(stage):
    t=stage/16
    im=Image.new('RGBA',(SIZE*SCALE,SIZE*SCALE))
    d=ImageDraw.Draw(im)
    diameter=190+110*t;r=diameter/2
    d.ellipse(tuple(round(v*SCALE) for v in (150-r,150-r,150+r-1,150+r-1)),
              fill=(188,212,236,32),outline=(132,157,181,255),width=SCALE)
    small=[(150,107),(187,171),(113,171)]
    large=[(150,75),(215,188),(85,188)]
    for i,name in enumerate(('puzzle','flappy','snake')):
        size=round((64+24*t)*SCALE)
        x=round((small[i][0]+(large[i][0]-small[i][0])*t)*SCALE-size/2)
        y=round((small[i][1]+(large[i][1]-small[i][1])*t)*SCALE-size/2)
        badge=Image.new('RGBA',(size,size));bd=ImageDraw.Draw(badge)
        bd.ellipse((0,0,size-1,size-1),fill='#b7dc8b' if i==2 else '#ffd18b')
        icon=Image.open(ROOT/'output'/f'icon-{name}.png').convert('RGBA').resize((size,size),Image.Resampling.LANCZOS)
        badge.alpha_composite(icon);im.alpha_composite(badge,(x,y))
    im=im.resize((SIZE,SIZE),Image.Resampling.LANCZOS)
    rgb=bytearray();alpha=bytearray()
    for r,g,b,a in im.getdata():
        # Transparent pixels must be zero for useful delta compression.
        v=((r>>3)<<11)|((g>>2)<<5)|(b>>3) if a else 0
        rgb.extend(struct.pack('<H',v));alpha.append(a)
    raw=rgb+alpha
    return bytes(raw)

lines=['/* Generated transparent frames: output/generate-folder-frames.py. */','#include "views/watch_folder_frames.h"']
total=0;sizes=[]
for stage in range(17):
    packed=compress(frame(stage));sizes.append(len(packed));total+=len(packed)
    lines.append(f'static const uint8_t folder_frame_{stage}[] __attribute__((aligned(4))) = {{')
    lines += [','.join(f'0x{x:02x}' for x in packed[i:i+32])+',' for i in range(0,len(packed),32)]
    lines.append('};')
lines += ['const uint8_t *const watch_folder_frames[WATCH_FOLDER_FRAME_COUNT] = {',','.join(f'folder_frame_{i}' for i in range(17)),'};']
lines += ['const uint32_t watch_folder_frame_sizes[WATCH_FOLDER_FRAME_COUNT] = {'+','.join(map(str,sizes))+'};']
(ROOT/'App/views/watch_folder_frames.c').write_text('\n'.join(lines)+'\n')
print(f'17 transparent LZ4 frames: {total} flash bytes; 270000 RAM bytes; round-trips passed.')
