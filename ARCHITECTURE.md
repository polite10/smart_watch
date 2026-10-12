# Smartwatch — C/LVGL MVVM mimarisi

Uygulama, tek ana döngü üzerinde çalışan hafif bir MVVM düzeni kullanır. Model ve
ViewModel katmanları LVGL, HAL veya ekran nesnelerine bağımlı değildir. Widget
oluşturma, yerleşim, renkler ve çizim yalnız View katmanındadır. RTOS eklenmemiştir.

## Katmanlar

| Klasör | Sorumluluk |
|---|---|
| `App/models/` | Kalıcı veri, doğrulama, alarm/su kuralları, tarih hesapları, hesap makinesi ayrıştırıcısı |
| `App/viewmodels/` | Ekran durumu, düzenleme taslakları, kullanıcı komutları, ekrana hazır metinler |
| `App/views/` | LVGL ekranları, ortak widget yardımcıları, kadran çizimi, ikon/font varlıkları |
| `App/services/` | Kalıcı kayıt ve saat servisi sözleşmeleri, saat değişimi bildirimi |
| `App/platform/` | STM32 RTC, flash günlüğü, DMA2D/LTDC ekran ve dokunmatik bağlantıları |
| `App/navigation/` | Sayfa geçişleri, kenar kaydırması, çift dokunma, kilit/ekran açık-kapalı davranışı |
| `Core/Src/main.c` | Donanımı başlatır, uygulamayı bağlar, RTC/LVGL/LED ana döngüsünü çalıştırır |

Bağımlılık yönü: **View → ViewModel → Model/Servis**. Platform, servislerin donanım
işlevlerini uygular. Saat servisi bir callback ile uygulama ViewModel'ine sonucu
bildirir; RTC sürücüsü ekran açmaz veya widget değiştirmez. Ekran/dokunmatik
adaptörü, LVGL'nin donanım bağlantısı olduğundan LVGL arayüzlerini kullanır.

## Ekran ve ViewModel eşleşmeleri

| Özellik | View | ViewModel |
|---|---|---|
| Kadranlar ve seçim | `watch_faces_view.c` | `watch_faces_vm.c` |
| Alarm listesi/düzenleme | `watch_alarm_view.c` | `watch_alarm_vm.c` |
| Not listesi/düzenleme | `watch_notes_view.c` | `watch_notes_vm.c` |
| Su, hedef, geçmiş | `watch_water_view.c` | `watch_water_vm.c` |
| Hesap makinesi | `watch_calculator_view.c` | `watch_calculator_vm.c` |
| Takvim | `watch_calendar_view.c` | `watch_calendar_vm.c` |
| 15 Bulmaca | `watch_puzzle_view.c` | `watch_puzzle_vm.c` |
| Flappy Bird ve Yılan | `watch_arcade_view.c` | `watch_arcade_vm.c` |
| Etkileşimli İDA simülasyonu | `watch_ida_view.c` | `watch_ida_vm.c` |
| Ayarlar | `watch_settings_view.c` | `watch_settings_vm.c` |
| Saat/tarih düzenleme | `watch_clock_view.c` | `watch_clock_vm.c` |
| Alarm/su bildirimleri | `watch_notification_view.c` | `watch_notification_vm.c` |
| Uygulama zamanı ve kayıt durumu | `watch_ui.c` | `watch_app_vm.c` |

Mevcut ViewModel sözleşmeleri `App/viewmodels/watch_viewmodels.h`, yeni oyun ve
İDA sözleşmeleri `watch_arcade_vm.h` ve `watch_ida_vm.h` içindedir. Ekranlar,
salt okunur durum getter'larını ve komut fonksiyonlarını kullanır. Widget
adresleri ViewModel'e verilmez. Modelin verisi de salt okunur getter ile sunulur;
değişiklikler model fonksiyonları üzerinden yapılır. `watch_data` sembolü mevcut
debug/test araçlarının uyumluluğu için korunur, uygulama başlığında dışarı açılmaz.

## Güncelleme ve komut akışı

Su ekleme örneği:

1. LVGL buton callback'i `watch_water_vm_add(1)` çağırır.
2. ViewModel saat geçerliliğini denetler; model bir bardak ekler ve sınırları uygular.
3. ViewModel kayıt servisini çağırır; sonuç uygulamanın `storage_ok` durumuna yazılır.
4. View, `watch_water_vm_state()` sonucuyla metinleri ve halkayı günceller.
5. Bildirim ViewModel'i yeni alarm/su uyarısı durumunu sağlar.

