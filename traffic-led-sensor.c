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
#include <button.h>
#include <dht/dht.h>

#ifndef SENSOR_PIN
#error SENSOR_PIN is not specified
#endif

#ifndef GREEN_PIN
#error GREEN_PIN is not specified
#endif

#ifndef RED_PIN
#error RED_PIN is not specified
#endif

#ifndef BUTTON_PIN
#error BUTTON_PIN is not specified
#endif

bool led_green_on = false;
bool led_red_on = false;

void led_green_write(bool on) {
    gpio_write(GREEN_PIN, on ? 0 : 1);
}

void led_red_write(bool on) {
    gpio_write(RED_PIN, on ? 0 : 1);
}

void led_init() {
    gpio_enable(GREEN_PIN, GPIO_OUTPUT);
    led_green_write(led_green_on);
    gpio_enable(RED_PIN, GPIO_OUTPUT);
    led_red_write(led_red_on);
}

void led_identify_task(void *_args) {
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            led_green_write(true);
            led_red_write(false);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            led_green_write(false);
            led_red_write(true);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    led_green_write(led_green_on);
    led_red_write(led_red_on);

    vTaskDelete(NULL);
}

void led_identify(homekit_value_t _value) {
    printf("Traffic Led Sensor identify\n");
    xTaskCreate(led_identify_task, "Traffic Led Sensor Identify", 128, NULL, 2, NULL);
}

homekit_value_t led_green_on_get() {
    return HOMEKIT_BOOL(led_green_on);
}

homekit_value_t led_red_on_get() {
    return HOMEKIT_BOOL(led_red_on);
}

void led_green_on_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }

    led_green_on = value.bool_value;
    led_green_write(led_green_on);
}

void led_red_on_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }

    led_red_on = value.bool_value;
    led_red_write(led_red_on);
}

homekit_characteristic_t led_green_characteristic = HOMEKIT_CHARACTERISTIC_(
    ON, false,
    .getter=led_green_on_get,
    .setter=led_green_on_set
);

homekit_characteristic_t led_red_characteristic = HOMEKIT_CHARACTERISTIC_(
    ON, false,
    .getter=led_red_on_get,
    .setter=led_red_on_set
);

homekit_characteristic_t temperature = HOMEKIT_CHARACTERISTIC_(CURRENT_TEMPERATURE, 0);
homekit_characteristic_t humidity    = HOMEKIT_CHARACTERISTIC_(CURRENT_RELATIVE_HUMIDITY, 0);

void temperature_sensor_task(void *_args) {
    gpio_set_pullup(SENSOR_PIN, false, false);

    float humidity_value, temperature_value;
    while (1) {
        bool success = dht_read_float_data(
            DHT_TYPE_DHT22, SENSOR_PIN,
            &humidity_value, &temperature_value
        );
        if (success) {
            temperature.value.float_value = temperature_value;
            humidity.value.float_value = humidity_value;

            homekit_characteristic_notify(&temperature, HOMEKIT_FLOAT(temperature_value));
            homekit_characteristic_notify(&humidity, HOMEKIT_FLOAT(humidity_value));
        } else {
            printf("Couldnt read data from sensor\n");
        }

        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}

void temperature_sensor_init() {
    xTaskCreate(temperature_sensor_task, "Temperatore Sensor", 256, NULL, 2, NULL);
}

void increment_led_status(int value) {
  int status = 0;
  if (led_green_on) {
    status += 1;
  }
  if (led_red_on) {
    status += 2;
  }
  status = status + value;
  led_green_on = (status & 1) == 1;
  led_red_on = (status & 2) == 2;
}

void button_callback(button_event_t event, void *context) {
    switch (event) {
        case button_event_single_press:
            printf("single press\n");
            increment_led_status(1);
            led_green_write(led_green_on);
            led_red_write(led_red_on);
            homekit_characteristic_notify(&led_green_characteristic, led_green_on_get());
            homekit_characteristic_notify(&led_red_characteristic, led_red_on_get());
            break;
        case button_event_double_press:
            printf("double press\n");
            increment_led_status(-1);
            led_green_write(led_green_on);
            led_red_write(led_red_on);
            homekit_characteristic_notify(&led_green_characteristic, led_green_on_get());
            homekit_characteristic_notify(&led_red_characteristic, led_red_on_get());
            break;
        case button_event_long_press:
            printf("long press\n");
            led_green_on = false;
            led_red_on = false;
            led_green_write(led_green_on);
            led_red_write(led_red_on);
            homekit_characteristic_notify(&led_green_characteristic, led_green_on_get());
            homekit_characteristic_notify(&led_red_characteristic, led_red_on_get());
            break;
        default:
            printf("unknown button event: %d\n", event);
    }
}


homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Traffic LED Sensor"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Tech From The Shed"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "237A2BABF19D"),
            HOMEKIT_CHARACTERISTIC(MODEL, "Traffic Led Sensor"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.0.1"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, led_identify),
            NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .primary=true, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Green LED"),
            &led_green_characteristic,
            NULL
        }),
        HOMEKIT_SERVICE(LIGHTBULB, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Red LED"),
            &led_red_characteristic,
            NULL
        }),
        HOMEKIT_SERVICE(TEMPERATURE_SENSOR, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Temperature Sensor"),
            &temperature,
            NULL
        }),
        HOMEKIT_SERVICE(HUMIDITY_SENSOR, .characteristics=(homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Humidity Sensor"),
            &humidity,
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "111-11-111"
};

void on_wifi_ready() {
}

void user_init(void) {
    uart_set_baud(0, 115200);

    button_config_t button_config = BUTTON_CONFIG(
        button_active_low,
        .max_repeat_presses=2,
        .long_press_time=1000,
    );
    if (button_create(BUTTON_PIN, button_config, button_callback, NULL)) {
        printf("Failed to initialize button\n");
    }

    led_init();
    temperature_sensor_init();
    homekit_server_init(&config);
}
