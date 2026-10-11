#include "services/watch_motion_service.h"
#include "stm32u5x9j_discovery_bus.h"
#include "stm32u5x9j_discovery_errno.h"
/* LSM6DSOX on the expansion I2C2 bus (PF1 SCL / PF0 SDA).
 * 1.8 V logic; use a compatible module/level shifter. See README wiring notes.
 * Datasheet DS12814: WHO_AM_I=0x6C, BDU+IF_INC, 104 Hz at +/-2 g.
 * Keep timeouts short: a disconnected module must never stall the UI. */
static uint16_t motion_address;
static bool bus_started;
static bool read_reg(uint8_t reg,uint8_t *data,unsigned count)
{
    return HAL_I2C_Mem_Read(&hbus_i2c2,motion_address,reg,I2C_MEMADD_SIZE_8BIT,data,count,3)==HAL_OK;
}
static bool write_reg(uint8_t reg,uint8_t value)
{
    return HAL_I2C_Mem_Write(&hbus_i2c2,motion_address,reg,I2C_MEMADD_SIZE_8BIT,&value,1,3)==HAL_OK;
}
bool watch_motion_start(void)
{
    if(!bus_started) {
        if(BSP_I2C2_Init()!=BSP_ERROR_NONE) return false;
        bus_started=true;
    }
    motion_address=0;
    for(unsigned address=0x6A;address<=0x6B;++address) {
        motion_address=(uint16_t)(address<<1);
        uint8_t identity=0;
        if(read_reg(0x0F,&identity,1) && identity==0x6C &&
           write_reg(0x12,0x44) && write_reg(0x10,0x40)) return true;
    }
    motion_address=0;
    return false;
}
bool watch_motion_read(watch_acceleration_t *sample)
{
    uint8_t ready,data[6];
    if(!motion_address || !read_reg(0x1E,&ready,1) || !(ready&1) || !read_reg(0x28,data,6)) return false;
    /* +/-2 g, 0.061 mg/LSB; expose milli-g independent of the sensor driver. */
    sample->x=(int16_t)((int16_t)(data[0]|data[1]<<8)*61/1000);
    sample->y=(int16_t)((int16_t)(data[2]|data[3]<<8)*61/1000);
    sample->z=(int16_t)((int16_t)(data[4]|data[5]<<8)*61/1000);
    return true;
}
void watch_motion_stop(void)
{
    if(motion_address) (void)write_reg(0x10,0);
    motion_address=0;
}
