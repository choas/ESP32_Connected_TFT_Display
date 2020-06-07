/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "tftspi.h"
#include "tft.h"

#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "tcpip_adapter.h"
#include "protocol_examples_common.h"

#include "mqtt_client.h"
#include "esp_log.h"

// ==========================================================
// Define which spi bus to use TFT_VSPI_HOST or TFT_HSPI_HOST
#define SPI_BUS TFT_HSPI_HOST
// ==========================================================

//-------------------------------------------------------------------
// Mandelbrot calculation
//
// note: this code is based on the C64 BASIC code
//       https://semioriginalthought.blogspot.com/2012/04/how-to-simply-create-mandelbrot-set-on.html
//
double xl = -2.0;
double xu = 0.5;
double yl = -1.1;
double yu = 1.1;

int reps = 40;
int width = 40;
int height = 25;

int calcMandel(int i, int j)
{

	double xinc = (xu - xl) / width;
	double yinc = (yu - yl) / height;

	double nreal = xl + i * xinc;
	double nimg = yl + j * yinc;
	double rz = 0.0;
	double iz = 0.0;
	double r2z = 0.0;
	double i2z = 0.0;
	for (int k = 1; k <= reps; k++)
	{

		r2z = rz * rz - iz * iz;
		i2z = 2 * rz * iz;
		rz = r2z + nreal;
		iz = i2z + nimg;

		if ((rz * rz + iz * iz) > 4)
		{
			return k;
		}
	}
	return 0;
}

static color_t getColor(int c) {
		color_t color;
		color.r = 10;
		color.g = 10;
		color.b = c * 6;
		return color;
}


//-----------------------
static void mandelbrot_demo()
{

	const int radius = 3;

	int px = 0;
	int py = 0;
	while (px < 27)
	{
		int c = calcMandel(py, px);
		TFT_fillCircle(px * 8 + 14, py * 8 + 4, radius, getColor(c));

		py += 1;
		if (py > 40)
		{
			py = 0;
			px += 1;
		}
	}
	vTaskDelay(1000 / portTICK_RATE_MS);

	xl = -0.6;
	xu = -0.5;
	yl = -0.6;
	yu = -0.7;

	xl = -0.566500;
	yu = -0.666500;

	// -0.566500 -0.598000 -0.698000 -0.666500

	int d = 10;

	double in = 0.000005;

	while(1) {
//	for (reps = 40; reps < 220; reps += 1) {


		//xl -= in * -d;
		xu += in * -d;
		yl += in * -d;
		//yu -= in * -d;


		reps += d;
		if (reps > 2000 || reps < 30) {
			d *= -1;
		}

		printf("%d: %f %f %f %f\n", reps, xl, xu, yl, yu);

		px = 0;
		py = 0;
		while (px < 27)
		{
			int c = calcMandel(py, px);
			TFT_fillCircle(px * 8 + 14, py * 8 + 4, radius, getColor(c));

			py += 1;
			if (py > 40)
			{
				py = 0;
				px += 1;
			}
		}

		vTaskDelay(10 / portTICK_RATE_MS);
	}

}



static const char *TAG = "MQTT_EXAMPLE";


static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
    };
#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.uri, "FROM_STDIN") == 0) {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128) {
            int c = fgetc(stdin);
            if (c == '\n') {
                line[count] = '\0';
                break;
            } else if (c > 0 && c < 127) {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void mqtt_main()
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    mqtt_app_start();
}









