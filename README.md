# Smartwatch

STM32U5A9J-DK için STM32CubeIDE projesi. TouchGFX gerektirmez; LVGL 9.3.0 kullanır.

## Çalıştırma durumu — 4 Ekim 2026

- CubeIDE 2.2.0 Debug derlemesi: 0 hata, 0 uyarı.
- Flash: 623.212 bayt; statik RAM: 1.090.912 bayt (data + bss).
- Bağlı STM32U5A9J-DK'ye yüklendi; flash doğrulaması başarılı.
- Uygulama çalıştırıldı. `smartwatch_status=4` (ekran, dokunmatik, UI hazır).
- Çalışma sayacı iki okumada 7'den 35'e ilerledi.
- `screen.png`, kartın gerçek framebuffer verisinden elde edilmiş menü görüntüsüdür.
  Panelin fotoğrafı değildir. Dokunmatik donanım başlatıldı; her kullanıcı akışı ayrıca elle denenmelidir.

## CubeIDE'de açma

File → Import → General → Existing Projects into Workspace → Select root directory
adımında bu **Smartwatch** klasörünü seçin. Smartwatch projesini işaretleyip Finish'e basın.
Projeyi bu konumda kullanmak için “Copy projects into workspace” seçeneğini kapalı tutun.
Project → Build Project ile derleyin. Run/Debug Configuration oluştururken ST-LINK ve
`Debug/Smartwatch.elf` dosyasını seçin. Kartın BOOT0 düğmesine basmadan normal başlangıç kullanın.

Alternatif olarak bu klasörde PowerShell'den `./Build.ps1`, ardından `./Flash.ps1` çalıştırılabilir.
IDE sürümü değişirse betiklere `-IdeRoot` parametresi verin.
Betikteki ST-LINK seri numarası bu oturumda bağlı karta aittir.

## Dosyalar

- `App/watch_ui.c`: saat, tarih, halkalar, menü, takvim, ayarlar.
- `Core/Src/main.c`: sistem saati, RTC, LVGL ekran/dokunmatik portu ve ana döngü.
- `Core/Inc/build_time.h`: ilk RTC başlatmasında kullanılan tarih ve saat.
- `Middlewares/lv_conf.h`: LVGL yapılandırması, 32 bit ARGB görüntü ve 256 KB LVGL havuzu.
- `Drivers`: ST HAL, CMSIS ve bu karta ait BSP sürücüleri.
- `Debug/Smartwatch.elf`: derlenmiş ve karta yüklenmiş firmware.
- `Backup/previous-internal-flash.bin`: yüklemeden önce okunan 4 MB dahili flash yedeği.
  Yedek, harici flash/eMMC veya option-byte ayarlarını içermez.

## İlk sürüm sınırları

Pil yüzdesi ölçülmediği için `--` görünür; aktivite halkaları demo verisidir.
RTC ilk başlatmada derleme tarih/saatine ayarlanır, gerçek zaman sunucusuna bağlı değildir.
Şimdilik dahili LSI osilatörü kullanır; zaman kayması olabilir. Güç tamamen kesilirse
RTC korunması garanti edilmez. Hassas saat ve kullanıcıdan saat/tarih ayarı sonraki geliştirmedir.
12/24 saat ve açık/koyu tema ayarları RAM'de tutulur.
Takvimde aylar arasında gezinme ve gün seçme vardır; etkinlik kaydı/çevrimiçi eşitleme yoktur.
Metinler standart Montserrat fontuyla uyum için ASCII Türkçe yazılmıştır.

Ekran portu BSP'nin GFXMMU framebuffer'ına satır satır yazar; yazılım çizimi kullanır.
GPU hızlandırması ve yırtılmasız çift tampon bu sürümde etkin değildir.

## Kaynaklar ve lisanslar

- ST STM32CubeU5 resmi BSP örneğinin sistem saati, startup ve linker dosyaları temel alındı.
- HAL/CMSIS/BSP bileşenleri ST'nin resmi depolarından alınmıştır; kendi LICENSE dosyaları korunmuştur.
- LVGL kaynakları v9.3.0 etiketinden alınmıştır; `Middlewares/lvgl/LICENCE.txt` korunmuştur.
- https://github.com/STMicroelectronics/STM32CubeU5
- https://github.com/STMicroelectronics/stm32u5x9j-dk-bsp
- https://github.com/lvgl/lvgl/tree/v9.3.0

## GitHub copy

The previous internal-flash backup and local build/debug outputs are excluded from this repository. Firmware/Smartwatch.bin contains only the new Smartwatch application. Flash.ps1 requires the ST-LINK serial number as a parameter. Build.ps1 creates a private local workspace under .local/.

