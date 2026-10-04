from pathlib import Path
from PIL import Image, ImageDraw
root = Path(__file__).resolve().parent
names = ['face-pastel', 'face-neon', 'face-classic', 'face-picker']
for name in names:
    data = (root / (name + '-framebuffer.bin')).read_bytes()
    frame = Image.frombytes('RGBA', (768, 480), data, 'raw', 'BGRA').crop((0, 0, 480, 480)).convert('RGB')
    mask = Image.new('L', (480, 480))
    ImageDraw.Draw(mask).ellipse((0, 0, 479, 479), fill=255)
    image = Image.new('RGB', (480, 480), '#22252c')
    image.paste(frame, (0, 0), mask)
    image.save(root / (name + '.png'))
sheet = Image.new('RGB', (1440, 480), '#22252c')
for i, name in enumerate(names[:3]):
    sheet.paste(Image.open(root / (name + '.png')), (i * 480, 0))
sheet.save(root / 'watch-faces-preview.png')
before = (root / 'settings-before-faces-test.bin').read_bytes()
after = (root / 'settings-after-faces-test.bin').read_bytes()
assert before == after, 'Settings changed during face tests'
print('Preserved all settings, notes, alarms and water records:', len(before), 'bytes')
