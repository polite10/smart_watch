# LED uygulaması: PuTTY komutları ve UART RX DMA

Bu sürüm yeşil LD3 LED'ini terminalden yönetir. STM32 firmware'i komutları alıp
uygular; PuTTY tarafında program yazmak gerekmez. Mevcut LCD LED/DMA göstergesi
aynı çalışma durumundan beslendiği için yeni LED durumunu ve aralığını da gösterir.

## Bağlantı ve ilk deneme

1. Güncel **DeviceBoot uyumlu** `Firmware/Smartwatch.dfw` paketini Device Studio
   üzerinden karta yükleyin. Binary uygulama tabanı **0x0800A000**'dır; eski
   bootloader'sız uygulama gibi `0x08000000` adresine yüklenmemelidir.
2. Kartı **ST-LINK USB CN5** üzerinden bilgisayara bağlayın.
3. PuTTY'de **Serial** bağlantısı ve kartın `COM…` portunu seçin.
4. **115200 baud, 8 data bits, parity None, 1 stop bit, flow control None** kullanın.
5. İsterseniz Terminal bölümündeki **Local echo** ve **Local line editing**
   seçeneklerini **Force on** yapın. Firmware girilen karakterleri geri yansıtmaz;
   satır düzenlemeyi terminal yapar. Ham backspace/DEL karakterleri komutta geçersizdir.
6. `HELP` yazıp Enter'a basın. `OK HELP: ...` cevabını görün.

USART1 **TX PA9 / AF7**, **RX PA10 / AF7** kullanır. Bilgisayardaki USB seri
haberleşmesi, ST-LINK sanal COM portu üzerinden UART'a çevrilir. UART her
karakteri 1 start + 8 veri + 1 stop biti olarak taşır. Uygulama komutunun
nerede bittiğini ise aşağıdaki metin protokolü belirler.

## Komut protokolü

- Komutlar **büyük harfli ASCII** metindir. Parametreler boşluk veya tab ile ayrılır.
- **CR**, **LF** ve **CRLF** satırı bitirir; boş satırlar yok sayılır.
- Satır sonu hariç en fazla **63 karakter** kabul edilir. Ek parametre reddedilir.
- Komut birden fazla DMA bildiriminde gelebilir; tek bildirimde birden fazla
  komut bulunabilir. **UART IDLE, komut sonu değildir.**
- Başarılı cevap `OK ` ile, hata cevabı `ERR ` ile başlar ve CRLF ile biter.
- Her saniye gelen kendiliğinden durum satırı `LED=` ile başlar.
- Bilgisayar uygulaması her komut için `OK`/`ERR` cevabını beklemelidir.
  Cevap beklemeden sınırsız komut göndermek, sınırlı alım tamponunu doldurabilir.

| Komut | Davranış |
|---|---|
| `LED ON` | LED'i hemen sürekli açık tutar. |
| `LED OFF` | LED'i hemen sürekli kapalı tutar. |
| `LED BLINK` | Saklanan aralıkla yanıp sönmeyi başlatır; ilk geçiş tam bir aralık sonra olur. |
| `SET INTERVAL 250` | Aralığı ayarlar ve yanıp sönmeyi başlatır; ilk geçiş tam bir aralık sonra olur. |
| `GET STATUS` | Yeni bir durum görüntüsünü `OK LED=...` cevabıyla verir. |
| `RESET PRESSES` | Fiziksel butona basılma sayacını sıfırlar; çalışma modunu değiştirmez. |
| `HELP` | Komut listesini gönderir. |

Aralık **10–60000 ms** ve **10 ms'nin katı** olmalıdır. Timer 10 ms adımlarla
çalışır; örneğin 15 ms kabul edilmez. Sayısal taşma, negatif sayı ve bozuk sayı
reddedilir. Yanıp sönmede mevcut açık/kapalı faz korunur; süre sayacı sıfırlanır.
250 ms, açık veya kapalı durumun tek süresidir; tam çevrim 500 ms'dir.

Örnek oturum (cevaptaki sayaçlar çalışma anına bağlıdır):

```text
> SET INTERVAL 250
OK LED=LD3 mode=BLINK state=OFF toggle_ms=250 cycle_ms=500 presses=0 uptime_ms=1200 uart_ok=1 uart_errors=0 interval_ms=250 rx_errors=0 rx_overflows=0
> LED OFF
OK LED=LD3 mode=OFF state=OFF toggle_ms=0 cycle_ms=0 presses=0 uptime_ms=2000 uart_ok=3 uart_errors=0 interval_ms=250 rx_errors=0 rx_overflows=0
> SET INTERVAL 15
ERR interval_range 10..60000 step 10
```

