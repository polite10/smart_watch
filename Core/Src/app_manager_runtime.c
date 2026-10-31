#include "app_manager_runtime.h"
#include "am_picker.h"
#include "am_manager.h"
#include "am_port.h"
#include "am_nor_stm32.h"
#include "stm32u5xx_hal.h"
#include "device_contract.h"
#include "services/watch_display_service.h"
#include <string.h>
extern const device_app_info_t device_app_info;
static int nor_ready;
static int requested_slot=-1;
static uint32_t requested_at;
static am_picker_entry_t entries[AM_SLOT_COUNT];
/* Defer flash I/O until after LVGL's click callback returns. */
static int select_app(uint32_t slot,void *ctx){
 (void)ctx;
 if(!device_boot_installed() || ((const device_boot_info_t *)DEVICE_BOOT_INFO)->version<3)return -1;
 if(requested_slot>=0 || slot>=AM_SLOT_COUNT)return -1;
 requested_slot=(int)slot;requested_at=HAL_GetTick();return 0;
}
void app_manager_open(void){
 if(am_picker_is_open())return;
 if(!nor_ready)nor_ready=am_nor_init()==0;
 unsigned count=0;
 if(nor_ready)for(unsigned slot=0;slot<AM_SLOT_COUNT;++slot){
  am_header_t h;
  if(am_read_header(&am_stm32_io,slot,&h)!=AM_OK)continue;
  entries[count]=(am_picker_entry_t){.slot=slot,.id=h.id,.version=h.version,.size=h.size};
  memcpy(entries[count].name,h.name,sizeof h.name);++count;
 }
 watch_ui_display_power(true);
 am_picker_open(entries,count,device_app_info.id,select_app,0);
 if(!device_boot_installed() || ((const device_boot_info_t *)DEVICE_BOOT_INFO)->version<3)
  am_picker_show_error("DeviceBoot v3 gerekli. Uygulama degistirilemedi.");
 else if(!nor_ready)am_picker_show_error("Bellek okunamadi. Degisiklik yapilmadi.");
}
void app_manager_poll(void){
 static uint32_t down_at;static int was_down,handled;
 int down=HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_13)==GPIO_PIN_SET;
 uint32_t now=HAL_GetTick();
 if(down && !was_down){down_at=now;handled=0;}
 if(down && !handled && now-down_at>=1500){handled=1;app_manager_open();}
 was_down=down;
 if(requested_slot>=0 && now-requested_at>=100){
  unsigned slot=(unsigned)requested_slot;requested_slot=-1;
  am_result_t result=am_request(&am_stm32_io,slot);
  if(result==AM_OK){am_nor_deinit();__DSB();NVIC_SystemReset();}
  else am_picker_show_error("Dosya veya bellek hatasi. Uygulama korunuyor.");
 }
}