Veri bağlama bu sürümde açık komut/güncelleme çağrılarıyla ve zaman değişimlerinde
`watch_vm_observer_t` callback'iyle yapılır. Böylece ViewModel testleri LVGL
başlatmadan çalışabilir. LVGL Subject/Observer altyapısı açık olsa da yeni yapının
çalışması için kullanılmasına gerek yoktur. Getter'dan dönen yapılar başka bir
komut çağrısında değişebileceğinden View bunları aynı döngü içinde tüketir.

Not, alarm, kadran ve saat düzenleme durumları kendi ViewModel'lerinde tutulur.
Kaydetmeden geri dönmek model değişikliğine yol açmaz. Not metninin değişmesi
silme onayını sıfırlar. Başarısız not kaydında düzenleme ekranı açık kalır.
Başarısız kadran kaydında önceki aktif seçim geri yüklenir ve taslak tekrar
denemek için korunur. Diğer kayıt hatalarında RAM'deki güncel değer ve genel
kayıt hata bildirimi korunur; işlemler tüm uygulama için atomik bir transaction
olarak uygulanmaz.

İDA ViewModel'i sensör kullanmadan metre cinsinden araç/hedef konumunu,
0–20 kn hızı, pusula açısını, mesafeyi, ETA'yı ve en fazla 32 iz noktasını
hesaplar. Radar pikselini mevcut menzile göre dünya koordinatına çevirir;
menzil değişimi konumu değiştirmez. Varış hedefe tam olarak oturtulur.
View 50 ms timer'ından yalnız görünür ve açık ekranda geçen süreyi iletir;
bildirimler hareketi dondurur, ekran çıkışı görevi duraklatır. RTC metni ortak
saat/geçerlilik akışından beslenir. Sabit arka plan yalnız halkalar ve kurmaca
kıyı dokusudur; tarama, temaslar, rota ve araç ayrı LVGL çizim katmanındadır.
Kontroller aynı ekrandaki panelleri açar. Görev durumu RAM'de kalır ve kullanıcı
flash kayıt formatını değiştirmez.

## Donanım ve kayıt uyumluluğu

15 Bulmaca modelinin `watch_puzzle_state_t` durumu RAM'dedir. Model, karıştırma,
çözülebilirlik paritesi, komşuluk, hamle sayımı ve kazanma kurallarını uygular.
ViewModel her açılışta farklı bir oyun üretir; LVGL tick bilgisi View'den sayısal
tohum olarak gelir. ViewModel HAL veya LVGL'ye bağımlı değildir. View sayıya ait
widget'ı yeni hücresine kaydırır ve hareket bitince durum metnini yeniler.
`watch_navigation_show` ortak içerik giriş hareketlerini ve yarıda kalan geçişleri
yönetir; dokunma filtrelemesi hedef ekranı hemen görür. Bu özellik flash veri
yapısını veya kullanıcı notlarını değiştirmez.

- Flash veri yapısı **916 bayt**, sürümü **1** olarak korunmuştur; derleme zamanı
  kontrolü bu boyutun değişmesini engeller.
- İki sayfalı CRC/sıra numaralı flash günlüğü ve ayrılmış flash adresleri korunur.
- `watch_ui_*`, `watch_clock_set_datetime`, `refresh_rtc` ve donanım/debug sembolleri
  mevcut testler ve `Sync-Time.ps1` için korunur. Kökteki küçük başlıklar uyumluluk
  girişleridir; yeni kod ilgili katmanın başlığını kullanmalıdır.
- Ana döngü ve bildirim öncelikleri korunur. Uygulama kontrolleri bırakma beklemesi
  kullanmaz; yalnız menüdeki boş alanda 300 ms çift dokunma tanınır.
- İkon bitmapleri ortak bir `.c` dosyasından sunulur; menü ve ayarlar aynı
  bitmapleri kullanır.

Analog kadranın ibre merkezi ve işaretleri **(240, 240)** etrafındadır. Rakamların
merkezleri eşit yarıçapta hizalanır. Tarih ve su bilgisi merkezin üstüne/altına
simetrik yerleşir. Analog kadranda dijital saniye etiketi yoktur; saniye ibresi
ve diğer dijital kadranların saniye göstergeleri çalışmaya devam eder.

## Oyun klasörü ve yeni ekranlar

`watch_menu_view.c` dokuz uygulamayı dış çembere, üç oyunu merkezdeki yuvarlak
klasöre yerleştirir. 260 ms büyüme sırasında oyun hedefleri tıklanamaz; hareket
bittiğinde etkinleşir. Navigasyon menüden ayrılırken klasörü sıfırlar. Appbar
etiketleri gizli metadata olarak tutulur; mevcut sayfa çocuk indeksleri ve geri
hedefleri korunur.

