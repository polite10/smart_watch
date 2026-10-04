from pathlib import Path
from PIL import Image, ImageDraw
root = Path(__file__).resolve().parent
for file in root.glob('*-framebuffer.bin'):
    data = file.read_bytes()
    frame = Image.frombytes('RGBA', (768, 480), data, 'raw', 'BGRA').crop((0, 0, 480, 480)).convert('RGB')
    mask = Image.new('L', (480, 480))
    ImageDraw.Draw(mask).ellipse((0, 0, 479, 479), fill=255)
    image = Image.new('RGB', (480, 480), '#20242c')
    image.paste(frame, (0, 0), mask)
    image.save(root / (file.stem.replace('-framebuffer', '') + '.png'))
names = ['menu', 'water', 'calculator', 'notes', 'alarm', 'face-picker']
sheet = Image.new('RGB', (1440, 960), '#20242c')
for i, name in enumerate(names):
    sheet.paste(Image.open(root / (name + '.png')), ((i % 3) * 480, (i // 3) * 480))
sheet.save(root / 'new-apps-preview.png')
