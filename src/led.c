#include <stdio.h>
#include <espressif/esp_wifi.h>
#include <espressif/esp_sta.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <wifi_config.h>

#include <rboot-api.h>

const int led_gpio = 15;
bool led_on = false;

volatile bool paired = false;
volatile bool Wifi_Connected = false;

// GLOBAL CHARACTERISTICS


homekit_characteristic_t manufacturer = HOMEKIT_CHARACTERISTIC_(MANUFACTURER, "Curla92");
homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, "Simple LED");
homekit_characteristic_t model = HOMEKIT_CHARACTERISTIC_(MODEL, "SimpleLED");
homekit_characteristic_t serial = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, NULL);
homekit_characteristic_t revision = HOMEKIT_CHARACTERISTIC_(FIRMWARE_REVISION, "0.0.1");

void led_write(bool on) {
    gpio_write(led_gpio, on ? 0 : 1);
}

void led_init() {
    gpio_enable(led_gpio, GPIO_OUTPUT);
    led_write(led_on);
}

void led_identify_task(void *_args) {
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            led_write(true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            led_write(false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    led_write(led_on);

    vTaskDelete(NULL);
}

void led_identify(homekit_value_t _value) {
    printf("LED identify\n");
    xTaskCreate(led_identify_task, "LED identify", 128, NULL, 2, NULL);
}

homekit_value_t led_on_get() {
    return HOMEKIT_BOOL(led_on);
}

void led_on_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }

    led_on = value.bool_value;
    led_write(led_on);
}


homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
        &name,
        &manufacturer,
        &serial,
        &model,
        &revision,
            HOMEKIT_CHARACTERISTIC(IDENTIFY, led_identify),
            NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Sample LED"),
            HOMEKIT_CHARACTERISTIC(
                ON, false,
                .getter=led_on_get,
                .setter=led_on_set
            ),
            NULL
        }),
        NULL
    }),
    NULL
};

void on_event(homekit_event_t event)
{


    if (event == HOMEKIT_EVENT_SERVER_INITIALIZED)
    {

        printf("SERVER JUST INITIALIZED\n");


        if (homekit_is_paired())
        {

            printf("Found pairing, starting timers\n");
        }
    }
    else if (event == HOMEKIT_EVENT_CLIENT_CONNECTED)
    {


        if (!paired)

            printf("CLIENT JUST CONNECTED\n");
    }
    else if (event == HOMEKIT_EVENT_CLIENT_DISCONNECTED)
    {


        if (!paired)

            printf("CLIENT JUST DISCONNECTED\n");
    }
    else if (event == HOMEKIT_EVENT_PAIRING_ADDED || event == HOMEKIT_EVENT_PAIRING_REMOVED)
    {


        paired = homekit_is_paired();

        printf("CLIENT JUST PAIRED\n");
    }
}


void create_accessory()
{


    // Accessory Name


    uint8_t macaddr[6];


    sdk_wifi_get_macaddr(STATION_IF, macaddr);


    int name_len = snprintf(NULL, 0, "Simple LED-%02X%02X%02X",


        macaddr[3], macaddr[4], macaddr[5]);


    char* name_value = malloc(name_len + 1);


    snprintf(name_value, name_len + 1, "Simple LED-%02X%02X%02X",


        macaddr[3], macaddr[4], macaddr[5]);


    name.value = HOMEKIT_STRING(name_value);


    // Accessory Serial


    char* serial_value = malloc(13);


    snprintf(serial_value, 13, "%02X%02X%02X%02X%02X%02X", macaddr[0], macaddr[1], macaddr[2],
        macaddr[3], macaddr[4], macaddr[5]);


    serial.value = HOMEKIT_STRING(serial_value);
}


homekit_server_config_t config = {

    .accessories = accessories,

    .password = "111-11-111",

    .on_event = on_event,
};


void on_wifi_event(wifi_config_event_t event)
{


    if (event == WIFI_CONFIG_CONNECTED)
    {

        printf("CONNECTED TO >>> WIFI <<<\n");
        Wifi_Connected = true;

        homekit_server_init(&config);
    }
    else if (event == WIFI_CONFIG_DISCONNECTED)
    {
        Wifi_Connected = false;
        printf("DISCONNECTED FROM >>> WIFI <<<\n");
    }
}


void hardware_init()
{
    // LED INIT
    gpio_enable(led_gpio, GPIO_OUTPUT);
    led_write(led_on);
}


void user_init(void)
{

    uart_set_baud(0, 115200);

    printf("SDK version:%s\n", sdk_system_get_sdk_version());

    hardware_init();

    create_accessory();

    // wifi_config_init("Simple LED", NULL, on_wifi_ready);

    wifi_config_init2("Simple LED", NULL, on_wifi_event);
}