`watch_arcade.c` saf C oyun kurallarını, `watch_arcade_vm.c` RAM durumlarını
barındırır. Oyun View'i 20 ms LVGL timer'ından geçen zamanı toplar. Flappy 20 ms,
Yılan puana bağlı 160–80 ms sabit adımla ilerler; render her timer çağrısında en
çok bir kez yapılır. Debugger duruşundan dönüşte yakalama süresi 160 ms ile
sınırlıdır. Dokunma sırasında oyun saati yalnız başlatma/yeniden başlatmada
sıfırlanır. Yılan her adımda tek dönüş alır; baskın sürükleme yönü modele iletilir.
Menüye çıkış oyunu duraklatır. Bildirim, kapalı panel ve görünmeyen oyun ekranı
fiziği ilerletmez.

`watch_ida_vm.c` salt okunur statik örnek veri sunar. Radar, rota ve harita da
statik görseldir; SIM etiketi gerçek telemetri olmadığı bilgisini verir.
`watch_art.c` flash RGB565 arka planları ve ARGB8888 kuş içerir;
`output/generate-arcade-art.py` ve `output/generate-menu-icons.py` varlıkları üretir.
GPU2D/NemaGFX sürücüsü eklenmedi; var olan yazılım çizimi/DMA2D kopyası kullanılır.

## Doğrulama

`Build.ps1` CubeIDE ile tüm alt klasörleri derler ve eşleşen `.bin` dosyasını ARM
objcopy ile `Debug/Smartwatch.elf` dosyasından otomatik üretir. `-Clean` seçeneği
temiz derleme yapar. macOS'ta `python3 output/build-macos.py --clean` kurulu
CubeIDE ARM GCC'sini ve `.cproject` kaynak listelerini kullanır. Artımlı derlemede
GCC dependency dosyaları ve derleme seçenekleri izlenir. Testler aynı ELF'i çalıştırır:

- `output/verify-round-ui.py`: gerçek derlenmiş ARM kodu ve LVGL ile dokunma,
  ekran geçişi, kayıt hatası, kadran, analog hizalama, klasör animasyonu, 120/70 ms
  süreler, gerçek oyun dokunmaları/sürüklemeleri ve statik İDA; PNG önizlemeler.
- `output/verify-viewmodels.py`: LVGL başlatmadan Model/ViewModel komutları, taslaklar,
  sınırlar, saat servisi hataları, alarm erteleme ve 916 baytlık kayıt uyumluluğu.

- `output/test-arcade.c`: 512 Yılan başlangıcı, dört yön, ters/çift dönüş engeli,
  dört kenardan geçiş, gövde çarpışması, büyüme, dolu tahta, duraklatma, Flappy fizik/puan/yeniden
  başlatma ve statik İDA için bağımsız C modeli/VM testi.

Testler yerel `unicorn`, `pyelftools` ve UI görselleri için `Pillow` kullanır.
RTC, panel ve flash portları yazılım testlerinde taklit edilir. Bu sonuçlar
fiziksel panel zamanlaması, güç kesintisi veya karta yükleme doğrulaması değildir.

Yeni bir özellik eklerken önce model kuralını ve ViewModel durum/komutlarını
yazın; ardından View'i bu sözleşmeye bağlayın. HAL işlevlerini ViewModel'e,
iş kurallarını LVGL callback'lerine taşımayın.

## Kontrol ve performans güncellemesi — 11 Ekim 2026

Yeni rekor modeli `watch_records.c`, kayıt servisi `watch_records_service.c` ve
ortak oyun ViewModel'i `watch_games_vm.c` üzerinden kullanılır. Oyun VM'leri
sonuçları RAM'de günceller; View yalnız güvenli oyun sınırlarında kayıt komutunu
çağırır. Başarısız kayıt dirty durumunu temizlemez. `watch_data_t` 916 bayt kalır;
platform günlüğü eski 916 veya yeni 928 bayt payload'u kabul eder. Yeni payload'un
son 12 baytı `watch_records_t` yapısıdır. Ayar kayıtları rekorları korur.

Yılan doğrudan dokunma/kaydırma kontrolüyle açılır. View dokunma bölgesinden
veya kaydırmanın baskın ekseninden yönü çıkarır; VM hücre adımı, ters dönüş ve
bekleyen yön kurallarını uygular. Kontrol seçimi, eğim kalibrasyonu ve harici
sensör portu kaldırılmıştır.

