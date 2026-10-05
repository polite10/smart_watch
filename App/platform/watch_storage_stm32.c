#include "services/watch_repository.h"
#include "stm32u5xx_hal.h"
#include <string.h>

/* Append snapshots across two reserved pages. Commit header is written last;
 * when rotating, erase the other page while retaining the latest valid record. */
#define STORE_BASE 0x083FC000U
#define STORE_PAGE 8192U
#define STORE_SLOT 1024U
#define STORE_MAGIC 0x53574132U
typedef struct { uint32_t magic, sequence, crc, size; } store_header_t;
static uint32_t latest_address, latest_sequence;
static uint8_t record[STORE_SLOT] __attribute__((aligned(16)));
_Static_assert(sizeof(watch_data_t) <= STORE_SLOT - 16, "Settings exceed journal slot");
static uint32_t checksum(const void *data, unsigned size)
{
    const uint8_t *p = data;
    uint32_t crc = 0xFFFFFFFFU;
    while(size--) {
        crc ^= *p++;
        for(unsigned bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ (0xEDB88320U & (0U - (crc & 1U)));
    }
    return ~crc;
}
static bool valid_record(uint32_t address)
{
    const store_header_t *h = (const store_header_t *)address;
    return h->magic == STORE_MAGIC && h->size == sizeof(watch_data_t) &&
           h->crc == checksum((const void *)(address + 16), h->size);
}
bool watch_storage_load(watch_data_t *data)
{
    latest_address = latest_sequence = 0;
    for(uint32_t address = STORE_BASE; address < STORE_BASE + STORE_PAGE * 2; address += STORE_SLOT) {
        const store_header_t *h = (const store_header_t *)address;
        if(valid_record(address) && (!latest_address || (int32_t)(h->sequence - latest_sequence) > 0)) {
            latest_address = address;
            latest_sequence = h->sequence;
        }
    }
    if(!latest_address) return false;
    memcpy(data, (const void *)(latest_address + 16), sizeof *data);
    return true;
}
static bool erased_slot(uint32_t address)
{
    const uint32_t *p = (const uint32_t *)address;
    for(unsigned i = 0; i < STORE_SLOT / 4; ++i) if(p[i] != 0xFFFFFFFFU) return false;
    return true;
}
bool watch_storage_save(const watch_data_t *data)
{
    if(latest_address && !memcmp(data, (const void *)(latest_address + 16), sizeof *data)) return true;
    uint32_t page = latest_address ? (latest_address - STORE_BASE) / STORE_PAGE : 0;
    uint32_t address = STORE_BASE + page * STORE_PAGE;
    while(address < STORE_BASE + (page + 1) * STORE_PAGE && !erased_slot(address)) address += STORE_SLOT;
    bool erase = address == STORE_BASE + (page + 1) * STORE_PAGE;
    if(erase) { page ^= 1; address = STORE_BASE + page * STORE_PAGE; }
    /* A previous firmware may have programmed 0xFF including its ECC bits.
     * With no valid journal, explicitly erase before the first write. */
    if(!latest_address) { page = 0; address = STORE_BASE; erase = true; }
    if(HAL_FLASH_Unlock() != HAL_OK) return false;
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_ALL_ERRORS);
    HAL_StatusTypeDef status = HAL_OK;
    if(erase) {
        FLASH_EraseInitTypeDef e = {0}; uint32_t error;
        e.TypeErase = FLASH_TYPEERASE_PAGES;
        e.Banks = FLASH_BANK_2;
        e.Page = 254 + page;
        e.NbPages = 1;
        status = HAL_FLASHEx_Erase(&e, &error);
    }
    memset(record, 0xFF, sizeof record);
    store_header_t h = {STORE_MAGIC, latest_sequence + 1, checksum(data, sizeof *data), sizeof *data};
    memcpy(record, &h, sizeof h);
    memcpy(record + 16, data, sizeof *data);
    unsigned length = (16 + sizeof *data + 15) & ~15U;
    for(unsigned offset = 16; status == HAL_OK && offset < length; offset += 16)
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, address + offset, (uint32_t)(record + offset));
    if(status == HAL_OK) status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, address, (uint32_t)record);
    HAL_FLASH_Lock();
    /* ICACHE also caches flash data reads; refresh journal reads after writes. */
    if(HAL_ICACHE_Invalidate() != HAL_OK) return false;
    if(status != HAL_OK || !valid_record(address)) return false;
    latest_address = address; latest_sequence = h.sequence;
    return true;
}
