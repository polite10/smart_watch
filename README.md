# Smartwatch

STM32U5A9J-DK için STM32CubeIDE projesi. LVGL 9.3.0 kullanır; TouchGFX gerekmez.

## Arayüz ve oyun yenilikleri — 13 Ekim 2026

- Klasik saatte tarih, su ve pil bilgisi ibrelerin üzerinde okunur; ibreler yarı
  saydamdır. Saatten uygulamalara geçiş, parmağın dört yöndeki kaydırmasını izler.
- Uygulama içeriğinin açılışı 64 px hareket ve 440 ms yumuşak geçiş kullanır.
  Çarkın hafif sürüklenmesi ve 720 ms savurma, 340 ms klasör animasyonu korunur.
- Bulmaca taşları daha açık renkler ve kabartma kenarlar kullanır; doğru konum
  yeşildir. Yeni oyun düğmesi kaldırıldı; çıkıp yeniden açmak yeni oyun başlatır.
- Flappy çiçekleri her 2–5 yeni engelde bir, borular arasındaki boş koridorun
  ortasında gelir. Dokunmak bir bonus puan, boruyu geçmek normal puan kazandırır.
  Arılar ayrı koridorlarda, merkez rotanın yanında görünür; temas oyunu bitirir.
  Bulutlar ve iki tepe katmanı farklı hızlarda, başlangıç ekranında da hareket eder.
- Yılanın 2D zemini ışıklı kenarlar, gölge, çalı ve taş ayrıntılarıyla derinlik
  kazanır. Üç böcek başlangıç gövdesi ve yemden ayrı doğar; temas oyunu bitirir.
  Kenardan karşı tarafa geçiş ve küçük alanları yeniden çizme davranışı korunur.
- Uygulamalar çarkındaki saat düğmesinin yerine canlı **DMA / bayt** göstergesi
  geldi. Dokununca tamamlanan aktarım, toplam bayt, hata ve LED durumu açılır.
  Mevcut TIM6 LED yönetimi ve RAM → USART1 TX DMA aktarımı değiştirilmedi:
  hedef sıklık saniyede bir mesaj, 192 bayt tampon, 115200 baud. Mesaj boyu canlı
  sayaçların basamak sayısına göre değişir. Bu bilgi CPU kullanım ölçümü değildir.

Bu yeniliklerde GPU/ekran sürücüsü ayarları değiştirilmedi. Odaklı ARM/UI
kontrolleri [yenilik raporunda](output/verify-refinements.json) bulunur.
Son derleme 0 hata / 0 uyarı; 25 odaklı kontrol ve kısa kart kontrolü geçti.
Kartta tek örnekte Flappy 40 kare / 1021 ms, Yılan 40 kare / 1000 ms verdi;
klasör açılışı 362 ms içinde 8 kare sundu. En uzun kare süreleri sırasıyla
26, 14 ve 37 ms idi. Son DMA mesajı 110 bayt; UART hatası ve GPU zaman aşımı
sıfır. Firmware doğrulanarak yüklendi ve kart saat ekranında çalışır bırakıldı.
Kullanıcı kayıt formatı korunur; günlükteki kayıtların CRC kontrolü geçmiştir.
12 Ekim yedeğindeki bütün kayıtlı yuvalar günlükte aynı içerikle durur;
o yedekten sonra eklenen dört geçerli kayıt nedeniyle ham günlük birebir aynı değildir.
Aşağıdaki performans ve kapsamlı test sonuçları önceki GPU sürümüne aittir.

## GPU ve akıcı animasyonlar — 12 Ekim 2026, önceki sürüm

NeoChrom GPU2D artık LVGL'nin NemaGFX çizim birimi üzerinden çalışır. RGB565A8
görsellerin ayrı alfa düzlemi GPU'da işlenir; DMA2D ekran kopyalarını yürütür.
Flappy'nin bulut, tepe, çiçek ve kelebekleri her karede yeniden rasterize edilmek
yerine GPU'ya hazır görsellerden çizilir. Mevcut klasör tamponu oyunla paylaşılır;
oyundan menüye dönüşte klasör karesi yeniden çözülür. Ekran sunumundan sonra
bekleyen çizimin gereksiz bir 16 ms daha gecikmesi de kaldırıldı.

Aynı STM32U5A9J-DK üzerindeki önce/sonra ölçümü:

| Ölçüm | Önce | Güncel sürüm |
|---|---:|---:|
| Flappy'nin ilk saniyesinde sunulan kare hızı | yaklaşık 14 FPS | 35–36 FPS |
| Klasör açılışında sunulan kare | 2 | 8 |
| Klasör kapanışında sunulan kare | 2 | 9 |
| Klasör açılışındaki en uzun kare süresi | 101 ms | 37–39 ms |