Görüntü interpolasyonu View'e aittir; Yılan'ın hücre fiziği 160–80 ms kalır.
Canvas eski/yeni gövde bölgelerini temizler ve yalnız değişen görüntüde çizim
ister. Menü ikon boyutları `generate-folder-icons.py` ile önceden rasterize edilir.
`generate-turkish-font.py`, mevcut MIT lisanslı Montserrat glyph'lerinden arayüz
metinleri için Türkçe ek glyph'ler üretir; ASCII ve semboller mevcut fonta düşer.

Son flush vblank isteğini başlatır ve LVGL tamponunu sürücüde tutarak döner.
`watch_platform_display_poll`, reload sonrası değişen satır aralıklarını gizli
tampona eşitler, sonra `lv_display_flush_ready` çağırır. Refresh timer callback'i
pending kare varken render'ı engeller; LVGL invalidation timer'ı tekrar açsa da
hazır olmayan tampon kullanılmaz. ISR yalnız tamamlanma bayrağını yazar.
`smartwatch_sync_bytes` eşitlenen baytları, `smartwatch_frame_total_ms` ilk çizim
başlangıcından eşitleme sonuna kadar geçen süreyi ölçer. Mevcut `frame_ms` ve
`frame_max_ms` ilk flush'tan başlayan ölçümleriyle korunur.

`verify-game-storage.py` gerçek ARM günlüğünün eski kayıt, CRC, torn write,
no-op yazma ve sayfa dönüşü davranışlarını; `verify-game-platform.py` gerçek ARM
frame sahipliği, DMA stride/bölge kopyasını doğrular.
Bu simülasyon testleri fiziksel GFXMMU eşlemesini veya donanım zamanlamasını
kanıtlamaz; fiziksel klasör ölçümü ayrı `verify-folder-performance` raporundadır.

## Klasör kare önbelleği ve sınırsız Yılan — 11 Ekim 2026

Klasörün 260 ms açılış/kapanış hareketi, canlı üç ikon ve yarı saydam daireyi
her adımda yeniden çizmek yerine 33 önceden çizilmiş RGB565 kare kullanır.
`generate-folder-frames.py` derlenmiş LVGL ile arka planı, ikonları ve daireyi
birleştirir. Flash'taki kare farkları 16 bit literal/tekrar paketleriyle saklanır;
XOR farkları ileri ve geri aynı biçimde uygulanır. Atlanan timer adımları da
aynı son görüntüyü üretir. Tek 300 × 300 tampon 180.000 bayt statik RAM kullanır.
Geçişte opak görüntüler alttaki canlı çizimi örter; bitişte mevcut dokunma
hedefleri ve buton geri bildirimleri yeniden görünür. Navigasyon veya geri
kaydırma, devam eden hareketi ve iki önbellek katmanını birlikte kapatır.
İki görüntünün `bg_opa` değeri de opaktır: LVGL 9.3'ün base-object cover
kontrolü bu değeri istediği için yalnız görüntünün RGB565 olması yeterli değildir.
Arayüz testi, merkezi çizim alanında `lv_refr_get_top_obj` sonucunun önbellek
canvas'ı olduğunu denetleyerek alttaki ikonların yeniden çizilmesini önler.

`watch_draw_dma2d.h`, LVGL'nin özel blend uzantısını platformdaki
`watch_display_blend_rgb565` fonksiyonuna bağlar. Tam opak, masksiz RGB565 →
ARGB8888 çizimleri DMA2D PFC ile yapılır. Clipping ve byte stride değerleri
LVGL'den gelir; transfer tamamlanmadan callback dönmez. Aynı UI görevinde
çalışan flush/eşitleme yolu daha sonra aynı DMA2D'yi yeniden yapılandırabilir.
Maskeli, saydam veya desteklenmeyen stride/hizalama durumları yazılım çizimine
döner. `smartwatch_rgb565_dma_transfers`, gerçek donanım yolunun kullanımını sayar.

Yılan modeli koordinatları 18'e göre sarar; kenara ulaşmak oyunu bitirmez.
Gövde çarpışması, kuyruk hücresinin boşalması ve büyüme kuralları korunur.
View, 17 ↔ 0 geçişini en kısa hücre hareketi olarak interpolasyonla çizer;
taşan pikseller ve invalidation alanları karşı kenara sarılır. Izgarasız zemin
ve yalnız eski/yeni gövde bölgelerinin çizilmesi, mevcut 20 ms görünüm timer'ını
korur. Model, ViewModel ve kalıcı kayıt boyutları değişmez.

