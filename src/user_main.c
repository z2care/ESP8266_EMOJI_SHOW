#include "esp_common.h"
#include "freertos/task.h"
#include "gpio.h"
#include <esp_wifi.h>
#include <uart.h>

#include "user_config.h"
#ifdef Mxp
    #include "mxp.h"
#endif
#ifdef Artnet
    #include "artnet.h"
#endif
#include "ws2811dma.h"
#include "hellos.h"
#include "AdafruitNeopixel.h"
#include "i2c_master.h"
#include "lis3dh_reg.h"

#include "string.h"

/**
 * Caso a placa fique piscando loucamente após gravar e não volte a funcionar nem por ação de reza brava. Use o seguinte comando
 *   esptool.py --port /dev/ttyUSB0 write_flash 0x3fc000 ~/.platformio/packages/framework-esp8266-rtos-sdk/bin/esp_init_data_default.bin
 * 
 * Para instalar o esptool.py, use
 *   pip install esptool
 * 
 * Para dar erase 
 *   esptool.py --port /dev/ttyUSB0 erase_flash
 * 
 */

#if defined(Artnet) && defined(Mxp)
    #error "Choose only one protocol!"
#endif

#if !defined(Artnet) && !defined(Mxp)
    #error "At least one protocol must be enable!"
#endif


static struct ip_info ipConfig;
unsigned char *p = (unsigned char*)&ipConfig.ip.addr;

/* User AP config is defined in wifi_config.h, make sure to create this file and associate USER_SSID and USER_PASS*. This file will not be versioned.*/
#define WIFI_CLIENTSSID            USER_SSID
#define WIFI_CLIENTPASSWORD        USER_PASS

void ICACHE_FLASH_ATTR wifiConnectCb(System_Event_t *evt)
{
    printf("Wifi event: %d\r\n", evt->event_id);
    switch (evt->event_id) {
    case EVENT_STAMODE_CONNECTED:
        printf("connected to ssid %s, channel %d\n",
                evt->event_info.connected.ssid,
                evt->event_info.connected.channel);

        break;
    case EVENT_STAMODE_DISCONNECTED:
        printf("disconnected from ssid %s, reason %d\n",
                evt->event_info.disconnected.ssid,
                evt->event_info.disconnected.reason);
        break;
    case EVENT_STAMODE_AUTHMODE_CHANGE:
        printf("mode: %d -> %d\n",
                evt->event_info.auth_change.old_mode,
                evt->event_info.auth_change.new_mode);
        break;
    case EVENT_STAMODE_GOT_IP:
        wifi_get_ip_info(STATION_IF, &ipConfig);
        printf("%d.%d.%d.%d\n\nListening to UDP packages for LED display...",p[0],p[1],p[2],p[3]);
        #ifdef Artnet
            printf("Running Artnet!\n");
            artnet_init();
        #endif
        #ifdef Mxp
            printf("Running MXP!\n");
            mxp_init(ws2811dma_put);
        #endif

        break;
    case EVENT_SOFTAPMODE_STACONNECTED:
        printf("station: " MACSTR "join, AID = %d\n",
                MAC2STR(evt->event_info.sta_connected.mac),
                evt->event_info.sta_connected.aid);
        break;
    case EVENT_SOFTAPMODE_STADISCONNECTED:
        printf("station: " MACSTR "leave, AID = %d\n",
                MAC2STR(evt->event_info.sta_disconnected.mac),
                evt->event_info.sta_disconnected.aid);
        break;
    default:
        break;
    }
}

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;
        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}



void task_blink(void* ignore)
{
    #define PIN 2
    GPIO_AS_OUTPUT(PIN);
    while(true) {
        GPIO_OUTPUT_SET(PIN, 0);
        vTaskDelay(1000/portTICK_RATE_MS);
        GPIO_OUTPUT_SET(PIN, 1);
        vTaskDelay(1000/portTICK_RATE_MS); 
        char* test_str = "Painel de LED do LHC esta vivo!\r\n";
        printf(test_str);
    }

    vTaskDelete(NULL);
}