Flappy ve klasör ölçümleri üçer kez tekrarlandı. Çarkın hızlı savurma hareketi
724 ms içinde 28 kare sundu; uygulama girişinde 11 kare ve en fazla 36 ms kare
süresi ölçüldü. Uygulama girişinin ardından durağan ekran da ölçüm penceresine
dahildir; bu satır sürekli oyun FPS'si olarak yorumlanmaz.

Çarkta sürükleme kazancı %10 arttı, dönüş eşiği 4° yerine 3° oldu. Savurma
süresi 620 → 720 ms, tahmini süzülme mesafesi yaklaşık %17 arttı. Uygulama
açılışları 180 → 280 ms, ekran kaymaları 240 → 320 ms, klasör açılış/kapanışı
260 → 340 ms oldu. Klasör hareketi yumuşak hızlanma ve yavaşlama kullanır.

Temiz ARM derlemesi **511 kaynak, 0 hata, 0 uyarı** ile tamamlandı. Aynı son ELF
üzerinde **407 kontrol geçti**: 266 UI/dokunma, 60 Model/ViewModel, 17 Flappy,
24 İDA, 24 çark/ortak tampon ve 16 ekran sürücüsü kontrolü. Son firmware karta
yüklendi, flash doğrulaması geçti ve uygulama çalışır durumda bırakıldı.
GPU zaman aşımı ve UART hata sayacı sıfır; heartbeat ve RTC çalışıyor.
16 KB kullanıcı kayıt günlüğü yükleme öncesi yedekle birebir aynı kaldı.

[Birleşik test raporu](output/verify-interactions.json),
[donanım performans raporu](output/verify-graphics-performance.json),
[temiz derleme raporu](output/build-gpu-smooth.json).
Donanım ölçümleri kare sunum sayaçlarından alınır; görseller kartın gerçek
framebuffer'ından okunmuştur. Fiziksel parmak hissi otomatik olarak ölçülmez.
`output/Verify-Graphics-Performance.ps1` donanım denetimini tekrar çalıştırır;
ardından `output/render-device-graphics.py` yakalanan ekranları PNG'ye dönüştürür.
Aşağıdaki bölümler önceki sürümlerin tarihli notlarıdır.

## Kadran, uygulama çarkı ve aurora — 12 Ekim 2026, önceki sürüm

- **Klasik kadran:** işaretlerin yarıçapı 166 → 218 px oldu. Tarih ve su bilgisi
  kadranın içine alındı; rakamlar, ibreler ve pil etiketi yeni çembere göre hizalandı.
- **Uygulama çarkı:** merkezdeki üç oyun sabit kalır. Dış dokuz ikonu parmakla
  döndürün; hızlı savurma en fazla 620 ms yavaşlayarak devam eder. Son konum
  başlangıçtaki 40° aralıklı yuvalardan biridir; yarıçap ve yerleşim açıları korunur.
  Dönüş sırasında ikon açılmaz. Hareketli çarka dokunmak dönüşü kesip yakındaki
  yuvaya oturtur. Sol kenardan sağa kaydırma geri dönüş olarak önceliklidir.
- **Global geri geçişi:** uygulamalarda, alt sayfalarda ve saat/menü arasında
  240 ms yatay kaymada iki ekran birlikte görünür.
  Geçiş eşik aşıldığında başlar; parmağı birebir takip eden bir önizleme değildir.
  Geçiş sırasında dokunmalar uygulama açmaz. Klasör açıkken geri hareketi önce
  klasörü kapatır. Geri hedefleri tek `watch_navigation_back()` fonksiyonundan
  açılır; yeni sayfa yalnız üst sayfasını tanımlar.
- **Çift dokunma:** yalnız uygulama menüsünün boş alanında, 300 ms ve 32 px
  sınırlarıyla saati açar. Uygulamalarda ve uygulama düğmelerinde kilitleme yoktur;
  örneğin su artı düğmesine iki kez dokunmak iki bardak ekler. Uygulama
  kontrollerinde önceki 120 ms bırakma beklemesi kaldırıldı. Politika tek yerde,
  `App/navigation/watch_navigation.c` içinde yönetilir.
