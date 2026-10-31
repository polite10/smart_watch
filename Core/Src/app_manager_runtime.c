#include "app_manager_runtime.h"
#include "am_picker.h"
#include "am_manager.h"
#include "am_port.h"
#include "am_nor_stm32.h"
#include "stm32u5xx_hal.h"
#include "device_contract.h"
#include "lvgl.h"
#include "services/watch_display_service.h"
#include <string.h>
extern const device_app_info_t device_app_info;
static int requested_slot=-1,close_requested;
static uint32_t requested_at;
static am_picker_entry_t entries[AM_SLOT_COUNT];
static unsigned entry_count;
static uint32_t boot_version(void){
 return device_boot_installed()?((const device_boot_info_t *)DEVICE_BOOT_INFO)->version:0;
}
/* Defer flash I/O and LVGL deletion until after the click callback returns.
 * The running application closes the picker; another one is installed. */
static int select_app(uint32_t slot,void *ctx){
 (void)ctx;
 if(requested_slot>=0 || close_requested)return -1;
 for(unsigned i=0;i<entry_count;++i)
  if(entries[i].slot==slot && entries[i].id==device_app_info.id){close_requested=1;return 0;}
 if(boot_version()<3 || slot>=AM_SLOT_COUNT)return -1;
 requested_slot=(int)slot;requested_at=HAL_GetTick();return 0;
}
void app_manager_open(void){
 if(am_picker_is_open())return;
 /* The NOR port is opened only for the catalog and closed again, so the
  * application keeps its own peripherals. */
 int nor=am_nor_init()==0;
 entry_count=0;
 if(nor)for(unsigned slot=0;slot<AM_SLOT_COUNT;++slot){
  am_header_t h;
  if(am_read_header(&am_stm32_io,slot,&h)!=AM_OK)continue;
  entries[entry_count]=(am_picker_entry_t){.slot=slot,.id=h.id,.version=h.version,.size=h.size};
  memcpy(entries[entry_count].name,h.name,sizeof h.name);++entry_count;
 }
 am_nor_deinit();
 watch_ui_display_power(true);
 am_picker_open(entries,entry_count,device_app_info.id,select_app,0);
 if(boot_version()<3)am_picker_show_error("DeviceBoot v3 gerekli: uygulama değiştirilemez.");
 else if(!nor)am_picker_show_error("Uygulama belleği okunamadı.");
}
void app_manager_poll(void){
 static uint32_t down_at;static int was_down,handled,started;
 uint32_t now=HAL_GetTick();
 /* Every power-up and reset starts at the picker, except in the application
  * the bootloader has just installed for it. */
 if(!started && now>=300 && lv_display_get_default()){
  started=1;
  if(!device_take_opened(device_app_info.id) && boot_version()>=4)app_manager_open();
 }
 int down=HAL_GPIO_ReadPin(GPIOC,GPIO_PIN_13)==GPIO_PIN_SET;
 if(down && !was_down){down_at=now;handled=0;}
 if(down && !handled && now-down_at>=1500){handled=1;app_manager_open();}
 was_down=down;
 if(close_requested){close_requested=0;am_picker_close();}
 if(requested_slot>=0 && now-requested_at>=150){
  unsigned slot=(unsigned)requested_slot;requested_slot=-1;
  am_result_t result=am_nor_init()==0?am_request(&am_stm32_io,slot):AM_E_IO;
  am_nor_deinit();
  if(result==AM_OK){__DSB();NVIC_SystemReset();}
  else am_picker_show_error("Kurulum isteği yazılamadı. Uygulama korunuyor.");
 }
}
