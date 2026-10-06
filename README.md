# Smartwatch

STM32U5A9J-DK için STM32CubeIDE projesi. LVGL 9.3.0 kullanır; TouchGFX gerekmez.

## 15 Bulmaca, dokuz uygulama ve animasyonlar — 6 Ekim 2026

Menü artık **dokuz adet 92 px ikon** içerir. Ortadaki amber ikon yeni **15 Bulmaca**
oyununu açar. Üç sıra soldan sağa ve merkez sıra etrafında üstten alta simetriktir;
üst sıranın yan ikonları aşağıda, alt sıranın yan ikonları yukarıdadır. Bütün
dokunma alanları 480 px yuvarlak panelin içinde kalır.

Oyun, **4 × 4** karede 1–15 sayıları ve tek boş hücre kullanır. Yalnız boş hücrenin
yatay veya dikey komşusuna dokunmak taşı hareket ettirir. Hedef soldan sağa,
yukarıdan aşağı 1–15 sırası ve sağ altta boş hücredir. Her açılışta ve **Yeni oyun**
düğmesinde yeni, çözülebilir ve henüz tamamlanmamış bir tahta hazırlanır.
Fisher–Yates karıştırması ve çift genişlikli tahtanın parite düzeltmesi kullanılır;
zamana ve açılış sayacına dayanan sözde rastgelelik oyun içindir.

Taşlar **62 px**, aralıklar **8 px**; oyun tahtası ve yeni oyun düğmesi çemberin
içindedir. Doğru hücredeki taşlar mint rengiyle vurgulanır. Her geçerli hareket
hamle sayısını bir artırır. Son hareket tamamlanınca **Tebrikler!** ve toplam hamle
sayısı görünür; yeni oyun başlatılana kadar taşlar sabit kalır. Oyun RAM'de tutulur;
menüden yeniden açılması eski oyunu sürdürmez ve flash kayıt formatını değiştirmez.

Animasyonlar:

- Saatten kaydırarak menüyü açınca ikonlar **12 px** yukarı hareket ederek yerleşir;
  **160 ms** hareket ve ikon başına **12 ms** gecikme kullanılır.
- Uygulama ve düzenleme sayfaları açılırken içerik **18 px** sağdan yerine gelir,
  hareket **140 ms** sürer. Başlık yerinde kalır; saate kilitleme anında uygulanır.
- Küçük butonlar basılınca yaklaşık **%5** küçülür ve bırakılınca geri gelir;
  renk ve ölçek geçişi **90 ms** sürer. Büyük kartlarda renk geçişi kullanılır.
  Alarm ve tema anahtarları **120 ms** içinde hareket eder.
- Bulmaca taşı boş hücreye **140 ms** içinde kayar. Hareket sırasında ikinci taş
  komutu engellenir; yeni oyun hareketi iptal edip yeni tahtayı doğru konumlandırır.
- Hızlı sayfa değişimleri eski giriş hareketlerini temizler; ikon konumları birikmez.
  Tam ekran opacity tamponu kullanılmaz. Gerçek panelde FPS/akıcılık ayrıca ölçülmelidir.

Yeni dosyalar `App/models/watch_puzzle.[ch]`, `App/viewmodels/watch_puzzle_vm.c`
ve `App/views/watch_puzzle_view.c` dosyalarıdır. Menü ve ikon varlıkları,
`watch_viewmodels.h`, `watch_view_internal.h`, ortak widgetlar, navigasyon ve
mevcut ekranların geçiş çağrıları güncellendi. Mevcut uygulamaları baştan yazmak
gerekmedi. İkonları `output/generate-menu-icons.py` yeniden üretir.
`Build.ps1` ELF'i, `Firmware/Smartwatch.bin` dosyasını ve derleme zamanı başlığını
yeniler; doğrulama betikleri raporları ve ekran görüntülerini üretir.

[Dokuz uygulamalı menü](output/round-menu.png), [oyun](output/round-puzzle.png) ve
[kazanılmış oyun](output/round-puzzle-won.png) derlenmiş ARM/LVGL çizimleridir.
Yeni sürüm için `output/verify-round-ui.json` ve `output/verify-viewmodels.json`
raporları geçerlidir; aşağıdaki 5 Ekim test sayıları geçmiş sürümü anlatır.
Derleme **0 hata, 0 uyarı**; ARM/LVGL ekran ve dokunma testlerinde **125**,
ekransız Model/ViewModel testlerinde **54** kontrol geçti: toplam **179**, başarısız **0**.
Bulmaca testleri 512 farklı çözülebilir başlangıç, 1024 geçerli hareket, geçersiz
dokunma, kazanma, yeniden başlatma ve mevcut 916 baytlık kayıtların korunmasını kapsar.
Bu sürümde fiziksel karta yükleme yapılmadı.