- **Flappy:** bulutlar, iki tepe katmanı, çiçekler ve küçük kelebekler farklı
  hızlarda kayar. Çiçekler ikinci ve üçüncü borunun bir dudağından boşluğa uzanan
  ek engellerdir; görünüm ve çarpışma ortak model geometrisini kullanır.
  Arka plandaki kelebekler ve zemindeki çiçekler dekoratiftir. Duraklatma,
  bildirim, kapalı ekran veya oyundan çıkış parallax hareketini de durdurur.
  Güncel 175 px boru aralığı, 104 px boşluk ve hıza bağlı kuş eğimi korunur.
  [Derlenmiş firmware’den hareketli önizleme](output/flappy-parallax.gif).
- **Ayarlar → Duvar kagidi:** yeşil/turkuaz, mavi/mor ve gül/mor aurora.
  Oklarla önizleyin, ✓ ile kaydedin; geri çıkmak seçimi iptal eder. Kayıt hatası
  eski seçimi korur ve tekrar denemeye izin verir. Önceden görünümü etkilemeyen
  `face_color` baytı kullanılır; 916 bayt kullanıcı verisi ve 928 bayt kayıt
  payload'u değişmez. Eski 0/1/2 değerleri aynı indeksli auroraya karşılık gelir.

[Görsel önizleme](output/interaction-preview.png), [geçişin ara karesi](output/round-home-slide.png),
[üç aurora örneği](output/aurora-preview.png), [görsel kaynakları ve üretim istemleri](output/wallpapers/README.md).

Klasör hareketi, duvar kâğıdı ve çark konumundan bağımsız **17 şeffaf kare**
kullanır. 260 ms boyunca yaklaşık her 16 ms için bir kare vardır. LZ4 ile
saklanan kareler tek 270 KB RGB565A8 tampona açılır; ikonlar çalışma anında
ölçeklenmez. Eski duvar kâğıdının animasyonda belirmesi engellenir. Bu sürüm,
GitHub’daki kenardan geçen Yılan, canlı İDA ve DMA2D çizim iyileştirmelerini korur.

ARM derlemesi: **506 kaynak, 0 uyarı**. Aynı ELF üzerinde **266 UI/dokunma**,
**60 Model/ViewModel**, **17 Flappy** ve **24 İDA** kontrolü geçti
(**367 kontrol, 0 başarısızlık**). [Birleşik rapor](output/verify-interactions.json).
Bağımsız C testi oyun kurallarını, 512 Yılan başlangıcını ve yeni çiçeklerin
çarpışma/güvenli geçişini doğruladı. `Firmware/Smartwatch.bin` bu ELF ile eşleşir;
binary yaklaşık 4,01 MB'dir ve ayrılmış flash kayıt alanına taşmaz.
Bağlı ST-LINK bulunamadığı için bu sürüm **fiziksel karta yüklenmedi**;
donanım FPS ve parmak hissi ölçülmedi.
Simülatör önizlemeleri panel fotoğrafı değildir. Aşağıdaki tarihli bölümler önceki
sürümlerin davranışını ve test sonuçlarını anlatır.

## Etkileşimli İDA simülasyonu — 11 Ekim 2026

İDA'nın mevcut radar görünümü artık dokunmaya tepki verir. **SIM** her zaman
görünür; konum, yön, hız, radar temasları ve hedef mesafesi yazılım simülasyonudur.
Saat mevcut RTC'den gelir; saat geçersizken `--:--` gösterilir. Sabit örnek pil
yüzdesi ve bağlantı metni kaldırıldı; alt sırada **HIZ / HEDEF / GOREV** bulunur.

- Radarın içine dokunarak W1 hedefi seçilir. Merkezdeki 8 px nötr bölge ve
  radar dairesi dışındaki dokunuşlar hedef oluşturmaz. Aktif görevde yeni hedef
  seçmek rotayı değiştirir ve hareket devam eder.
- **HIZ** alanı `− / +` panelini açar. Hız 0–20 kn arasında 0,5 kn adımlarla
  ayarlanır. Sıfır hızda araç durur ve varış süresi `--` olur.
- **HEDEF** paneli mesafe, yön ve tahmini varış süresini gösterir; hedef
  kaldırılabilir. Hız, mesafe ve ETA aynı konum hesabından üretilir.
- **GOREV** panelinden başlat, duraklat, devam ve sıfırla seçilir. Hedefe varınca
  araç tam hedef konumunda durur ve `HEDEFE VARILDI` mesajı görünür. Sıfırlama
  hedefi, konumu ve izi temizler; saat korunur.
- **Menzil** alanına dokunarak 600 → 1200 → 200 → 600 m ölçeği seçilir. Bu,
  dünya koordinatlarını değiştirmez. Menzil dışındaki hedef çemberin kenarında
  `W1 >` ile gösterilir.

