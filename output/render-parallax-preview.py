"""Render a short GIF from the compiled ARM/LVGL Flappy scene, without hardware."""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
harness=ROOT/'output/verify-round-ui.py'
ctx={'__file__':str(harness),'__name__':'parallax_preview'}
exec(compile(harness.read_text().split("snapshot('home')")[0],str(harness),'exec'),ctx)
call=ctx['call'];Image=ctx['Image'];ImageDraw=ctx['ImageDraw']
call('flappy_open');call('watch_flappy_vm_flap')
frames=[]
mask=Image.new('L',(480,480));ImageDraw.Draw(mask).ellipse((0,0,479,479),fill=255)
for index in range(20):
    ctx['tick']+=60;call('arcade_tick',0)
    state=call('watch_flappy_vm_state')
    y=ctx['struct'].unpack('<f',ctx['u'].mem_read(state,4))[0]
    if y>242:call('watch_flappy_vm_flap')
    call('lv_display_refr_timer',0)
    picture=Image.frombytes('RGBA',(480,480),bytes(ctx['frame']),'raw','BGRA').convert('RGB')
    out=Image.new('RGB',(480,480),'#171e2c');out.paste(picture,(0,0),mask);frames.append(out)
assert frames[0].tobytes()!=frames[-1].tobytes()
frames[0].save(ROOT/'output/flappy-parallax.gif',save_all=True,append_images=frames[1:],duration=60,loop=0)
print('Rendered 20 real ARM/LVGL frames to output/flappy-parallax.gif')