Sonraki uygulama fikri: **kronometre ve geri sayım**. Büyük başlat/duraklat düğmesi,
tur süreleri ve yuvarlak ilerleme halkası mevcut donanımla uyumludur. Alternatifler
nefes egzersizi veya Pomodoro sayacıdır; bu sürümde uygulama sayısı dokuzdur.

## MVVM düzeni ve analog kadran — 5 Ekim 2026

Uygulama Model–View–ViewModel düzenine taşındı. Her özelliğin ekranı ve ViewModel'i
ayrı dosyalardadır; RTC, flash, ekran/dokunmatik bağlantıları donanım modüllerindedir.
ViewModel'ler LVGL veya HAL başlatmadan test edilebilir. Mevcut **916 baytlık**
kayıt formatı ve ana döngü korunmuştur. Dosya eşleşmeleri ve geliştirme kuralları
[ARCHITECTURE.md](ARCHITECTURE.md) içindedir.

Analog kadranın dijital saniye etiketi kaldırıldı. Saniye ibresi korunur;
rakamlar ve ibre merkezi ekranın tam merkezine hizalanır. Tarih üstte, su bilgisi
altta simetrik yerleşir. [Güncel analog önizleme](output/round-classic.png).

Güncel yazılım sonuçları: [ARM/LVGL arayüz testleri](output/verify-round-ui.json)
**105 geçti**, [ekransız Model/ViewModel testleri](output/verify-viewmodels.json)
**43 geçti**; toplam **148 kontrol, 0 başarısızlık**.
Derleme **0 hata, 0 uyarı** ile tamamlandı.
Bu sürüm için hazır binary yeniden üretilmiştir; eski kart doğrulama günlükleri
önceki firmware sürümlerine aittir.

## Ekran görüntüleri ve hazır firmware

- [Yeni yuvarlak arayüzün önizlemesi](output/round-ui-preview.png)
- [Düz hesap tuşları, kapsül ayarlar ve yuvarlak onay düğmesi](output/controls-preview.png)
- [Sabit renkli kadran galerisi ve yeni alarm kartları](output/gallery-preview.png)
- [Saat ve tarih ayarı](output/round-clock-editor.png)
- `Firmware/Smartwatch.bin`: mevcut başarılı Debug derlemesinden üretilen ham firmware;
  yükleme başlangıç adresi `0x08000000` olmalıdır. ELF ile geliştirme için aşağıdaki derleme akışını kullanın.

GitHub sürümü güncel uygulama kaynaklarını, hazır firmware'i, ekran görüntülerini ve
ilgili doğrulama günlüklerini içerir. PDF rehberleri ve belge üretim betikleri yerel tutulur.
`Debug/`, `Backup/`, geçici PDF render dosyaları ve kullanıcı ayarlarını içeren `.bin` yakalamaları
yerel tutulur. README'deki `Backup/` yolları geliştirme bilgisayarındaki yedekleri anlatır.

## Yuvarlak arayüz ve performans — 5 Ekim 2026

- Menü: sekiz adet **92 px** renkli, özgün ikon; yazısız, yuvarlak ekrana göre kavisli düzen.
  Üst üçlünün kenarları aşağıda, alt üçlünün kenarları yukarıdadır; ortada iki ikon bulunur.
  Bütün ikonların dokunma alanı 480 px yuvarlak ekranın içinde kalır.
- Saat: dijital saat tam merkezde, analog kadranın merkezi (240, 240); menü düğmesi kaldırıldı.
  Saat üzerindeki parmak hareketi **24 px** eşiğini aşınca kilit açılır ve menü gelir.
- Saat görünürken tek kısa dokunma paneli kapatır; kapalıyken tek dokunma paneli açar.
  Uyandıran dokunma başka bir işlem çalıştırmaz. Dokunmatik ve RTC açık kalır; bu MCU uyku modu değildir.
- Menüde veya uygulamalarda aynı bölgede **260 ms** içinde çift dokunma saati gösterip kilitler.
  İlk bırakma olayı bu kısa aralıkta bekletilir, basma geri bildirimi hemen görünür.
  Böylece çift dokunma bir hesap tuşunu veya su ekleme işlemini çalıştırmaz.
  Bildirim düğmeleri bu beklemeyi kullanmaz; alarm/su uyarısı kapalı paneli uyandırır.
- Her sayfada aynı transparan başlık alanı vardır; başlık ekranın tam ortasına hizalanır.
  Geri düğmesi kaldırıldı. **Sol 64 px kenardan başlayıp sağa en az 64 px kaydırmak** üst sayfayı açar.
  Ekranın ortasından sürükleme geri dönmez; menüden geri kaydırma saati gösterir.
  Hesap makinesi **68 px** tuşlar, su takibi **88 px** ekleme ve **72 px** geri alma alanları kullanır.
  Alarmlar, notlar, takvim, ayarlar, su geçmişi, saat ayarı ve kadran seçimi de düzenlendi.
