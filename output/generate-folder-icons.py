"""Pre-rasterize folder icon sizes so animation never resamples images on the MCU."""
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
lines=['#include "views/watch_folder_icons.h"']
for name in ('puzzle','flappy','snake'):
    source=Image.open(ROOT/'output'/f'icon-{name}.png').convert('RGBA')
    for size in range(64,89,4):
        im=source.resize((size,size),Image.Resampling.LANCZOS)
        # RGB565A8: compact native RGB565 plane followed by the alpha plane.
        pixels=bytearray();alpha=bytearray()
        for r,g,b,a in im.getdata():
            v=((r>>3)<<11)|((g>>2)<<5)|(b>>3)
            pixels.extend((v&255,v>>8));alpha.append(a)
        data=pixels+alpha;ident=f'folder_{name}_{size}'
        lines.append(f'static const uint8_t {ident}_pixels[] = {{')
        lines += [','.join(f'0x{v:02x}' for v in data[i:i+32])+',' for i in range(0,len(data),32)]
        lines += ['};',f'static const lv_image_dsc_t {ident} = {{',f'.header={{.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_RGB565A8,.w={size},.h={size},.stride={size*2}}},',f'.data_size=sizeof {ident}_pixels,.data={ident}_pixels','};']
lines.append('const lv_image_dsc_t *const watch_folder_icons[3][7] = {')
for name in ('puzzle','flappy','snake'):
    lines.append('{'+','.join(f'&folder_{name}_{size}' for size in range(64,89,4))+'},')
lines.append('};')
(ROOT/'App/views/watch_folder_icons.c').write_text('\n'.join(lines)+'\n')
(ROOT/'App/views/watch_folder_icons.h').write_text('#ifndef WATCH_FOLDER_ICONS_H\n#define WATCH_FOLDER_ICONS_H\n#include "lvgl.h"\nextern const lv_image_dsc_t *const watch_folder_icons[3][7];\n#endif\n')