`>` yalnız örnekte kullanıcı girişini gösterir; gönderilecek komuta dahil değildir.
`mode=ON` sürekli açık, `mode=OFF` sürekli kapalıdır. Bu iki modda `toggle_ms`
ve `cycle_ms` sıfırdır; `interval_ms` daha sonra `LED BLINK` ile kullanılacak
ayarı korur. `uart_ok` mevcut cevabın gönderimi tamamlanmadan alınmış sayaçtır.
`uart_errors` TX toparlanma/zaman aşımı sayısıdır; ortak HAL UART/DMA hatasında
iki yön toparlandığından RX hatası bu sayacı da artırabilir. `rx_errors` RX
toparlanma, `rx_overflows` yazılım halkasının taşma sayısıdır.

Ayarlar **RAM'de** tutulur. Yeniden başlatma 1000 ms yanıp sönme ve sıfır sayaçlarla
başlar; flash'a kayıt veya `SAVE` komutu yoktur. Fiziksel B1 butonu kendi preset
indeksini sürdürür: 1000 → kapalı → 100 → 250 → 500 → 1000 ms. Seri komut bu
indeksi değiştirmez; sonraki B1 basışı buton dizisindeki bir sonraki preset'i uygular.

## Alım yönteminden bağımsız komut işleyicisi

```text
USART1 RX → GPDMA1 Channel 1 → 256 baytlık dairesel DMA tamponu
                               ↓ IDLE / HT / TC kesmesi
                         2048 baytlık yazılım halkası
                               ↓ ana döngü
                  watch_led_commands_push(byte, &command)
                               ↓ doğrulanmış komut
                    execute_command() → LED ayarı
                               ↓
             cevap tamponu → TX DMA Channel 0 → USART1 TX
```

`App/services/watch_led_commands.h/.c` sadece standart C tiplerini kullanır.
İçinde UART, HAL, DMA, GPIO veya LVGL çağrısı yoktur. `watch_led_commands_push()`
tek baytı alır; satır tamamlandığında komut türü ve parametresini üretir.
Eksik satırda ayar değişmez. Hatalı satırın kalan kısmı satır sonuna kadar
atılır; geçerli görünen bir son ek ayrı komut gibi çalıştırılmaz.

İleride RX kesmesine veya farklı bir kaynağa geçilirse aynı fonksiyona aynı
baytlar verilir. Değişen parça veriyi getiren adaptördür; komut söz dizimi ve
doğrulama değişmez. Parser tek yürütme bağlamında çağrılmalıdır; bu projede
bu bağlam ana döngüdür.

`Core/Src/led_button_demo.c` donanım adaptörünü ve komut uygulamasını içerir:

- GPDMA1 Channel 1 için **tek düğümlü dairesel linked-list** oluşturur.
  `HAL_UARTEx_ReceiveToIdle_DMA()` alımı başlatır; normal veri akışında yeniden
  başlatılmaz. STM32U5 GPDMA'nın dairesel modu linked-list üzerinden kurulur.
- **HT** (yarım tampon), **TC** (tam tampon) ve USART **IDLE** bildirimleri yeni
  baytları yazılım halkasına kopyalar. Komut çalıştırma/cevap biçimlendirme kesmede yapılmaz.
- Callback, o anki DMA kalan veri sayısını okuyarak yeni konumu bulur.
  Aynı IDLE/HT/TC bildiriminin yinelenmesi baytları tekrar işlettirmez; IDLE'dan
  sonra servis edilen eski bir HT/TC olayı da daha eski veriyi yeniden kopyalamaz.
- RX DMA ve USART kesmeleri aynı önceliktedir; iki üretici callback iç içe çalışmaz.
  Halka indeksleri bellek bariyerleriyle yayınlanır.
- Ana döngü her çağrıda en fazla **256 bayt / bir tamamlanmış komut** işler.
  Bir cevap bekliyorsa sonraki komut halka içinde bekler; aktif TX DMA tamponu
  son USART TC kesmesine kadar korunur. Cevaplar periyodik raporlardan önceliklidir.