- Notlar: **46 px** yuvarlak harf tuşlarıyla A–Z'nin tamamı aynı ekranda, kavisli 6/7/7/6 düzenindedir.
  `A/a` büyük/küçük harfi değiştirir; `123` ve `#+=` rakam/noktalama/satır sonu modlarını açar.
  Kaydetme, boşluk, karakter silme ve iki dokunuşla not silme ayrı kontrollerdir.
- Takvim: yuvarlak gün düğmeleri, ay değiştirme, bugün işareti ve seçilen tarih;
  arayüz seçimi: **140 px** dairesel dört kadran önizlemesi, sabit renkler ve ikiye iki galeri.
  Renk seçici kaldırıldı; eski renk baytı veri uyumluluğu için korunur ve görünüme etki etmez.
  Klasik kadran koyu temada koyu zemin ve beyaz rakam/ibreler kullanır; mavi saniye ibresi ve
  sayısal saniye göstergesi vardır. Neon'un alarm satırı halkanın içinde kalır; AM/PM ve tarih ayrıldı.
  Yeni **Orbit** kadranı saniye halkası, büyük dijital saat ve su kartı içerir.
  Hesap makinesinin **68 px** tuşları dört sütun ve dört düz satırda hizalanır.
  Ayarlar **64 px** yüksekliğinde kapsül satırlar kullanır: ikon, başlık, mevcut değer veya tema anahtarı.
  Saat biçimi ve tema satırın tamamına dokunarak değişir; tarih/saat ve arayüz satırları alt sayfayı açar.
  Arayüz seçimi alt ortadaki **64 px mint ✓** düğmesiyle kaydedilir ve saat ekranına dönülür.
  Kayıt başarısızsa önceki seçim korunur, yeniden deneme simgesi ve hata metni gösterilir.
  Alarm/not listeleri ve su geçmişi ekran çemberine göre hizalanır.
  Alarm listesi **88 px** yüksekliğinde üç kart, **32 px** saat yazısı, tekrar bilgisi ve ayrı aç/kapat alanı kullanır.
  Menüde alarm simgesi zil biçimindedir; saat simgesinden ayrılır.

Eski görüntü yolu, görünen tamponu 40 satırlık parçalarla değiştiriyordu; bu yüzden sayfa yüklenmesi
yukarıdan aşağı açılıyormuş gibi görünüyordu. Yeni yol iki fiziksel GFXMMU tamponu kullanır:
LVGL gizli tampona çizer; son parça tamamlanınca **LTDC dikey boşlukta** tamamlanmış görüntüyü gösterir.
Gizli tampon sonraki kısmi çizim için eşitlenir. Bu 5 Ekim sürümünde sayfalar animasyonsuz geçer;
6 Ekim sürümünün içerik hareketleri yukarıda açıklanmıştır.
Çizim parçalarının ve fiziksel tamponun kopyalanması **DMA2D** ile yapılır; işlem tamamlanmadan
LVGL tamponu yeniden kullanmaz. **ICACHE** etkinleştirildi; flash kaydından sonra önbellek temizlenir.
Debug derlemesi `-O2` kullanır; çizim ve giriş zamanlayıcıları 16 ms'dir.
Değişmeyen metinler yeniden yazılmaz, gizli sayfalardaki saat çizimleri sınırlandırılır.
16 ms zamanlayıcı gerçek 60 FPS garantisi değildir.
`smartwatch_frame_ms`, `smartwatch_frame_max_ms` ve `smartwatch_frame_count` cihaz ölçümü için hazırdır.

**Doğrulama:** derleme hatasız/uyarısız; [ARM/LVGL yazılım testleri](output/verify-round-ui.json)
**100 geçti, 0 başarısızlık**. Önizlemeler yeni derlemenin gerçek LVGL çizimleriyle yerel ARM
emülatöründe üretildi; panel fotoğrafı değildir. Panel komutları ve flash I/O emülatörde taklit edilir;
bu yazılım sonuçları fiziksel LTDC/DSI zamanlamasını doğrulamaz.