Bu testler fiziksel GFXMMU eşlemesini veya donanım zamanlamasını kanıtlamaz.

## Uygulama çarkı ve duvar kâğıdı — 12 Ekim 2026

`watch_navigation.c` dokunmanın ekranını ve başlangıç hedefini tek noktada
belirler. Bildirim/uyanma, sol kenar, menü çarkı, saat hareketi ve boş alandaki
çift dokunma bu sırayla değerlendirilir. Uygulama kontrollerinin bırakma olayı
bekletilmez. Boş alan hit-test'i LVGL nesne ağacını kullanır; klasör, ikon ve
uygulama içindeki boşluk çift dokunma kısayoluna dahil değildir.

`watch_menu_view.c` yalnız dış halkayı döndürür. Açı 0,1° biriminde birikir;
atan2 geçişi ±180° etrafında normalize edilir. Hız sınırlıdır; bırakınca hedef
40° katına yuvarlanır ve ease-out animasyonu ile yerleşir. Uzun bekleyen
bırakışa hız verilmez. Klasör açma/ekran değiştirme yarım animasyonu sonlandırır.
İkon merkezleri her zaman 178 px yarıçapındadır; merkez oyunlar bu hesaba girmez.

Saat/menü için LVGL MOVE_LEFT/MOVE_RIGHT kullanılır, ekranlar silinmez.
Bütün kenar-geri işlemleri `watch_navigation_back()` üzerinden aynı MOVE_RIGHT
geçişini kullanır; sayfalar yalnız `back_targets` ile üst sayfalarını tanımlar.
İleri uygulama açılışlarındaki 180 ms içerik hareketi korunur. LVGL ekran geçişi süresince
indev okumayı durdurur; geçiş sonrasında hâlâ basılı görünen temas bırakılana
kadar tüketilir. Testler animasyon sonunda gerçek okuma döngüsündeki boşta
bırakma örneğini de gönderir. Ekran yükleme/geçiş bitişinde layout çözülür;
ilk dokunmanın eski animasyon konumlarına gitmesi önlenir.

Duvar kâğıdı seçimi Model → Settings VM → View akışındadır. Sınır kontrolü
modeldedir; VM başarısız kayıtta önceki seçimi geri yükler. `face_color`, mevcut
0..2 aralığı ve veri boyutu korunarak menü duvar kâğıdı indeksine ayrılmıştır.
Üç bitmap flash'ta RGB565 tutulur; PNG kaynakları ve üretim istemleri
`output/wallpapers/` içindedir. `generate-arcade-art.py` yeniden üretir.

Flappy'nin dekoratif katmanları küçük LVGL nesneleri olarak taşınır; çalışma
anında tüm ekranlık yeni canvas veya dönüşüm tamponu ayrılmaz. Görsel adımlar
fizik adımlarıyla ilerler ve oyunla birlikte donar. `watch_flappy_flower`,
çiçek engelinin merkezini hem çizime hem modelin daire/dikdörtgen çarpışma
hesabına sağlar. Kelebekler ve zemindeki çiçekler çarpışmaya dahil değildir.

### Değişen arka planlarla klasör animasyonu

Önceki 11 Ekim opak karelerinin yerini 17 bağımsız RGB565A8/LZ4 kare aldı.
Opak kareler duvar kâğıdını ve ikon yuvalarını içerdiğinden yeni çarkla uyumsuzdu.
Yeni kareler yalnız yarı saydam klasör ve oyun ikonlarını içerir; 260 ms harekete
16 ms civarında örneklenir. Önceki kareleri çözmeden doğrudan hedef kareye gidilir;
ileri/geri ve atlanan tick'ler aynı sonucu verir. Animasyon sırasında canlı
klasör/oyun nesneleri gizlenir, hedef kare sonunda yeniden etkinleşir. Tek tampon
270.000 bayttır; 17 kare flash'ta 497.420 bayt kullanır. Eski tam ekran backdrop
kaldırılmıştır. Opak RGB565 arka planlar DMA2D yolunu kullanmayı sürdürür.

`generate-folder-frames.py` mevcut ikon kaynaklarını Pillow ile rasterize eder;
projeye dahil BSD lisanslı LZ4'ü yerel C derleyicisiyle geçici kütüphaneye derler.
Ek pip/ağ bağımlılığı olmadan her kareyi sıkıştırıp birebir açılabildiğini doğrular.
Yeni firmware için eski donanım klasör süreleri geçerli kabul edilmez; kartta
FPS ölçümü henüz yapılmamıştır.