Radar araç merkezlidir: araç oku merkezde kalır, hedef ve son 32 konumdan oluşan
iz hareket eder; ok hedefin yönüne döner. Tarama dört saniyede bir tur atar;
iki kurmaca temas tarama geçtikten sonra solar. Kıyı dokusu dekoratiftir;
gerçek harita veya engel algılama değildir. Paneller aynı ekranın üzerinde
açılır ve **Radara don** ile kapanır; menüye sol kenardan sağa kaydırılır.

İDA güncellemesi 50 ms LVGL timer'ından çalışır; uzun gecikmeler 200 ms ile
sınırlanır. Kapalı ekranda veya bildirim üzerinde simülasyon ilerlemez; ekrandan
çıkmak görevi duraklatır. Dönüşte **Devam** ile sürdürülür. Görev RAM'dedir;
yeniden açılışta başlangıç durumuna döner ve flash'a görev kaydı yazılmaz.

Derleme: `./Build.ps1`. Doğrulama: `python output/verify-ida.py`,
`python output/verify-round-ui.py` ve `python output/verify-viewmodels.py`.
Emülasyon için `unicorn`, `pyelftools` ve `Pillow` gerekir.
[İDA model raporu](output/verify-ida.json),
[arayüz raporu](output/verify-round-ui.json),
[ViewModel raporu](output/verify-viewmodels.json),
[derleme raporu](output/build-ida.json).
Kontroller derlenmiş ARM kodunu çalıştırır; fiziksel kartta FPS/dokunma ölçümü
yapılmadı ve bu değişiklik kapsamında karta firmware yüklenmedi.

[Radar](output/round-ida.png), [hız paneli](output/round-ida-speed.png),
[görev paneli](output/round-ida-task.png), [hedef paneli](output/round-ida-target.png)
gerçek LVGL çizimlerinin emülatör yakalamalarıdır.
Arka planı diğer uygulama varlıklarına dokunmadan üretmek için
`python output/generate-arcade-art.py --ida-only` kullanılabilir.

## Klasör animasyonu ve kenarlardan geçen Yılan — 11 Ekim 2026

Oyun klasörünün açılış/kapanışında 33 önceden çizilmiş kare kullanılır. Opak
önbellek, alttaki ikonların her karede tekrar çizilmesini önler; RGB565 görüntüler
DMA2D ile ekran tamponuna dönüştürülür. Hareketin hedef süresi 260 ms'dir.
STM32U5A9J-DK üzerinde aynı açılış/kapanış yolu için alınan ölçüm:

| Ölçüm | Önce | Güncel firmware |
| --- | --- | --- |
| Açılışta gösterilen kare | 2 | 11 |
| Açılış süresi | 400 ms | 297 ms |
| Açılışta en uzun flush/sunma | 101 ms | 26 ms |
| Kapanışta gösterilen kare | 2 | 11 |
| Kapanış süresi | 318 ms | 274 ms |
| Kapanışta en uzun flush/sunma | 124 ms | 28 ms |

Bunlar firmware sayaçlarıyla bir açılış/kapanışta ölçülen değerlerdir;
ekranın kamera ile ölçülmüş FPS değeri değildir. Donanım testi için
`./output/Verify-Folder-Performance.ps1` çalıştırılır. Test sırasında dokunma
okuma kısa süre durdurulur; bitişte saat ekranına dönülür ve dokunma açılır.
[Donanım ölçüm raporu](output/verify-folder-performance.json).

Yılanın ızgarası kaldırıldı. Dört kenardan geçen baş ve gövde, hareket sırasında
karşı kenarda görünür; duvara ulaşmak oyunu bitirmez. Yalnız kendi gövdesine
çarpması kaybettirir. Kuyruk hücresinin boşalması, büyüme, yön kontrolü ve
160–80 ms hücre adımları / 20 ms görüntü güncellemesi korunur.
[Yılan önizlemesi](output/round-snake.png).

Son derleme **0 hata, 0 uyarı** ile tamamlandı. Derlenmiş ARM/LVGL üzerinde
**323 kontrol, 0 başarısızlık** ve bağımsız ARM C oyun testleri (512 başlangıç
tahtası ve dört kenar geçişleri dahil) geçti. Firmware karta yüklendi ve flash
doğrulaması tamamlandı; saat ve UART çalışıyor. Kullanıcı ayarları ile oyun
rekorlarını içeren flash günlüğü yükleme öncesiyle byte byte aynı kaldı.
[Derleme raporu](output/build-smooth-folder.json),
[test özeti](output/verify-games.json),
[bağımsız oyun modeli testi](output/verify-arcade-model.json).

