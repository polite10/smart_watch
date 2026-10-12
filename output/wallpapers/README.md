# Aurora duvar kâğıtları

12 Ekim 2026. Yerleşik `image_gen` aracıyla üretilmiştir (CLI/API anahtarı kullanılmadı).
Kaynak görseller bu klasörde saklanır:

- `aurora-green.png`: yeşil / turkuaz.
- `aurora-violet.png`: mavi / mor.
- `aurora-rose.png`: gül / mor.

`output/generate-arcade-art.py`, bu kaynakları firmware için 480 × 480 RGB565
olarak dönüştürür. Çalışma sırasında JPEG/PNG çözme veya görüntü ölçekleme yoktur.
Ayarlar → Duvar kagidi sayfasındaki oklar önizlemeyi değiştirir; ✓ kalıcı kaydeder.

Üç çağrıda da kullanılan tam istem (PALETTE yerleri aşağıdadır):

> Use case: stylized-concept. Asset type: square smartwatch wallpaper, full bleed. Create an elegant dark aurora wallpaper with PALETTE flowing soft light ribbons over near-black navy. Soft smooth gradients, subdued luminance for colorful app icon legibility. Abstract light only, no landscape, no stars, no text, no frames, no UI or watch device. Concentrate delicate luminous folds toward the edges and keep the center quiet and dark. Square composition.

1. PALETTE = `emerald green and cyan`
2. PALETTE = `deep blue and violet`
3. PALETTE = `muted rose and warm purple`

`transparent_background=false`; referans görüntü kullanılmadı.
