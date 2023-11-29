
#include "esp_err.h"

//Maksymalna liczba diód jakie chcemy sterowaæ w jednym ³añcuchu
#define WS_LED_CNT			35
//Okreœlenie numeru kana³u RMT, który bêdzie wykorzytsany do sterowania diód programowalnych
#define RMT_TX_CHANNEL 		0
//Zdefioniowanie pinu steruj¹cego diodami programowalnymi
#define GPIO				17

//Definiowanie czasów trwania poszczególnych stanów przy wysy³aniu pojedyñczych bitów
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


//Typ taœmy LED
typedef struct led_strip_s led_strip_t;

//Typ urz¹dzenia z taœm¹ LED
typedef void *led_strip_dev_t;


// Deklaracja struktury dla taœmy LED
struct led_strip_s {
    /* Ustaw kolor RGB dla wskazanej diody LED
     	 * @param strip: wska¿nik do ³acuch z diodami LED
     	 * @param index: numer kolejny diody do zmiany ustawieñ
     	 * @param red: wartoœæ koloru czerwonego
     	 * @param green: wartoœæ koloru zielonego
     	 * @param blue: bwartoœæ koloru niebieskiego
    * @zwracane wartoœci
    	- ESP_OK: Set RGB for a specific pixel successfully
     	- ESP_ERR_INVALID_ARG: Set RGB for a specific pixel failed because of invalid parameters
    	- ESP_FAIL: Set RGB for a specific pixel failed because other error occurred
    */
    esp_err_t (*set_pixel)(led_strip_t *strip, uint32_t index, uint32_t red, uint32_t green, uint32_t blue);

    /* Odœwie¿ pamiêæ kolorów diód
       * @param strip: wska¿nik na ³acuch z diodami LED
       * @param timeout_ms: wartoœæ opóŸnienia czasowego
    * @zwracane wartoœci
    	- ESP_OK: Refresh successfully
   		- ESP_ERR_TIMEOUT: Refresh failed because of timeout
    	- ESP_FAIL: Refresh failed because some other error occurred
    * @uwaga:
   		Po odœwie¿eniu danych nale¿y wywo³aæ funkcjê do wysy³ania danych do diód
    */
    esp_err_t (*refresh)(led_strip_t *strip, uint32_t timeout_ms);

    /* Zgaszenie wszystkich diód
     	* @param strip: wska¿nik na ³añcuch z diodami LED
     	* @param timeout_ms: czas wykonania opóxnienia przy zerowaniu diód
     * @zwracane wartoœci
   		- ESP_OK: Clear LEDs successfully
   		- ESP_ERR_TIMEOUT: Clear LEDs failed because of timeout
    	- ESP_FAIL: Clear LEDs failed because some other error occurred
    */
    esp_err_t (*clear)(led_strip_t *strip, uint32_t timeout_ms);

    /*Zwolnienie zasobów pamiêci zajêtych przez diody
     	* @param strip: wska¿nik na ³añcuch z diodami LED
    * @zwracane wartoœci
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


// Okrêœlenie w³asnego typu ³añcucha LED do wykorzytsania w programie
typedef struct {
    uint32_t max_leds;   // Maksymalna liczba diód w ³añcuchu
    led_strip_dev_t dev; /*!< LED strip device (e.g. RMT channel, PWM channel, etc) */
} led_strip_config_t;

// Domyœlna konfiguracja dla ³añcucha LED
#define LED_STRIP_DEFAULT_CONFIG(number, dev_hdl) \
    {                                             \
        .max_leds = number,                       \
        .dev = dev_hdl,                           \
    }

/* Instalacja nowego driwera dla diód WS2812 w oparciu o sprzêtowy RTM
	 @param config: Odwo³anie do typu opisuj¹cego ³añcuch LED
 * @zwracane wartoœci
 	 Powi¹zanie typu ze wskaŸnikiem lub NULL
*/
led_strip_t *led_strip_new_rmt_ws2812(const led_strip_config_t *config);

/* Inicjowanie RMT sprzêtowego do obs³ugi sterowania diodami LED
 	 * @param[in] channel: numer kana³u RMT
 	 * @param[in] gpio: Numer pinu GPIO do wysy³ania danych steruj¹cych diodami
 	 * @param[in] led_num: Liczba diód po³¹czona w ³añcuchu
  * @zwracane wartoœci
 	 Powi¹zanie typu ze wskaŸnikiem lub NULL
 */
led_strip_t * led_strip_init(uint8_t channel, uint8_t gpio, uint16_t led_num);

/* Zwolnienie RMT z powi¹zanymim dioami LED
 	 * @param[in] strip: wska¿nik na ³añcuch z diodami LED
 * @zwracane wartoœci
	- ESP_OK
 	 - ESP_FAIL
 */
esp_err_t led_strip_denit(led_strip_t *strip);


//Inicjowanie diód programowalnych
led_strip_t * ProgrammableLED_init(uint8_t channel_rmt, uint8_t gpio, uint16_t cnt_led);

//Funkcja do zamiany palety braw z HSV na RGB dla diód programowalnych
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);

//Funkcja do zamiany palety barw z HSV na RGB na podstawie biblioteki z AVR
void hsv_to_rgb( uint8_t h, uint8_t s, uint8_t v, uint32_t *r, uint32_t *g, uint32_t *b );