## Buton, timer kesmesi ve UART DMA uygulaması — 11 Ekim 2026

Saatle birlikte çalışan uygulama `Core/Src/led_button_demo.c` içindedir.
Kartın **mavi B1 USER butonu (PC13)** ve **yeşil LD3 LED'i (PE0)** kullanılır.
Kırmızı LD4 saatin alarm uyarısını göstermeye devam eder. Ek kablo gerekmez.

- Başlangıçta LD3 sönüktür; her **1000 ms**'de durum değiştirir.
  Her B1 basışında aralık **1000 ms → kapalı → 100 → 250 → 500 → 1000 ms** olur.
  Böylece hız en yavaş kademeden sonra tamamen kapanır; başlangıçtan ilk basış kapatır.
  Kapalı modda LD3 hemen söndürülür ve sonraki basışa kadar sönük kalır.
  Buton algılama, saat ve UART raporları kapalı modda çalışmaya devam eder.
  Bu süre tek durum değiştirme aralığıdır; tam yanıp sönme çevrimi bunun iki katıdır.
- **EXTI13 GPIO kesmesi** basış ve bırakış kenarlarını kaydeder.
  **TIM6 kesmesi**, son kenardan sonra seviye 40 ms kararlı kaldığında basışı kabul eder.
  Basılı tutma tek basış sayılır; yeniden basış için kararlı bırakış gerekir.
  Açılışta basılı duran buton bırakılıp yeniden basılmadan hız değiştirmez.
- **TIM6**, 10 ms'de bir kesme üretir; LED yalnız timer callback'inde değiştirilir.
  160 MHz timer saatinde prescaler **15999**, auto-reload **99** olur.
  LED zamanlaması ana döngünün veya ekran çiziminin hızına bağlı değildir.
- Aynı timer her **1 saniyede** UART durum mesajını hazır bekler hale getirir.
  Ana döngü mesajı oluşturur ve `HAL_UART_Transmit_DMA()` ile gönderir.
  **USART1 TX (PA9 / AF7)** ve **GPDMA1 Channel 0 / USART1_TX isteği** kullanılır.
  EXTI13/TIM6 kesme önceliği 6, DMA/USART1 önceliği 7'dir.

ST-LINK USB **CN5** üzerinden bağlanın ve kartın sanal seri portunu
**115200 baud, 8 bit, parity yok, 1 stop bit, akış kontrolü yok (8N1)** ile açın.
macOS'ta port genellikle `/dev/cu.usbmodem…`, Windows'ta `COM…` olarak görünür.
Örnek mesaj:

```text
LED=LD3 mode=BLINK state=ON toggle_ms=500 cycle_ms=1000 presses=4 uptime_ms=2000 uart_ok=1 uart_errors=0
```

Sürekli kapalı mod `mode=OFF state=OFF toggle_ms=0 cycle_ms=0` olarak bildirilir.
Yanıp sönmenin sönük fazı ise `mode=BLINK state=OFF` olduğundan ayırt edilebilir.

`uart_ok` tamamlanmış gönderim, `uart_errors` hata/zaman aşımı sayısıdır.
DMA normal modda çalışır; SRAM mesaj tamponu son byte'ın **USART1 TC kesmesi**
gelene kadar değiştirilmez. Hem GPDMA hem USART1 IRQ handler'ı gereklidir.
250 ms'de tamamlanmayan aktarım veya DMA hatası ana döngüde durdurulup toparlanır.
Ana döngü gecikirse bekleyen raporlar tek güncel mesajda birleştirilir.
Kesme içinde mesaj biçimlendirme veya `HAL_Delay()` yapılmaz.

Derleme: `python3 output/build-macos.py` veya Windows'ta `./Build.ps1`.
Firmware `Firmware/Smartwatch.bin`, ELF `Debug/Smartwatch.elf` olarak üretilir.
ARM simülasyon testi: `python3 output/verify-led-button.py`
(`unicorn` ve `pyelftools` gerekir; macOS sandbox dışında bellek eşleme izni gerekebilir).
Test derlenmiş uygulama ile gerçek HAL EXTI/TIM6/GPDMA/UART kesme yollarını çalıştırır;
periferik register'ları, DMA veri hareketi ve UART hattı simüle edilir.
ARM derlemesi **505 kaynak, 0 hata, 0 uyarı** ile tamamlandı;
LED/buton/UART simülasyonunda **41 kontrol, 0 başarısızlık**:
[derleme raporu](output/build-led-button.json), [test raporu](output/verify-led-button.json).
Bu önceki raporda fiziksel kart doğrulaması yapılmamıştı; güncel firmware'in
karta yükleme ve çalışma doğrulaması yukarıdaki bölümde yer alır.

