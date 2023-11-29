
#include "esp_err.h"

//Maksymalna liczba di�d jakie chcemy sterowa� w jednym �a�cuchu
#define WS_LED_CNT			35
//Okre�lenie numeru kana�u RMT, kt�ry b�dzie wykorzytsany do sterowania di�d programowalnych
#define RMT_TX_CHANNEL 		0
//Zdefioniowanie pinu steruj�cego diodami programowalnymi
#define GPIO				17

//Definiowanie czas�w trwania poszczeg�lnych stan�w przy wysy�aniu pojedy�czych bit�w
//     bit = 0				    bit = 1
//  +----+        |        +--------+    |
//  |    |        |        |        |    |
//  |    |        |        |        |    |
//  |    |        |        |        |    |
//  |    |        |        |        |    |
//  |    +--------+        |        +----+
// T0H_NS  T0L_NS			 T1H_NS T1L_NS
#define WS2812_T0H_NS (350)
#define WS2812_T0L_NS (900)
#define WS2812_T1H_NS (800)
#define WS2812_T1L_NS (450)
#define WS2812_RESET_US (280)


//Typ ta�my LED
typedef struct led_strip_s led_strip_t;

//Typ urz�dzenia z ta�m� LED
typedef void *led_strip_dev_t;


// Deklaracja struktury dla ta�my LED
struct led_strip_s {
    /* Ustaw kolor RGB dla wskazanej diody LED
     	 * @param strip: wska�nik do �acuch z diodami LED
     	 * @param index: numer kolejny diody do zmiany ustawie�
     	 * @param red: warto�� koloru czerwonego
     	 * @param green: warto�� koloru zielonego
     	 * @param blue: bwarto�� koloru niebieskiego
    * @zwracane warto�ci
    	- ESP_OK: Set RGB for a specific pixel successfully
     	- ESP_ERR_INVALID_ARG: Set RGB for a specific pixel failed because of invalid parameters
    	- ESP_FAIL: Set RGB for a specific pixel failed because other error occurred
    */
    esp_err_t (*set_pixel)(led_strip_t *strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue);

    /* Od�wie� pami�� kolor�w di�d
       * @param strip: wska�nik na �acuch z diodami LED
       * @param timeout_ms: warto�� op�nienia czasowego
    * @zwracane warto�ci
    	- ESP_OK: Refresh successfully
   		- ESP_ERR_TIMEOUT: Refresh failed because of timeout
    	- ESP_FAIL: Refresh failed because some other error occurred
    * @uwaga:
   		Po od�wie�eniu danych nale�y wywo�a� funkcj� do wysy�ania danych do di�d
    */
    esp_err_t (*refresh)(led_strip_t *strip, uint32_t timeout_ms);

    /* Zgaszenie wszystkich di�d
     	* @param strip: wska�nik na �a�cuch z diodami LED
     	* @param timeout_ms: czas wykonania op�xnienia przy zerowaniu di�d
     * @zwracane warto�ci
   		- ESP_OK: Clear LEDs successfully
   		- ESP_ERR_TIMEOUT: Clear LEDs failed because of timeout
    	- ESP_FAIL: Clear LEDs failed because some other error occurred
    */
    esp_err_t (*clear)(led_strip_t *strip, uint32_t timeout_ms);

    /*Zwolnienie zasob�w pami�ci zaj�tych przez diody
     	* @param strip: wska�nik na �a�cuch z diodami LED
    * @zwracane warto�ci
    	- ESP_OK: Free resources successfully
    	- ESP_FAIL: Free resources failed because error occurred
    */
    esp_err_t (*del)(led_strip_t *strip);
};

typedef struct {
    led_strip_t parent;
    rmt_channel_t rmt_channel;
    uint32_t strip_len;
    uint8_t buffer[0];
} STR_WSRBG;
//} ws2812_t;


// Okr�lenie w�asnego typu �a�cucha LED do wykorzytsania w programie
typedef struct {
    uint32_t max_leds;   // Maksymalna liczba di�d w �a�cuchu
    led_strip_dev_t dev; /*!< LED strip device (e.g. RMT channel, PWM channel, etc) */
} led_strip_config_t;

// Domy�lna konfiguracja dla �a�cucha LED
#define LED_STRIP_DEFAULT_CONFIG(number, dev_hdl) \
    {                                             \
        .max_leds = number,                       \
        .dev = dev_hdl,                           \
    }

/* Instalacja nowego driwera dla di�d WS2812 w oparciu o sprz�towy RTM
	 @param config: Odwo�anie do typu opisuj�cego �a�cuch LED
 * @zwracane warto�ci
 	 Powi�zanie typu ze wska�nikiem lub NULL
*/
led_strip_t *led_strip_new_rmt_ws2812(const led_strip_config_t *config);

/* Inicjowanie RMT sprz�towego do obs�ugi sterowania diodami LED
 	 * @param[in] channel: numer kana�u RMT
 	 * @param[in] gpio: Numer pinu GPIO do wysy�ania danych steruj�cych diodami
 	 * @param[in] led_num: Liczba di�d po��czona w �a�cuchu
  * @zwracane warto�ci
 	 Powi�zanie typu ze wska�nikiem lub NULL
 */
led_strip_t * led_strip_init(uint8_t channel, uint8_t gpio, uint16_t led_num);

/* Zwolnienie RMT z powi�zanymim dioami LED
 	 * @param[in] strip: wska�nik na �a�cuch z diodami LED
 * @zwracane warto�ci
	- ESP_OK
 	 - ESP_FAIL
 */
esp_err_t led_strip_denit(led_strip_t *strip);


//Inicjowanie di�d programowalnych
led_strip_t * ProgrammableLED_init(uint8_t channel_rmt, uint8_t gpio, uint16_t cnt_led);

//Funkcja do zamiany palety braw z HSV na RGB dla di�d programowalnych
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);

//Funkcja do zamiany palety barw z HSV na RGB na podstawie biblioteki z AVR
void hsv_to_rgb( uint8_t h, uint8_t s, uint8_t v, uint32_t *r, uint32_t *g, uint32_t *b );
