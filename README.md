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
Derleme zamanı artık gerçek saat olarak kabul edilmez. İlk güncellemede veya RTC yedek alanı kaybolduğunda
**Saat ve tarih** ekranı açılır. Telefondaki tarih ve saati 24 saat biçiminde seçip **Kaydet** düğmesine basın;
saniye 00 olur. Daha sonra **Ayarlar → Saat ve tarih ayarla** üzerinden düzeltilebilir. Geçerli aralık
2000–2099'dur; ay ve artık yıla göre gün seçenekleri değişir. Kaydetmeden geri dönmek saati değiştirmez.

Saat bilinmiyorken dijital kadran `--:--` gösterir, analog ibreler gizlenir; alarmlar, su hatırlatmaları ve
günlük su kayıtları saat ayarlanmasını bekler. Hesap makinesi ve notlar kullanılabilir. Saati düzeltmek
eski erteleme sürelerini temizler; ayarlanan dakikaya denk gelen alarm hemen çalmaz. Tarih daha önce
kayıtlı bir güne döndürülürse o günün su kaydı korunur. Notlar ve tüm mevcut flash ayarları korunur.

**Priz adaptörü sadece güç sağlar; güncel saat göndermez.** Wi-Fi/Bluetooth ve yedek besleme olmadan,
tam güç kesintisinde geçen süre cihaz tarafından belirlenemez. Tekrar açılışta tarih/saat elle girilir.
Güç açıkken sistem reseti RTC'yi korur; uygulama otomatik başlar ve tekrar derleme gerekmez.

RTC yedek pili, STM32'nin `VBAT` alanındaki RTC/kristal ve yedek kayıtları ana besleme kesilince yaşatan
küçük beslemedir; ekranı veya uygulamayı çalıştırmaz, saat doğruluğunu kalibre etmez. Piliniz olmadan bu
sürüm kullanılabilir. STM32U5A9J-DK üzerinde SB29 varsayılan olarak VBAT'ı VDD_MCU'ya bağlar; harici pil
eklemek kart şeması/revizyonuna uygun bağlantı ve besleme ayrımı gerektirir. VBAT'a doğrudan pil bağlamayın.
Bkz. [STM32U5A9NJ datasheet](https://www.st.com/resource/en/datasheet/stm32u5a9nj.pdf) ve
[UM2967 kart kılavuzu](https://www.st.com/resource/en/user_manual/um2967-discovery-kit-with-stm32u5a9nj-mcu-stmicroelectronics.pdf).

Pil ölçümü henüz bağlı değildir; geçerli veri gelince kadranda gösterilir.
Takvim ay gezinmesi ve gün seçimi sunar; etkinlik/eşitleme yoktur.

### Bilgisayardan isteğe bağlı saat eşitleme

Kartın **ST-LINK USB** portunu bilgisayara takın ve `./Sync-Time.ps1` çalıştırın. Betik bilgisayarın güncel
İstanbul saatini, UI görev sınırında ST-LINK üzerinden RTC'ye aktarır ve uygulamayı çalışır durumda bırakır.
IDE'de yeniden derleme veya firmware yükleme yapmaz; USB seri haberleşmesi/Wi-Fi/Bluetooth kullanmaz.
Bilgisayarın kendi saatinin doğru olması gerekir. Priz adaptörüyle kullanırken bu yöntem kullanılamaz;
kart üzerindeki manuel ayar kullanılır. Aktif IDE Debug oturumunu önce kapatın.

STM32CubeIDE'nin kurulu araçları ve **karta yüklenen firmware ile eşleşen** `Debug/Smartwatch.elf` dosyası
gereklidir. Başka bilgisayarda önce aynı kaynak sürümünü derleyip yükleyin. `-IdeRoot`, `-SerialNumber` ve
`-Port` parametreleri desteklenir. Saat, bağlantı kurulup hedef durduktan sonra okunur; sabit zaman damgası
kullanılmaz. İşlem sırasında kısa süreli duraklama vardır; betik harici servis kurmaz.

## Derleme ve çalıştırma

CubeIDE'de File → Import → General → Existing Projects into Workspace ile bu klasörü seçin.
Yerinde kullanmak için “Copy projects into workspace” kapalı olsun. Project → Build Project ile derleyin.
Run/Debug için ST-LINK ve `Debug/Smartwatch.elf` seçilir.

PowerShell'den `./Build.ps1`, ardından `./Flash.ps1` çalıştırılabilir.
Betikler Türkçe klasör yollarını destekler; Eclipse çalışma alanı geçici dizinde tutulur.
`Core/Inc/build_time.h` her derlemede İstanbul saatiyle güncellenir; yalnız tarih ayarı taslağının başlangıç
değeridir, RTC'ye otomatik olarak gerçek saat diye yazılmaz.
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

- Son Debug derlemesi: **0 hata, 0 uyarı**. Güncel boyutlar `output/build-clock.log` içinde bulunur.
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
- `output/verify-clock.log`: bağlı hedefte **33 kontrol geçti, 0 başarısızlık**. Saat kaybında ayar ekranı,
  geçersiz tarih reddi, artık yıl, gün sınırlaması, RTC'ye kaydetme, alarm sürelerinin düzeltilmesi,
  su kaydının tarih düzeltmesinde korunması ve besleme açıkken sistem reseti doğrulandı.
  RTC saat kaybı, yedek geçerlilik işareti silinerek simüle edildi; fiziksel güç kesintisi uygulanmadı.
  Test öncesi/sonrası 916 baytlık kalıcı kullanıcı verisi birebir aynı kaldı.
- `output/sync-clock.log`: bilgisayardan ST-LINK üzerinden güncel saat aktarıldı.
- `output/clock-setup.png`, `output/clock-settings.png`: yeni sayfaların hedef görüntü belleği yakalamaları.

## Dosyalar

- `App/watch_ui.c`: ekranlar ve kullanıcı etkileşimleri.
- `App/watch_faces.c`: üç kadran, canlı saat/veri güncellemeleri ve görünüm/renk seçimi.
- `App/watch_font_clock.c`: saat için büyük rakam fontu.
- `App/watch_model.c`: alarm, su takibi ve hesap mantığı.
- `App/watch_storage.c`: flash kayıt günlüğü.
- `Core/Src/main.c`: RTC, ekran/dokunmatik portu ve ana döngü.
- `App/watch_clock.h`: tarih/saat doğrulamalı donanım RTC ayarı arayüzü.
- `Sync-Time.ps1`: bilgisayardan isteğe bağlı İstanbul saati aktarımı.
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
