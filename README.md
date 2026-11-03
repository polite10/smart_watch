# Smartwatch

STM32U5A9J-DK için STM32CubeIDE projesi. LVGL 9.3.0 kullanır; TouchGFX gerekmez.

## PuTTY ile LED'i kontrol etme

1. Kartı **ST-LINK USB CN5** portundan bilgisayara bağlayın. Windows Aygıt
   Yöneticisi'nde **Bağlantı Noktaları (COM ve LPT)** altında ST-LINK'in COM
   numarasını bulun. Bu kart son denemede **COM5** olarak göründü; numara değişebilir.
2. Device Studio veya aynı COM portunu kullanan başka bir terminal açıksa kapatın.
   PuTTY'de **Session > Serial** seçin; **Serial line** alanına COM numarasını,
   **Speed** alanına `115200` yazın. **Connection > Serial** altında **8 data bits,
   1 stop bit, parity None, flow control None** ayarlayıp **Open**'a basın.
3. Her saniye `LED=...` durum satırı görünür. Büyük harflerle `HELP` yazıp Enter'a
   basın; `OK HELP: ...` yanıtı bağlantının ve komut alımının çalıştığını gösterir.
   Yazdıklarınız görünmüyorsa **Terminal > Local echo > Force on** seçin.
   Geri silme ve satır düzenleme için **Local line editing > Force on** seçin.

| Komut | Sonuç |
|---|---|
| `LED ON` | Yeşil LD3 LED sürekli yanar. |
| `LED OFF` | LED söner. |
| `SET INTERVAL 250` | LED her 250 ms'de durum değiştirir; tam yanıp sönme döngüsü 500 ms'dir. |
| `LED BLINK` | Kaydedilmiş aralıkla yanıp sönmeyi başlatır. |
| `GET STATUS` | Anlık durumu `OK LED=...` satırında gösterir. |
| `RESET PRESSES` | Fiziksel buton basma sayacını sıfırlar. |
| `HELP` | Komut listesini gösterir. |

Her komutun ardından `OK ...` veya `ERR ...` cevabını bekleyin. `SET INTERVAL`
değeri **10–60000 ms** aralığında ve **10 ms'nin katı** olmalıdır. Ayarlar RAM'de
tutulur; kart yeniden başladığında varsayılan 1000 ms yanıp sönmeye döner.
Komutlar ve örnek yanıtlar için [ayrıntılı PuTTY kılavuzuna](docs/led-uart-commands.md)
bakın.