Firmware USB/ST-LINK üzerinden karta yüklendi; yükleme doğrulaması başarılı ve uygulama çalışıyor.
[Gerçek kart doğrulaması](output/verify-curved-hardware.json): **32 kontrol geçti**; ekran tamponları
görsel olarak incelendi, karttaki firmware hazır binary ile birebir karşılaştırıldı.
Son koşuda çizim süreleri: alarmlar **60 ms**, menü **61 ms**, hesap **63 ms**, ayarlar/klavye **70 ms**,
su **71 ms**, Klasik **78 ms**, takvim **80 ms**, galeri **110 ms**, Neon **112 ms**, Orbit **132 ms**.
Dikey boşluk ve önbellek durumu süreyi değiştirir. Tam ekran regresyon kontrolü **150 ms** sınırı kullanır.
Süre ilk flush'tan dikey boşlukta gösterim ve tampon eşitlemenin sonuna kadardır;
ilk çizim parçasını veya hareket tanıma süresini içermez. Testler ST-LINK ile dokunma örnekleri enjekte eder;
parmakla kullanım testi veya panel fotoğrafı değildir. Gerçek tampon yakalamaları kullanıcı notlarını
içerebildiği için yerel tutulur. Aynı arayüzde önbellek/DMA2D iyileştirmesi öncesinde menü **235 ms** idi.

Kalıcı kullanıcı yapısı 916 bayt olarak korundu; notlar, alarmlar, su geçmişi, hedef, hatırlatma ve tema
güncelleme öncesi yedekle karşılaştırıldı. Seçili kadran mevcut geçerli flash kaydıyla eşleşiyor.
Son güncelleme öncesi 4 MB yedek `Backup/before-gallery-flash.bin` dosyasındadır.

Yerel testi tekrarlamak için Python ortamına `unicorn`, `pyelftools` ve `Pillow` yükleyin;
`output/verify-round-ui.py` betiği `Debug/Smartwatch.elf` dosyasını çalıştırır.
Gerçek kart kontrolleri `./output/Verify-Curved-Hardware.ps1` ile, yakalamaları inceleme ve veri
karşılaştırması `python output/check-device-captures.py --backup Backup/before-gallery-flash.bin` ile tekrarlanabilir.
Aktif IDE Debug oturumu kapalı olmalıdır.
İkonlar `output/generate-menu-icons.py` ile tekrar üretilebilir; harici ikon fontu/SVG yorumlayıcısı kullanılmaz.
Görsel yaklaşım için [Apple Watch uygulama ızgarası](https://support.apple.com/en-ie/guide/watch/apd3cd8641c2/watchos)
ve [Phosphor iki tonlu ikon ailesi](https://phosphoricons.com/?size=64&weight=duotone) incelendi;
projede kullanılan ikonlar bu proje için çizildi.

## Önceki özellik sürümü — 4 Ekim 2026

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

### Sabit renkli saat arayüzleri

Pusula kaldırıldı; yerine dört saat görünümü bulunur:

- **Pastel:** yumuşak renkler, büyük dijital saat ve su hedefi kartı.
- **Neon:** dijital saat, canlı saniye ve gerçek su hedefi ilerlemesini gösteren dış halka.
- **Klasik:** uygulama temasına göre koyu veya krem kadran, saat/dakika/saniye ibreleri,
  sayısal saniye, tarih ve su miktarı.
- **Orbit:** amber saniye halkası, beyaz dijital saat, tarih, su kartı ve alarm durumu.

Menü → Arayüzler veya Ayarlar → Saat arayüzleri sayfasını açın.
Dört dairesel önizlemeden birini seçin. Paletler sabittir; renk seçimi bulunmaz.
Alttaki **✓** seçimi kaydeder ve saat ekranına döner. Uygulamadan geri çıkmak mevcut seçimi korur.
Seçim yeniden başlatmada korunur. Klasik kadran açık/koyu uygulama temasını takip eder.
Kadran seçimi mevcut ayar yapısındaki baytta saklanır; eski renk baytı korunup görsel olarak yok sayılır.
Mevcut notlar, alarmlar ve su kayıtları korunur. Saat fontu mevcut Montserrat bitmaplerinden türetilmiş
72 px rakam/iki nokta fontudur.

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
Derleme `Firmware/Smartwatch.bin` dosyasını da günceller. Baştan temiz derleme
için `./Build.ps1 -Clean` kullanın.
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

## Önceki sürümlerin cihaz doğrulaması

- Önceki saat ayarı derlemesi: **0 hata, 0 uyarı**. O sürümün boyutları `output/build-clock.log` içinde bulunur.
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

- `App/views/`: ekranlar, dört kadran, ortak widgetlar ve ikon/font varlıkları.
- `App/viewmodels/`: ekran durumları, düzenleme taslakları ve kullanıcı komutları.
- `App/models/`: alarm/su kuralları, veri yapısı, tarih ve hesaplama mantığı.
- `App/services/`: kayıt ve saat servisleri ile donanım sözleşmeleri.
- `App/platform/`: STM32 RTC, flash günlüğü ve LVGL ekran/dokunmatik bağlantısı.
- `App/navigation/`: ekran geçişleri ve dokunma/kilit davranışı.
- `Core/Src/main.c`: donanım başlangıcı ve ana döngü.
- `App/watch_clock.h`: mevcut araçlar için saat servisi uyumluluk başlığı.
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
