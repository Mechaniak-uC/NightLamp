/*
 * Biblioteka do obs³ugi diód Programowalnych WS2812
 *
 */
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>


#include "esp_log.h"
#include "esp_attr.h"

#include "driver/rmt.h"

#include "WS2812.h"



static const char *TAG = "ws2812";
#define STRIP_CHECK(a, str, goto_tag, ret_value, ...)                             \
    do                                                                            \
    {                                                                             \
        if (!(a))                                                                 \
        {                                                                         \
            ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            ret = ret_value;                                                      \
            goto goto_tag;                                                        \
        }                                                                         \
    } while (0)



//Zmienne do obs³ugi wysy³ania bitów do diód programowalnych
static uint32_t ws2812_t0h_ticks = 0;
static uint32_t ws2812_t1h_ticks = 0;
static uint32_t ws2812_t0l_ticks = 0;
static uint32_t ws2812_t1l_ticks = 0;


/* Konwersja danych RGB na format RMT
	@param[in] src: dane Ÿród³owe do konwersj na format RMT
	@param[in] dest: miejsce gdzie zapisany zostanie wynik konwersji
	@param[in] src_size: rozmiar wielkoœci danych wejœciowych
	@param[in] wanted_num: number of RMT items that want to get
	@param[out] translated_size: liczba danych Ÿród³owych, które zosta³y przekonwertowane
	@param[out] item_num: liczba pozycji RMT, które s¹ konwertowane z danych Ÿród³owych
 */
static void IRAM_ATTR ws2812_rmt_adapter(const void *src, rmt_item32_t *dest, size_t src_size,
        size_t wanted_num, size_t *translated_size, size_t *item_num)
{
    if (src == NULL || dest == NULL) {
        *translated_size = 0;
        *item_num = 0;
        return;
    }
    const rmt_item32_t bit0 = {{{ ws2812_t0h_ticks, 1, ws2812_t0l_ticks, 0 }}}; //Logical 0
    const rmt_item32_t bit1 = {{{ ws2812_t1h_ticks, 1, ws2812_t1l_ticks, 0 }}}; //Logical 1
    size_t size = 0;
    size_t num = 0;
    uint8_t *psrc = (uint8_t *)src;
    rmt_item32_t *pdest = dest;
    while (size < src_size && num < wanted_num) {
        for (int i = 0; i < 8; i++) {
            // MSB first
            if (*psrc & (1 << (7 - i))) {
                pdest->val =  bit1.val;
            } else {
                pdest->val =  bit0.val;
            }
            num++;
            pdest++;
        }
        size++;
        psrc++;
    }
    *translated_size = size;
    *item_num = num;
}

static esp_err_t ws2812_set_pixel(led_strip_t *strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue)
{

    STR_WSRBG *ws2812 = __containerof(strip, STR_WSRBG, parent);

    uint32_t start = index * 3;
    // In thr order of GRB
    ws2812->buffer[start + 0] = green & 0xFF;
    ws2812->buffer[start + 1] = red & 0xFF;
    ws2812->buffer[start + 2] = blue & 0xFF;
    return ESP_OK;
}



static esp_err_t ws2812_refresh(led_strip_t *strip, uint32_t timeout_ms)
{
//    esp_err_t ret = ESP_OK;
    STR_WSRBG *ws2812 = __containerof(strip, STR_WSRBG, parent);
//    STRIP_CHECK(rmt_write_sample(ws2812->rmt_channel, ws2812->buffer, ws2812->strip_len * 3, true) == ESP_OK,
//                "transmit RMT samples failed", err, ESP_FAIL);
    rmt_write_sample(ws2812->rmt_channel, ws2812->buffer, ws2812->strip_len * 3, true);
    return rmt_wait_tx_done(ws2812->rmt_channel, pdMS_TO_TICKS(timeout_ms));
//err:
//    return ret;
}

static esp_err_t ws2812_clear(led_strip_t *strip, uint32_t timeout_ms)
{
	STR_WSRBG *ws2812 = __containerof(strip, STR_WSRBG, parent);
    // Write zero to turn off all leds
    memset(ws2812->buffer, 0, ws2812->strip_len * 3);
    return ws2812_refresh(strip, timeout_ms);
}

static esp_err_t ws2812_del(led_strip_t *strip)
{
	STR_WSRBG *ws2812 = __containerof(strip, STR_WSRBG, parent);
    free(ws2812);
    return ESP_OK;
}

