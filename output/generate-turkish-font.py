"""Add Turkish glyphs derived from MIT-licensed LVGL Montserrat ASCII glyphs."""
from pathlib import Path
import re
from PIL import Image,ImageDraw
root=Path(__file__).resolve().parents[1]
lines=['/* Derived from LVGL Montserrat; LVGL MIT license applies. */','#include "views/watch_fonts.h"']
chars={'Ç':'C','ç':'c','Ğ':'G','ğ':'g','İ':'I','ı':'i','Ö':'O','ö':'o','Ş':'S','ş':'s','Ü':'U','ü':'u'}
for size in (16,20,24):
    source=(root/f'Middlewares/lvgl/src/font/lv_font_montserrat_{size}.c').read_text()
    block=re.sub(r'/\*.*?\*/','',source.split('glyph_bitmap[] = {')[1].split('};')[0],flags=re.S)
    bitmap=bytes(int(n,16) for n in re.findall(r'0x([0-9a-fA-F]+)',block))
    descriptors=re.findall(r'\{([^{}]+)\}',source.split('glyph_dsc[] = {')[1].split('};')[0])
    out=bytearray();rows=['{0,0,0,0,0,0}'];cmaps=[]
    for glyph,(char,base) in enumerate(sorted(chars.items()),1):
        offset,advance,w,h,x,y=[int(v) for v in re.findall(r'=\s*(-?\d+)',descriptors[ord(base)-32+1])]
        im=Image.new('L',(w,h));im.putdata([(((bitmap[offset+i//2]>>4) if i%2==0 else bitmap[offset+i//2]&15)*17) for i in range(w*h)])
        top=4;bottom=3 if char in 'ÇçŞş' else 0
        canvas=Image.new('L',(max(w,4),h+top+bottom));canvas.paste(im,(0,top));d=ImageDraw.Draw(canvas)
        if char=='ı':
            # Remove the dot while retaining the original advance and baseline.
            gap=next((row for row in range(h) if not any(im.getpixel((col,row)) for col in range(w))),3)
            d.rectangle((0,top,canvas.width-1,top+gap),fill=0)
        elif char in 'ÖöÜü':
            d.rectangle((max(0,w//2-3),0,max(0,w//2-2),1),fill=255);d.rectangle((min(w-1,w//2+1),0,min(w-1,w//2+2),1),fill=255)
        elif char in 'Ğğ':
            d.line([(max(0,w//2-3),0),(max(0,w//2-2),2),(min(w-1,w//2+2),2),(min(w-1,w//2+3),0)],fill=255,width=1)
        elif char=='İ':d.rectangle((max(0,w//2-1),0,min(w-1,w//2),1),fill=255)
        else:d.line([(w//2,h+top-1),(w//2+1,h+top+1),(max(0,w//2-1),h+top+2)],fill=255,width=1)
        levels=[min(15,(v+8)//17) for v in canvas.getdata()]
        rows.append('{%d,%d,%d,%d,%d,%d}'%(len(out),advance,canvas.width,canvas.height,x,y-bottom))
        if len(levels)%2:levels.append(0)
        out.extend((levels[i]<<4)|levels[i+1] for i in range(0,len(levels),2))
        cmaps.append('{.range_start=%d,.range_length=1,.glyph_id_start=%d,.type=LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY}'%(ord(char),glyph))
    lines += [f'static const uint8_t bitmap_{size}[]={{',','.join(str(v) for v in out),'};',f'static const lv_font_fmt_txt_glyph_dsc_t glyphs_{size}[]={{',',\n'.join(rows),'};',f'static const lv_font_fmt_txt_cmap_t cmaps_{size}[]={{',',\n'.join(cmaps),'};',f'static const lv_font_fmt_txt_dsc_t dsc_{size}={{.glyph_bitmap=bitmap_{size},.glyph_dsc=glyphs_{size},.cmaps=cmaps_{size},.cmap_num=12,.bpp=4,.bitmap_format=0}};']
    lh=int(re.search(r'\.line_height\s*=\s*(\d+)',source).group(1));bl=int(re.search(r'\.base_line\s*=\s*(\d+)',source).group(1))
    lines.append(f'const lv_font_t watch_font_turkish_{size}={{.get_glyph_dsc=lv_font_get_glyph_dsc_fmt_txt,.get_glyph_bitmap=lv_font_get_bitmap_fmt_txt,.line_height={lh},.base_line={bl},.dsc=&dsc_{size},.fallback=&lv_font_montserrat_{size}}};')
(root/'App/views/watch_fonts.c').write_text('\n'.join(lines)+'\n')
(root/'App/views/watch_fonts.h').write_text('#ifndef WATCH_FONTS_H\n#define WATCH_FONTS_H\n#include "lvgl.h"\n'+''.join(f'extern const lv_font_t watch_font_turkish_{size};\n' for size in (16,20,24))+'#endif\n')