// 初始化 I2C 总线
void ICACHE_FLASH_ATTR i2c_init() {
    i2c_master_gpio_init();   // 初始化 GPIO
    i2c_master_init();        // 初始化 I2C 总线
}

// I2C 写函数
int ICACHE_FLASH_ATTR platform_i2c_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len) {
    uint8_t *data = (uint8_t *)os_malloc(len + 1);  // 分配内存，1 字节用于寄存器地址
    if (!data) {
        os_printf("Memory allocation failed for I2C write\n");
        return -1;  // 内存分配失败
    }

    data[0] = reg;  // 寄存器地址
    memcpy(&data[1], bufp, len);  // 将数据从 bufp 复制到 data 中

    i2c_master_start();  // 启动 I2C 通信
    int result = i2c_master_write((uint8_t *)handle, data, len + 1);  // 写入数据
    i2c_master_stop();  // 停止 I2C 通信

    os_free(data);  // 释放内存

    return result;  // 返回写操作的结果，0 表示成功，非零表示失败
}

// I2C 读函数
int ICACHE_FLASH_ATTR platform_i2c_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len) {
    int result;
    i2c_master_start();  // 启动 I2C 通信

    // 发送寄存器地址
    result = i2c_master_write((uint8_t *)handle, &reg, 1);
    if (result != 0) {
        i2c_master_stop();  // 停止 I2C 通信
        return result;      // 如果写寄存器地址失败，返回错误
    }

    i2c_master_restart();  // 重新启动 I2C

    // 从设备读取数据
    result = i2c_master_read((uint8_t *)handle, bufp, len);
    i2c_master_stop();  // 停止 I2C 通信

    return result;  // 返回读操作的结果，0 表示成功，非零表示失败
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{

    // 初始化 I2C
    i2c_init();

    // 示例数据
    uint8_t data_write[] = { 0x01, 0xFF };  // 写入数据，假设设备寄存器地址为 0x01，数据为 0xFF
    uint8_t data_read[1];  // 读取数据的缓冲区

    // 设备的 I2C 地址
    void *handle = (void *)0x3C;  // 假设设备地址是 0x3C

    // 写数据
    int write_result = platform_i2c_write(handle, 0x01, data_write, sizeof(data_write));
    os_printf("I2C write result: %d\n", write_result);

    // 读数据
    int read_result = platform_i2c_read(handle, 0x01, data_read, sizeof(data_read));
    os_printf("I2C read result: %d, Data: 0x%02x\n", read_result, data_read[0]);

    sayhello();
    WS2812B_Test();

    uart_init_new();
	UART_SetBaudrate(UART0, 115200);
	UART_SetBaudrate(UART1, 115200);
    xTaskCreate(&task_blink, "startup", 2048, NULL, 1, NULL);

    espconn_init();
    printf("SDK version:%s\r\n", system_get_sdk_version());
    static struct station_config stconfig;

    wifi_set_opmode(STATION_MODE);
    wifi_station_disconnect();
    wifi_station_dhcpc_stop();
    wifi_station_set_auto_connect(0);
    if(wifi_station_get_config(&stconfig))
    {
        memcpy(&stconfig.ssid, WIFI_CLIENTSSID, sizeof(WIFI_CLIENTSSID));
        memcpy(&stconfig.password, WIFI_CLIENTPASSWORD, sizeof(WIFI_CLIENTPASSWORD));
        wifi_station_set_config(&stconfig);
        printf("SSID: %s\n",stconfig.ssid);
    }

#ifdef SET_STATIC_IP
    ipConfig.ip.addr = ipaddr_addr(USER_AP_IP_ADDRESS);
    ipConfig.gw.addr = ipaddr_addr(USER_AP_GATEWAY);
    ipConfig.netmask.addr = ipaddr_addr(USER_AP_NETMASK);
#else
	wifi_station_dhcpc_start();
#endif


    wifi_set_ip_info(STATION_IF, &ipConfig);
    wifi_station_set_auto_connect(1);
    wifi_station_connect();
    printf("Hello World\n\n");
    wifi_set_event_handler_cb(wifiConnectCb);

    printf("Initializing WS2811...\n\n");
    ws2811dma_init();

}

