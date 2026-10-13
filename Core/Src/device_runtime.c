/* Runtime commands use UART RX interrupts, separately from existing TX DMA. */
#include "device_runtime.h"
#include "device_contract.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static UART_HandleTypeDef *port;
static const char *app_name;
static device_command_handler_t app_handler;
static volatile unsigned head,tail;
static volatile uint32_t dropped;
static unsigned seen_drops;
static uint8_t ring[256];
static char line[96];
static unsigned length;
static int discard;

void device_runtime_init(UART_HandleTypeDef *uart,const char *app,device_command_handler_t handler) {
    port=uart;app_name=app;app_handler=handler;
    head=tail=length=dropped=seen_drops=0;discard=0;
    __HAL_UART_ENABLE_IT(port,UART_IT_RXNE);
}
void device_runtime_rx_irq(void) {
    if(!port)return;
    uint32_t flags=port->Instance->ISR;
    if(flags&(USART_ISR_ORE|USART_ISR_FE|USART_ISR_NE|USART_ISR_PE)){
        port->Instance->ICR=USART_ICR_ORECF|USART_ICR_FECF|USART_ICR_NECF|USART_ICR_PECF;
        ++dropped;
    }
    while(port->Instance->ISR&USART_ISR_RXNE_RXFNE) {
        uint8_t b=(uint8_t)port->Instance->RDR;unsigned next=(head+1)&255U;
        if(next==tail)++dropped;
        else {ring[head]=b;__DMB();head=next;}
    }
}
static void command(void) {
    unsigned long seq=0;char cmd[40]={0};
    if(sscanf(line,"REQ %lu %39s",&seq,cmd)!=2)return;
    char message[320];const char *state="{}",*error="";
    if(!strcmp(cmd,"HELLO") || !strcmp(cmd,"STATUS")) {
        if(app_handler)state=app_handler("STATUS");
    } else if(!strcmp(cmd,"BOOT")) {
        if(!device_boot_installed())error="bootloader_missing";
    } else if(app_handler)state=app_handler(cmd);
    else error="unsupported_command";
    if(!state)state="{}";
    int n=snprintf(message,sizeof message,"{\"seq\":%lu,\"app\":\"%s\",\"version\":1,\"protocol\":1,\"boot\":%d,\"ready\":true,\"error\":\"%s\",\"state\":%s}\r\n",
       seq,app_name,device_boot_installed()?1:0,error,state);
    if(n>0 && n<(int)sizeof message && HAL_UART_Transmit(port,(uint8_t *)message,n,200)==HAL_OK && !strcmp(cmd,"BOOT") && !*error) {
        HAL_Delay(20);device_request_boot();
    }
}
void device_runtime_poll(void) {
    if(!port || port->gState!=HAL_UART_STATE_READY)return;
    if(seen_drops!=dropped){seen_drops=dropped;discard=1;length=0;}
    while(tail!=head) {
        uint8_t b=ring[tail];tail=(tail+1)&255U;
        if(b=='\n') {
            if(!discard && length){line[length]=0;command();}
            length=0;discard=0;return;
        }
        if(b=='\r')continue;
        if(length+1>=sizeof line){discard=1;length=0;}
        if(!discard)line[length++]=(char)b;
    }
}