### İlk öneriyle farkı

İlk öneride LED, ana döngüde `HAL_GetTick()` ile süre kontrol edilerek değiştirilirdi.
Bu da `HAL_Delay()` ile beklemeden çalışır; ancak ana döngü ekran çizimi veya başka
bir işlemle meşgulse LED değişimi gecikir. Buton için sonraki açıklamada zaten
EXTI kesmesi önerilmişti; bayrak ve debounce ana döngüde işlenecekti.

| Konu | İlk öneri | Uygulanan timer / EXTI / DMA yöntemi |
| --- | --- | --- |
| LED | Ana döngü `HAL_GetTick()` ile kontrol eder | TIM6 kesmesi LED'i değiştirir |
| Buton | EXTI bayrağı, ana döngüde debounce | EXTI iki kenarı kaydeder, TIM6 debounce yapar |
| UART | Öneride durum gönderimi yoktu | Her saniye USART1 üzerinden DMA ile rapor |
| Kurulum | Daha az periferik ve daha basit kod | Timer, DMA, UART ve dört IRQ yolu gerekir |
| İşlemci işi | Ana döngüde süre/buton kontrolü | 100 Hz timer IRQ; UART byte aktarımını DMA yapar |

Bu projede timer yöntemi, ekranla birlikte çalışan LED'in ana döngü yükünden
etkilenmesini azaltır. Kesme gecikmesi veya kesmelerin kapatılması yine zamanlamayı
etkileyebilir; bu bir donanım PWM çıkışı değildir. DMA mesajı byte byte işlemciden
göndermeden taşır; mesaj biçimlendirme, DMA başlatma ve tamamlanma işlemleri CPU'da
kalır. Saniyede bir kısa mesaj için performans kazancı küçük olabilir, fakat ana
döngüyü UART gönderimi bitene kadar bekletmez.

## Kontroller, akıcılık ve oyun rekorları — 11 Ekim 2026

Yılan doğrudan oyun tahtasıyla açılır; ilk dokunma oyunu başlatır.
Kaydırma ve kısa dokunma birlikte çalışır: ekranın sağ/sol/üst/alt bölgesine
dokunmak ilgili yönü seçer; merkezdeki 32 px alan yön değiştirmez.
Kaydırmanın bırakılması ek bir yön komutuna dönüşmez. Ters dönüş ve aynı hücre
adımında ikinci dönüş engeli korunur. Sensör gerektiren eğerek kontrol, kontrol
seçim ekranı ve sensör sürücüsü kaldırıldı; oyun yalnız dokunmayla kontrol edilir.

Akıcılık ve geri bildirim değişiklikleri:

- Kapalı klasörde ve büyüme sırasında oyun ikonları dokunma hedefi değildir;
  basma efekti oluşturmaz. Bu dokunuşu klasör karşılar. Yalnız tamamen açık
  klasörde oyun seçilebilir. Güncel 260 ms büyümede birleşik kare önbelleği
  kullanılır; hareketsiz ikonlar önceden çizilmiş RGB565A8 görsellerden seçilir.
- Genel sayfa girişleri 180 ms / 24 px, düğme renk/basma geçişleri 100 ms oldu.
  Menü ve aktif oyun açılışları anında kalır. Bulmaca taşları ölçek katmanı
  oluşturmadan renk geri bildirimi verir; 170 ms kayma ilk bırakmada başlar.
  Bulmaca, menü ve oyunlar 120 ms çift dokunma beklemesini kullanmaz.
- Yılanın 160–80 ms hücre fiziği korunur; görüntü hücre adımları arasında
  20 ms timer'dan yumuşatılır. Tahta yalnız başlangıçta tamamen hazırlanır;
  sonraki çizimler eski/yeni gövde ve yem bölgelerini yeniler. Sabit kare yeniden
  çizilmez. Flappy'nin kuşu her adımda yazılımla döndürülmez.
- Ekranın son parçasından sonra LTDC dikey boşluğu ana döngüde beklenmez.
  Tampon sahipliği reload tamamlanınca ana döngüde bırakılır; bekleyen kare varken
  yeni çizim başlamaz. Tampon eşitlemesi değişen satır aralıklarını kopyalar;
  büyük değişikliklerde mevcut 720 KiB fiziksel kopya kullanılır. Ana döngüdeki
  bekleme 1 ms oldu. Gizli uygulamalar/takvim her saat güncellemesinde yenilenmez.