led_strip_t *led_strip_new_rmt_ws2812(const led_strip_config_t *config)
{
    led_strip_t *ret = NULL;
//    STRIP_CHECK(config, "configuration can't be null", err, NULL);

    // 24 bits per led
    uint32_t ws2812_size = sizeof(STR_WSRBG) + config->max_leds * 3;
    STR_WSRBG *ws2812 = calloc(1, ws2812_size);
    STRIP_CHECK(ws2812, "request memory for ws2812 failed", err, NULL);

    uint32_t counter_clk_hz = 0;
    STRIP_CHECK(rmt_get_counter_clock((rmt_channel_t)config->dev, &counter_clk_hz) == ESP_OK,
                "get rmt counter clock failed", err, NULL);
    // ns -> ticks
    float ratio = (float)counter_clk_hz / 1e9;
    ws2812_t0h_ticks = (uint32_t)(ratio * WS2812_T0H_NS);
    ws2812_t0l_ticks = (uint32_t)(ratio * WS2812_T0L_NS);
    ws2812_t1h_ticks = (uint32_t)(ratio * WS2812_T1H_NS);
    ws2812_t1l_ticks = (uint32_t)(ratio * WS2812_T1L_NS);

    // set ws2812 to rmt adapter
    rmt_translator_init((rmt_channel_t)config->dev, ws2812_rmt_adapter);

    ws2812->rmt_channel = (rmt_channel_t)config->dev;
    ws2812->strip_len = config->max_leds;

    ws2812->parent.set_pixel = ws2812_set_pixel;
    ws2812->parent.refresh = ws2812_refresh;
    ws2812->parent.clear = ws2812_clear;
    ws2812->parent.del = ws2812_del;

    return &ws2812->parent;
err:
    return ret;
}

led_strip_t * led_strip_init(uint8_t channel, uint8_t gpio, uint16_t led_num)
{
    static led_strip_t *pStrip;

    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(gpio, channel);
    // set counter clock to 40MHz
    config.clk_div = 2;

    rmt_config(&config);
    rmt_driver_install(config.channel, 0, 0);

    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(led_num, (led_strip_dev_t)config.channel);

    pStrip = led_strip_new_rmt_ws2812(&strip_config);

//    if ( !pStrip ) {
//        ESP_LOGE(TAG, "install WS2812 driver failed");
//        return NULL;
//    }

    // Clear LED strip (turn off all LEDs)
    pStrip->clear(pStrip, 100);

    return pStrip;
}

esp_err_t led_strip_denit(led_strip_t *strip)
{
	STR_WSRBG *ws2812 = __containerof(strip, STR_WSRBG, parent);
    ESP_ERROR_CHECK(rmt_driver_uninstall(ws2812->rmt_channel));
    return strip->del(strip);
}



/**
 * @brief Init the RMT peripheral and LED strip configuration.
 *
 * @param[in] channel: RMT peripheral channel number.
 * @param[in] gpio: GPIO number for the RMT data output.
 * @param[in] led_num: number of addressable LEDs.
 * @return
 *      LED strip instance or NULL
 */
led_strip_t * ProgrammableLED_init(uint8_t channel_rmt, uint8_t gpio, uint16_t cnt_led)
{
    static led_strip_t *pStrip;

    //definiowanie portu wyjœciowego procesora i kana³u RMT, na którym maja pracowaæ diody
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(gpio, channel_rmt);
    config.clk_div = 2;	// podziel aktualn¹ czestotliwoœæ procesora przez dwa aby otrzymaæ czêstotliwoœæ równ¹ 40MHz

    rmt_config(&config);
    rmt_driver_install(config.channel, 0, 0);	//Zainstalowanie driwera do obs³ugi diód

    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(cnt_led, (led_strip_dev_t)config.channel);

    pStrip = led_strip_new_rmt_ws2812(&strip_config);

//    if ( !pStrip ) {
//        ESP_LOGE(TAG, "install WS2812 driver failed");
//        return NULL;
//    }

    // zerój wszystkie diody na zadeklarowanym pasku LED (turn off all LEDs)
   pStrip->clear(pStrip, 100);

    return pStrip;
}


// Funkcja do konwersji barw z palety HSV na RGB
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

//Funkcja na podstawie biblioteki AVR
void hsv_to_rgb( uint8_t h, uint8_t s, uint8_t v, uint32_t *r, uint32_t *g, uint32_t *b )
{
	uint16_t region=0, remainder, p, q, t;

	if (s == 0)
	{
		*r = *g = *b = v;
	} else region = h*6 /256;

	remainder = (h*6)%256;

	p = (v * (255 - s))/256;
	q = (v * (255 - (s * remainder)/256))/256;
	t = (v * (255 - (s * (255 - remainder))/256))/256;

	switch( region )
	{
	case 0:
		*r = v; *g = t; *b = p;
		break;
	case 1:
		*r = q; *g = v; *b = p;
		break;
	case 2:
		*r = p; *g = v; *b = t;
		break;
	case 3:
		*r = p; *g = q; *b = v;
		break;
	case 4:
		*r = t; *g = p; *b = v;
		break;
	case 5:
		*r = v; *g = p; *b = q;
		break;
	}

}