- LED modu, aralığı ve GPIO değişiklikleri kısa kritik bölümde uygulanır.
  Yanıp sönme kenarları ve buton debounce işlemi **TIM6** kesmesinde kalır.

DMA karakterler üzerinde yorum yapmaz; UART ile RAM arasında veri taşır.
DMA2D/LTDC ekran yolu ayrı donanımı kullanır ve bu değişiklikte değiştirilmez.

## Hatalar, kapasite ve yeniden eşzamanlama

Device Studio uyumluluğu: `REQ <seq> HELLO`, `REQ <seq> STATUS` ve
`REQ <seq> BOOT` aynı parser üzerinden yönlendirilir. Sıra numarası işaretsiz
32 bit tam sayıdır; yanıt eski JSON alanlarını korur. LED komutlarına `OK`/`ERR`,
bu isteklere JSON döner. `Core/Src/device_runtime.c` JSON biçimlendirmeyi yapar;
RXNE okuyucusu ve bloklayan UART gönderimi kaldırılmıştır. BOOT isteği kendi
JSON cevabının USART TC tamamlanmasından 20 ms sonra ana döngüde reset üretir;
önceki bir cevabın tamamlanması veya yalnız DMA TC reseti başlatamaz.

Geçersiz komut `ERR invalid_command`, uzun satır `ERR line_too_long`, aralık
hatası `ERR interval_range` cevabı verir. Hatalı komut LED ayarını değiştirmez.

2048 baytlık yazılım halkası bir boş yuva bırakır; kapasitesi **2047 bayttır**.
Taşma veya UART/DMA alım hatasında sıradaki alım verisi ve yarım komut atılır.
`ERR rx_data_lost; send a newline, then retry` bildirimi gönderilir.
**Önce boş Enter, sonra komut** gönderin; ilk satır sonuna kadar veri atılarak
eksik bir komutun son parçasının çalışması engellenir. Önceden uygulanmış bir
komut geri alınmaz; bağlantı hatasında kaybolan cevabın otomatik tekrar gönderimi yoktur.

UART/DMA hatası ana döngüde RX abort/restart ile toparlanır. HAL'in ortak DMA
hata yolu TX'i de durdurabildiğinden TX sahipliği de bırakılır. Yalnız TX'in
250 ms zaman aşımına uğraması RX'i yeniden başlatmaz. Normal çalışmada iki yön
aynı anda aktiftir; UART donanımsal akış kontrolü kullanılmaz.

Sonlu tamponlar sınırsız veri kaybı garantisi vermez. 115200 8N1'de yarım DMA
tamponu yaklaşık **11 ms**'de dolar; kesmeler bu süreyi aşacak şekilde sürekli
engellenmemelidir. Ana döngü de yazılım halkası dolmadan veriyi tüketmelidir.
Yoğun otomatik kullanımda her komutun cevabını beklemek gerekir.

## Derleme ve doğrulama

```sh
python3 output/build-macos.py
python3 tools/package-firmware.py
python3 output/verify-led-commands.py
python3 output/verify-led-button.py
```

Windows derlemesi: `./Build.ps1`. Derleyici `.cproject` içindeki App ve Core
kaynak klasörlerinden yeni parser dosyasını da alır.

- Yerel C parser testi: **65 kontrol**, UndefinedBehaviorSanitizer açık.
  Uygun makinelerde `--address` ile AddressSanitizer da etkinleştirilebilir.
- ARM/HAL entegrasyonu: **85 kontrol, 0 başarısızlık**. Parçalı/bitişik komutlar,
  CR/LF, kesintisiz HT/TC alımı, DMA tampon sınırı, eski bildirimler, hatalı
  girdiler, TX sahipliği, RX taşması, UART/DMA toparlanması, LCD telemetri kaynağı
  ve Device Studio / BOOT yanıt sıralaması doğrulanır.
- Raporlar: `output/verify-led-button.json`, `output/build-led-button.json`.
  Test raporu kullandığı ELF'in SHA-256 değerini içerir.

ARM testi Unicorn ve pyelftools ister; varsa `tmp/simulator-libs` kullanılır.
Simülasyonda gerçek derlenmiş ARM kodu ve HAL kesmeleri yürütülür; DMA veri
hareketi, node yüklemesi, periferik register'lar ve abort tamamlanması modellenir.
**Fiziksel kartta USB/seri port ve elektriksel zamanlama testi yerine geçmez.**
