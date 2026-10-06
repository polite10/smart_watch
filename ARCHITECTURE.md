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
| Statik İDA konsolu | `watch_ida_view.c` | `watch_ida_vm.c` |
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
- Ana döngü ve bildirim öncelikleri korunur. Menü/oyunlar dokunma beklemesini
  atlar; diğer uygulamalardaki çift dokunma penceresi 120 ms'dir.
- İkon bitmapleri ortak bir `.c` dosyasından sunulur; menü ve ayarlar aynı
  bitmapleri kullanır.

Analog kadranın ibre merkezi ve işaretleri **(240, 240)** etrafındadır. Rakamların
merkezleri eşit yarıçapta hizalanır. Tarih ve su bilgisi merkezin üstüne/altına
simetrik yerleşir. Analog kadranda dijital saniye etiketi yoktur; saniye ibresi
ve diğer dijital kadranların saniye göstergeleri çalışmaya devam eder.

## Oyun klasörü ve yeni ekranlar

`watch_menu_view.c` dokuz uygulamayı dış çembere, üç oyunu merkezdeki yuvarlak
klasöre yerleştirir. 180 ms büyüme sırasında oyun hedefleri tıklanamaz; hareket
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
  gövde/duvar çarpışması, büyüme, dolu tahta, duraklatma, Flappy fizik/puan/yeniden
  başlatma ve statik İDA için bağımsız C modeli/VM testi.

Testler yerel `unicorn`, `pyelftools` ve UI görselleri için `Pillow` kullanır.
RTC, panel ve flash portları yazılım testlerinde taklit edilir. Bu sonuçlar
fiziksel panel zamanlaması, güç kesintisi veya karta yükleme doğrulaması değildir.

Yeni bir özellik eklerken önce model kuralını ve ViewModel durum/komutlarını
yazın; ardından View'i bu sözleşmeye bağlayın. HAL işlevlerini ViewModel'e,
iş kurallarını LVGL callback'lerine taşımayın.