//=============
void app_main()
{

	mqtt_main();



	// ========  PREPARE DISPLAY INITIALIZATION  =========

	esp_err_t ret;

	// === SET GLOBAL VARIABLES ==========================

	// ===================================================
	// ==== Set display type                         =====
	tft_disp_type = DEFAULT_DISP_TYPE;
	//tft_disp_type = DISP_TYPE_ILI9341;
	//tft_disp_type = DISP_TYPE_ILI9488;
	//tft_disp_type = DISP_TYPE_ST7735B;
	// ===================================================

	// ===================================================
	// === Set display resolution if NOT using default ===
	// === DEFAULT_TFT_DISPLAY_WIDTH &                 ===
	// === DEFAULT_TFT_DISPLAY_HEIGHT                  ===
	_width = DEFAULT_TFT_DISPLAY_WIDTH;		// smaller dimension
	_height = DEFAULT_TFT_DISPLAY_HEIGHT; // larger dimension
	//_width = 128;  // smaller dimension
	//_height = 160; // larger dimension
	// ===================================================

	// ===================================================
	// ==== Set maximum spi clock for display read    ====
	//      operations, function 'find_rd_speed()'    ====
	//      can be used after display initialization  ====
	max_rdclock = 8000000;
	// ===================================================

	// ====================================================================
	// === Pins MUST be initialized before SPI interface initialization ===
	// ====================================================================
	TFT_PinsInit();

	// ====  CONFIGURE SPI DEVICES(s)  ====================================================================================

	spi_lobo_device_handle_t spi;

	spi_lobo_bus_config_t buscfg = {
			.miso_io_num = PIN_NUM_MISO, // set SPI MISO pin
			.mosi_io_num = PIN_NUM_MOSI, // set SPI MOSI pin
			.sclk_io_num = PIN_NUM_CLK,	 // set SPI CLK pin
			.quadwp_io_num = -1,
			.quadhd_io_num = -1,
			.max_transfer_sz = 6 * 1024,
	};
	spi_lobo_device_interface_config_t devcfg = {
			.clock_speed_hz = 8000000,				 // Initial clock out at 8 MHz
			.mode = 0,												 // SPI mode 0
			.spics_io_num = -1,								 // we will use external CS pin
			.spics_ext_io_num = PIN_NUM_CS,		 // external CS pin
			.flags = LB_SPI_DEVICE_HALFDUPLEX, // ALWAYS SET  to HALF DUPLEX MODE!! for display spi
	};

	// ====================================================================================================================

	vTaskDelay(500 / portTICK_RATE_MS);
	printf("\r\n==============================\r\n");
	printf("Pins used: miso=%d, mosi=%d, sck=%d, cs=%d\r\n", PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);
	printf("==============================\r\n\r\n");

	// ==================================================================
	// ==== Initialize the SPI bus and attach the LCD to the SPI bus ====

	ret = spi_lobo_bus_add_device(SPI_BUS, &buscfg, &devcfg, &spi);
	assert(ret == ESP_OK);
	printf("SPI: display device added to spi bus (%d)\r\n", SPI_BUS);
	disp_spi = spi;

	// ==== Test select/deselect ====
	ret = spi_lobo_device_select(spi, 1);
	assert(ret == ESP_OK);
	ret = spi_lobo_device_deselect(spi);
	assert(ret == ESP_OK);

	printf("SPI: attached display device, speed=%u\r\n", spi_lobo_get_speed(spi));
	printf("SPI: bus uses native pins: %s\r\n", spi_lobo_uses_native_pins(spi) ? "true" : "false");

	// ================================
	// ==== Initialize the Display ====

	printf("SPI: display init...\r\n");
	TFT_display_init();
	printf("OK\r\n");

	// ---- Detect maximum read speed ----
	max_rdclock = find_rd_speed();
	printf("SPI: Max rd speed = %u\r\n", max_rdclock);

	// ==== Set SPI clock used for display operations ====
	spi_lobo_set_speed(spi, DEFAULT_SPI_CLOCK);
	printf("SPI: Changed speed to %u\r\n", spi_lobo_get_speed(spi));

	printf("\r\n-----------------------\r\n");
	printf("Mandelbrot demo started\r\n");
	printf("-----------------------\r\n");

	font_rotate = 0;
	text_wrap = 0;
	font_transparent = 0;
	font_forceFixed = 0;
	gray_scale = 0;
	TFT_setGammaCurve(DEFAULT_GAMMA_CURVE);
	TFT_setRotation(PORTRAIT);
	TFT_setFont(DEFAULT_FONT, NULL);
	TFT_resetclipwin();

	//=========
	// Run demo
	//=========
	mandelbrot_demo();
}