- Flappy boru çiftleri arası 190 → **160 px**, geçiş boşluğu 128 → **112 px** oldu.
  Çizim ve çarpışma aynı sabitleri kullanır; uçuş hızı korunur.

Yılan ve Flappy **en yüksek puanı**, 15 Bulmaca **tamamlanan oyunlardaki en az hamleyi**
gösterir. Farklı bulmaca başlangıçlarının zorluğu değişir; hamle rekoru kişisel
istatistiktir. Rekor RAM'de anında güncellenir; oyun bitişi, duraklatma, yeniden
başlatma veya çıkışta kalıcı kayda geçirilir. Her puanda flash yazılmaz. Başarısız
kayıt RAM'deki sonucu korur ve yeniden denenebilir.

Eski kullanıcı yapısı **916 bayt / sürüm 1** olarak korunur. Aynı iki sayfalı
CRC günlüğünün yeni payload'u kullanıcı verisine 12 bayt rekor ekler (**928 bayt**).
Eski 916 bayt kayıt okunur; yeni rekorlar sıfırdan başlar. Yeni kayıtta ayar ve
rekor birlikte, commit header en son yazılarak saklanır. Bu geçiş notları, alarmları
ve su geçmişini korur. Eski firmware'e geri dönüş yeni payload'u okuyamaz;
güncellemeden önceki yedek eski sürüme dönmek için korunmalıdır.

Yeni önizlemeler: [oyun klasörü](output/round-games-folder.png), [Yılan](output/round-snake.png),
[Flappy](output/round-flappy.png), [Bulmaca rekoru](output/round-puzzle-won.png).

Doğrulama komutları: `python3 output/build-macos.py --clean`,
`python3 output/verify-round-ui.py`, `python3 output/verify-viewmodels.py`,
`python3 output/verify-game-storage.py`, `python3 output/verify-game-platform.py`.
Native oyun testi için:

```sh
cc -std=c11 -Wall -Wextra -Werror -IApp output/test-arcade.c \
  App/models/watch_arcade.c App/models/watch_records.c \
  App/services/watch_records_service.c App/viewmodels/watch_games_vm.c \
  App/viewmodels/watch_arcade_vm.c App/viewmodels/watch_ida_vm.c -o /tmp/watch-test-arcade
/tmp/watch-test-arcade
```

Güncel ARM derlemesi **508 bağlı nesne, 0 hata, 0 uyarı** ile tamamlandı. Son firmware
üzerinde **202 arayüz/dokunma**, **54 Model/ViewModel**, **11 flash kayıt**,
**15 ekran adaptörü** ve **41 LED/buton/UART** kontrolü geçti:
toplam **323 kontrol, 0 başarısızlık**.
Bağımsız C testi 512 başlangıç tahtasını, oyun kurallarını, rekorları
ve değişen satır aralıklarını doğruladı. Birleşik rapor: [verify-games.json](output/verify-games.json).

ARM/LVGL ve platform testleri gerçek derlenmiş kodu çalıştırır; DMA, LTDC, flash
programlama donanım erişimleri taklit edilir. Fiziksel kart FPS'i, panel
zamanlaması ve gerçek güç kesintisi bu simülasyonla ölçülmez. Güncel firmware
karta yüklendi; klasörün donanım sayaçlarıyla ölçümü ilk bölümde yer alır.
Aşağıdaki tarihli bölümlerde önceki sürümlerin test sonuçları da korunur.

## Yuvarlak oyun klasörü, Flappy Bird, Yılan ve İDA — 6 Ekim 2026

Menüde **12 uygulama** bulunur: alarm, hesap makinesi, notlar, kadranlar, takvim,
ayarlar, saat, su ve İDA dış çemberde; 15 Bulmaca, Flappy Bird ve Yılan ortadaki
**yuvarlak yarı saydam klasördedir**. Dış ikonların dokunma çapı 80 px, merkezleri
178 px yarıçapındadır. Menüde uygulama adı veya appbar görünmez; diğer ekranların
appbarları da gizlidir. Soldaki kenardan sağa kaydırmak önce klasörü kapatır,
ardından bir üst ekrana döner.

Klasöre dokununca çapı **190 → 300 px**, oyun ikonları **64 → 88 px** olur.
Güncel **260 ms** büyüme tamamlandıktan sonra oyunlar seçilebilir. Açık klasörün dışındaki
ikonlar soluklaşır ve geçici olarak tıklanamaz. Boş alana veya klasöre dokunmak
kapatır; bir oyun açıldığında menü eski boyutuna döner.

