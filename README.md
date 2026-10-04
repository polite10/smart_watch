# Smartwatch

STM32U5A9J-DK için STM32CubeIDE projesi. LVGL 9.3.0 kullanır; TouchGFX gerekmez.

## Ekran görüntüleri ve hazır firmware

- [Üç saat arayüzünün görünümü](output/watch-faces-preview.png)
- [Arayüz seçim sayfası](output/face-picker.png)
- `Firmware/Smartwatch.bin`: mevcut başarılı Debug derlemesinden üretilen ham firmware;
  yükleme başlangıç adresi `0x08000000` olmalıdır. ELF ile geliştirme için aşağıdaki derleme akışını kullanın.

GitHub sürümü güncel uygulama kaynaklarını, hazır firmware'i, ekran görüntülerini ve
ilgili doğrulama günlüklerini içerir. PDF rehberleri ve belge üretim betikleri yerel tutulur.
`Debug/`, `Backup/`, geçici PDF render dosyaları ve kullanıcı ayarlarını içeren `.bin` yakalamaları
yerel tutulur. README'deki `Backup/` yolları geliştirme bilgisayarındaki yedekleri anlatır.

## Yeni sürüm — 4 Ekim 2026

Menüde saat, alarm, hesap makinesi, not defteri, arayüzler, su takibi, takvim ve ayarlar bulunur.
Metinler mevcut Montserrat fontuyla uyumlu ASCII Türkçedir. Panel 480 × 480 piksel ve dokunmatiktir.

### Su takibi

- **Bir bardak 200 ml.** Ekleme ve geri alma düğmeleri vardır; miktar sıfırın altına düşmez.
- İlerleme halkası, bugünün toplamı ve hedefe kalan miktar gösterilir.
- Başlangıç hedefi 2000 ml; kullanıcı 200 ml adımlarla 200–6000 ml arasında değiştirir.
- Gün değişince toplam sıfırlanır. Son 14 kayıtlı gün, o günün hedefiyle saklanır.
- Hatırlatma: kapalı / 30 / 60 / 120 dakika. 08:00–22:00 arasında, hedef tamamlanmadığında ekran uyarısı çıkar.
- Su ekleme süreyi yeniden başlatır; “Daha sonra” seçilen süre kadar erteler. Yeniden başlatmada süre yeniden başlar.
- Miktar, geçmiş, hedef ve hatırlatma ayarı flash hafızada korunur.

### Alarm, hesap ve notlar

- Üç alarm: saat/dakika seçimi, aç/kapat, her gün veya bir kez. Kaydedilen alarm etkinleşir.
- Alarm ekranı ve kırmızı LED uyarısı; durdurma ve 5 dakika erteleme. Aynı dakikada tetiklenen alarmlar sırayla gösterilir.
- Bu sürüm ses/titreşim çıkışı veya uyku modundan alarm uyandırması içermez. Denetim firmware çalışırken yapılır.
- Hesap makinesi: dört işlem, negatif/ondalıklı sayılar, işlem önceliği, silme, temizleme ve sonuçla devam etme.
  Sıfıra bölme, eksik işlem ve taşma reddedilir; sonuç en fazla altı ondalık basamakla gösterilir.
- Dört not, her biri en fazla 191 ASCII karakter. Dokunmatik klavye, kaydetme, iptal ve iki dokunuşla silme.

### Özelleştirilebilir saat arayüzleri

Pusula kaldırıldı; yerine üç saat görünümü ve renk seçimi eklendi:

- **Pastel:** yumuşak renkler, büyük dijital saat ve su hedefi kartı.
- **Neon:** dijital saat, canlı saniye ve gerçek su hedefi ilerlemesini gösteren dış halka.
- **Klasik:** krem kadran, saat/dakika/saniye ibreleri, tarih ve su miktarı.

Menü → Arayüzler veya Ayarlar → Saat arayüzleri sayfasını açın. Saat ekranına basılı tutmak da
aynı sayfayı açar. Üç küçük önizlemeden birini, ardından Pembe / Mint / Mavi vurgu rengini seçin.
**Uygula** seçimi kaydeder ve saat ekranına döner. Uygulamadan geri çıkmak mevcut seçimi korur.
Seçim yeniden başlatmada korunur. Uygulamaların açık/koyu teması saat kadranlarının renklerinden ayrıdır.
Yeni seçim için eski ayar yapısının iki ayrılmış baytı kullanıldığı için mevcut notlar, alarmlar ve su
kayıtları korunur. Saat fontu mevcut Montserrat bitmaplerinden türetilmiş 72 px rakam/iki nokta fontudur.

### RTC

RTC kartın 32.768 kHz LSE kristalini kullanır. Kristal başlatılamazsa LSI'ye döner; kaynak ayarlarda gösterilir.
İlk başlatma saati derleme anındaki İstanbul saatidir. Tam güç kesintisinde RTC korunması garanti edilmez;
flash kayıtları korunur. Pil ölçümü henüz bağlı değildir; geçerli veri gelince kadranda gösterilir.
Takvim ay gezinmesi ve gün seçimi sunar; etkinlik/eşitleme yoktur.

## Derleme ve çalıştırma

