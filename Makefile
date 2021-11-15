PROGRAM = traffic-led-sensor

EXTRA_COMPONENTS = \
  extras/dht \
	extras/http-parser \
	extras/dhcpserver \
	$(abspath ../../components/esp8266-open-rtos/wifi_config) \
	$(abspath ../../components/esp8266-open-rtos/cJSON) \
	$(abspath ../../components/common/button) \
	$(abspath ../../components/common/wolfssl) \
	$(abspath ../../components/common/homekit)

# DHT11 sensor pin
SENSOR_PIN ?= 5

# Green Led pin
GREEN_PIN ?= 2

# Red Led pin
RED_PIN ?= 4

# Program / Led Mode button
BUTTON_PIN ?= 0


FLASH_SIZE ?= 32

EXTRA_CFLAGS += -I../.. -DHOMEKIT_SHORT_APPLE_UUIDS -DSENSOR_PIN=$(SENSOR_PIN) -DGREEN_PIN=$(GREEN_PIN) -DRED_PIN=$(RED_PIN) -DBUTTON_PIN=$(BUTTON_PIN)

include $(SDK_PATH)/common.mk

monitor:
	$(FILTEROUTPUT) --port $(ESPPORT) --baud 115200 --elf $(PROGRAM_OUT)