- **Flappy Bird:** dokunma başladığında kuş yükselir. Boruların arasından geçmek
  puan kazandırır; boruya veya üst/alt sınıra çarpmak oyunu bitirir. Tekrar dokunmak
  yeni oyun başlatır. Fizik hesabı 20 ms adımlıdır; art arda dokunuşlar zamanı
  sıfırlamaz. Üstte puan ve duraklat/devam düğmesi vardır.
- **Yılan:** parmağı ekranda yukarı, aşağı, sağa veya sola sürüklemek baskın yönde
  döndürür. 12 px altındaki hareketler süzülür; anlık ters dönüş ve aynı hücre
  adımındaki ikinci dönüş engellenir. Yön bir sonraki hücre adımında uygulanır.
  18 × 18 tahta, yemle büyüme, gövde çarpışması, puan ve duraklatma bulunur.
  Güncel sürümde duvar yerine karşı kenara geçiş uygulanır; zemin ızgarasızdır.
  İlk dokunuş oyunu başlatır; oyun bitince tekrar dokunmak yeniden başlatır.
- **15 Bulmaca:** mevcut çözülebilir 4 × 4 tahta, hamle sayacı, 140 ms taş kayması
  ve yeni oyun işlevleri korunur.
- **İDA:** pusula, menzil halkaları, rota/hedef, radar izi ve alt telemetri içeren
  konsoldur. **SIM** işareti görünür. Saat, yön, hız, hedef mesafesi, pil, menzil,
  bağlantı ve radar görüntüsü tamamen statik örneklerdir; sensör veya canlı veri
  bağlantısı kullanılmaz.

Oyunlar menüden her açıldığında yeni başlar. Oyun ekranından çıkmak çalışan oyunu
duraklatır; kapalı ekranda veya bildirim üzerinde fizik ilerlemez. Oyun durumu
RAM'dedir; **916 baytlık** kalıcı kullanıcı kaydı değişmez.

Dokunma ve geçiş süreleri:

| Davranış | Önce | Şimdi |
|---|---:|---:|
| Menü, oyun klasörü ve oyun düğmelerinde ek bırakma beklemesi | 260 ms | 0 ms |
| Flappy yükselme / Yılan ilk temas | Bırakma akışı | Dokunma başlangıcı |
| Diğer uygulamalarda çift dokunma ayırt etme | 260 ms | 120 ms |
| Genel içerik giriş hareketi | 140 ms / 18 px | 70 ms / 10 px |
| Küçük buton/kart geçişi | 90 ms | 50 ms |
| Menü, Flappy, Yılan ve İDA giriş hareketi | Genel geçiş | Anında yükleme |

0 ms burada **ek yazılım beklemesi olmaması** anlamındadır; dokunmatik örnekleme,
LVGL çizimi ve panel taraması yine süre alır. Menü ve oyunlarda çift dokunarak
kilitleme uygulanmaz; diğer uygulamalarda 120 ms penceresiyle korunur. Saatten
kaydırarak menüyü açma ve tek dokunarak paneli kapatma davranışları sürer.

Sabit arka planlar flash'ta RGB565, kuş ARGB8888 tutulur. Yılanın 288 × 288 RGB565
çizim tamponu statik RAM'dedir. Var olan LVGL yazılım çizimi ve **DMA2D kopyalama**
yolu kullanılır. GPU2D/NemaGFX entegrasyonu eklenmedi; bu depoda gerekli NemaGFX
SDK/kütüphanesi bulunmuyor. Bu sürüm için kartta FPS veya uçtan uca gecikme ölçülmedi.

[Gerçek ARM/LVGL ekran önizlemesi](output/arcade-preview.png),
[Flappy Bird](output/round-flappy.png), [Yılan](output/round-snake.png) ve
[İDA](output/round-ida.png) derlenmiş firmware'in emülatörde çizdiği ekranlardır.
`output/verify-round-ui.json` ve `output/verify-viewmodels.json` güncel doğrulama
raporlarıdır. macOS'ta `python3 output/build-macos.py --clean`, Windows'ta
`./Build.ps1 -Clean` derler ve eşleşen `Firmware/Smartwatch.bin` üretir.
Temiz ARM derlemesi **0 hata, 0 uyarı** ile tamamlandı. Aynı ELF üzerinde
**167 arayüz/dokunma**, **54 Model/ViewModel** kontrolü geçti: toplam
**221**, başarısız **0**. Bağımsız C oyun testi 512 başlangıç tahtasını,
oyun kurallarını ve duraklatma/yeniden başlatmayı da doğruladı.
Bu sürüm fiziksel karta yüklenmedi.

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