CubeIDE'de File → Import → General → Existing Projects into Workspace ile bu klasörü seçin.
Yerinde kullanmak için “Copy projects into workspace” kapalı olsun. Project → Build Project ile derleyin.
Run/Debug için ST-LINK ve `Debug/Smartwatch.elf` seçilir.

PowerShell'den `./Build.ps1`, ardından `./Flash.ps1` çalıştırılabilir.
Betikler Türkçe klasör yollarını destekler; Eclipse çalışma alanı geçici dizinde tutulur.
`Core/Inc/build_time.h` her derlemede İstanbul saatiyle güncellenir.
IDE konumu için `-IdeRoot`, farklı kart için Flash.ps1'de `-SerialNumber` parametresi kullanılır.

Kartın açılışı doğrulama sırasında flash uygulamasını seçecek şekilde ayarlandı: `nSWBOOT0=0`, `nBOOT0=1`,
`NSBOOTADD0=0x08000000`. Önceki seçenekler `output/boot-options.log` içindedir; önceki pin seçimine dönmek
isterseniz CubeProgrammer'da `nSWBOOT0=1` yapın. Bu seçim BOOT0 pininin açılış seçimindeki etkisini kaldırır.

## Kalıcı hafıza

Son iki 8 KB flash sayfası (0x083FC000–0x083FFFFF) ayarlar günlüğüne ayrılmıştır; linker firmware alanını
4080 KB ile sınırlar. 1 KB kayıtlar CRC32 ve sıra numarası taşır. Başlık son yazılır; sayfa geçişinde en son
geçerli kayıt korunur. Değişmeyen veri yeniden yazılmaz. Kayıt hatası ekranda gösterilir.
Normal firmware yüklemesi bu sayfaları korur; toplu flash silme kayıtları da siler.

## Doğrulama

- Son Debug derlemesi: **0 hata, 0 uyarı**. Flash (text + data): **560.676 bayt**; statik RAM: **1.094.520 bayt**.
- Bağlı kartta yükleme doğrulaması geçti; uygulama çalıştırıldı (`smartwatch_status=4`).
- Önceki sürümün `output/verify-features.log` kaydı: gerçek hedefte **29 kontrol geçti, 0 başarısızlık**.
  Su miktarı/sınırları/gün değişimi, hatırlatmalar, alarm/tekrar/erteleme, hesap hataları,
  flash günlüğünün sayfa geçişi ve not kaydı doğrulandı.
- `output/verify-reboot.log`: gerçek sistem yeniden başlatmasında su ve not kaydı korundu;
  UI hazır ve LSE aktif (`status=4`, `crystal=1`, `failures=0`). Geçici test kayıtları temizlendi.
- `output/verify-faces.log`: **25 kontrol geçti, 0 başarısızlık**. Üç görünümün ve renklerin uygulanması,
  yalnız seçili kadranın görünmesi, 12 saat biçimi, saniye ve analog ibreler, basılı tutma, seçimlerin
  gerçek reset sonrası korunması doğrulandı. Önceki/sonraki 916 baytlık ayarlar birebir aynı kaldı.
- `output/watch-faces-preview.png` ve `output/face-picker.png`: gerçek GFXMMU görüntü belleğinden okunan
  üç kadranın ve seçim sayfasının görüntüleri;
  panel fotoğrafı değildir. UI olayları hedefte işlendi. Fiziksel parmak dokunuşları ayrıca denenebilir.

## Dosyalar

- `App/watch_ui.c`: ekranlar ve kullanıcı etkileşimleri.
- `App/watch_faces.c`: üç kadran, canlı saat/veri güncellemeleri ve görünüm/renk seçimi.
- `App/watch_font_clock.c`: saat için büyük rakam fontu.
- `App/watch_model.c`: alarm, su takibi ve hesap mantığı.
- `App/watch_storage.c`: flash kayıt günlüğü.
- `Core/Src/main.c`: RTC, ekran/dokunmatik portu ve ana döngü.
- `Middlewares/lv_conf.h`: LVGL yapılandırması ve 256 KB havuz.
- `Backup/before-features-flash.bin`: bu sürümden önceki 4 MB dahili flash yedeği.
- `Backup/before-faces-flash.bin`: arayüz sürümünden önceki firmware ve kalıcı kayıt yedeği.
- `Backup/previous-internal-flash.bin`: önceki oturumun flash yedeği.

## Kaynaklar ve lisanslar

ST örneğinin sistem saati, startup ve linker dosyaları temel alındı. HAL/CMSIS/BSP lisansları kendi
dizinlerinde korunmuştur. LVGL v9.3.0 lisansı `Middlewares/lvgl/LICENCE.txt` dosyasındadır.

- https://github.com/STMicroelectronics/STM32CubeU5
- https://github.com/STMicroelectronics/stm32u5x9j-dk-bsp
- https://github.com/lvgl/lvgl/tree/v9.3.0
- https://www.st.com/en/evaluation-tools/stm32u5a9j-dk.html
- https://www.st.com/resource/en/user_manual/um2967-discovery-kits-with-stm32u5x9nj-mcus-stmicroelectronics.pdf
- https://support.apple.com/en-ie/guide/watch/apde9218b440/watchos
- https://www.samsung.com/us/support/answer/ANS10002950/
