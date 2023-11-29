/*
 * LAMPKA RGB LED & Touch Button & WiFi for ESP32 RTOS
 *
 * autor: Andrzej Bałazy
 * uwagi:
 * 	  -W lampce działają czasowo zmieniające się efekty świetlne, których czas na razie można regulować
 * 	   programowo, kolejność efektów jest z góry ustalona i nie można jej zmieniać
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "FreeRTOSConfig.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/i2c.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"

#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <errno.h>

#include "lwip/dns.h"

#include "esp_err.h"
#include "esp_log.h"

#include <time.h>
#include "lwip/apps/sntp.h"
#include <sys/time.h>

//-----------------------------------------

#include "mk_wifi.h"
#include "mk_tools.h"
#include "mk_i2c.h"
#include "mk_glcd_base.h"

//#include "mk_touch.h"
#include "driver/touch_sensor.h"
#include "driver/rmt.h"
#include "WS2812.h"

//static const char *TAG = "example";

#define RMT_TX_CHANNEL 0	//Numer kanału RMT na jakim mają działać diody

#define MOC1	50
#define MOC2	180

//TaskHandle_t Handle_magicLED;
#define LED1_GPIO 		2


#define TOUCH_ADC_CHECK_MODE		0

//Zdefioniowanie do jakiego portu pod��czony jest czujnik dotyku
#define TOUCH_GPIO	9

//Zdefiniowanie wyj�cia dla diody led, kt�ry s�u�y na razie do test�w
#define LED_UP_GPIO		19
#define LED_SIDE_GPIO	21

//Zdefiniowanie warto�� anlogowych zadzia�ania przycisku dotykowego
#define ADC_MIN			520 // 280	// 399
#define ADC_MAX			530  //380 // 401

//Utworzenie uchwytu do kolejki obs�uguj�cej jakie diody w lampce maj� by� aktywne
QueueHandle_t Q_LED_Blink;

//Utworzenie uchwytu do kolejki obs�uguj�cej przej�cia czasowe pomi�dzy efektami �wietlnymi
QueueHandle_t Q_Licznik;
QueueHandle_t Q_Licznik1;


/*
 * OBS�UGA DI�D PROGRAMOWALNYCH WS2812B
 */

//Obs�uga zdarzenia zwi�zanego z diodami programowalnymi WS2812B
void magicLED( void * arg )
{
	/*
	*Skalowanie warto�ci HSV
	*H:		0-255; 	0-Red; 	42-Yellow;	85-Green;	128-Aqua;	171-Blue;	214-Magenta;
	*S:		0-255;	0-tylko bia�a barwa; 	255-pe�ne kolory
	*V:		0-255;	0-brak �wiat�a; 		255-maksymalna jasno�c
	*/
	//Zmienne do odwzorowania kolor�w RGB na diodach
	uint32_t red = 0, green = 0, blue = 0 ;
	//Zmienen do przechowania warto�ci z zakresu platey barw HSV
	uint8_t hue = 0;
//	uint16_t start_rgb = 0;
	//Zmienne do obs�ugi p�tli w poszczeg�lnych efektach �wietlnych reazliowanych na diodach LED
	uint8_t j, krok, k;
	uint16_t i;

	//Zmienne do obs�ugi czasowych przej�c mi�dzy efektami
	//uint8_t wzorce_przejsc[] = {A,B,C,D,E};
	// A - czas w sekundach wy�wietlania EFEKTU - 1
	// B - czas w sekundach wy�wietlania EFEKTU - 2
	// C - czas w sekundach wy�wietlania EFEKTU - 3
	// D - czas w sekundach wy�wietlania EFEKTU - 4
	uint16_t wzorce_przejsc[] = { 60, 60, 60, 60 };
	uint16_t przejscia[4];	//czasy przej�cia pomi�dzy klejnymi efektami

//	int dane=0;
		BaseType_t xStatus;	//Zmienna zwracana przy odbiorze danych z kolejki

	uint16_t q_licznik, q_licznik1;	//os�uga kolejki -Aktualna warto�� czasu

	uint8_t q_tryb=0;	//obs�uga kolejki z trybem pracy touch pada
//	BaseType_t xStatus;	//Zmienna zwracana przy odbiorze danych z kolejki

	//Utworzenie wska�nika do di�d led oraz zainicjowanie wysy�ania danych do di�d LED
	led_strip_t *pasek_LED = ProgrammableLED_init( RMT_TX_CHANNEL, GPIO, WS_LED_CNT);

	//Obliczenie czas�w przej�� pomi�dzy efektami �wietlnymi
	for( uint8_t i=0; i<4; i++ ) {

		if( i== 0 )  przejscia[i] =  wzorce_przejsc[i]*10;
		else przejscia[i] =  przejscia[i-1] + wzorce_przejsc[i]*10;
	}

//	xStatus = xQueueReceive( Q_Licznik, &dane, pdMS_TO_TICKS( 1000 ) );

//	printf( "tryb: %d\n", przejscia[0] );
//	printf( "tryb: %d\n", przejscia[1] );
//	printf( "tryb: %d\n", przejscia[2] );
//	printf( "tryb: %d\n", przejscia[3] );

	while (true) {

		xStatus = xQueueReceive( Q_Licznik, &q_licznik, pdMS_TO_TICKS( 300000 ) );

//		if( xStatus == pdPASS ) {
//
//			printf( "tryb: %d\n", q_licznik );
//		}

		if( q_licznik <= przejscia[0] ) {
			/*
			 * EFEKT - 1
			 * Wszyskie diody maj� identyczny zmieniaj�cy si� powoli kolor
			 */
			//Odebranie danych z kolejki w jakim trybie ma pracowa� lampka
			xQueueReceive( Q_LED_Blink, &q_tryb, 0 );

			//Gdy jest wybrany odpowiedni tryb wy��cz boczne pod�wietlenie lampki
			if( q_tryb == 0 || q_tryb == 2 ) hsv_to_rgb( hue, 0, 0, &red, &green, &blue);
			//Gdy warunek spe�niony w��cz boczne pod�wietlenie lampki
			if( q_tryb == 1 || q_tryb == 3 ) hsv_to_rgb( hue, 255, MOC1, &red, &green, &blue);
			//Ustawiamy identyczny, zmieniaj�cy sie kolor na podstawie lampki
			for( j=0; j<28; j++) pasek_LED->set_pixel(pasek_LED, j, red, green, blue);

			//Gdy jest wybrany odpowiedni tryb wy��cz pod�wietlenie grafiki w lampce
			if( q_tryb == 0 || q_tryb == 3 ) hsv_to_rgb( hue, 0, 0, &red, &green, &blue);
			//Gdy warunek spe�niony w��cz boczne pod�wietlenie lampki
			if( q_tryb == 1 || q_tryb == 2 ) hsv_to_rgb( hue, 255, MOC2, &red, &green, &blue);
			//Ustaw owpowienie warto�ci danych dla di�d z pod�wietlenia grafiki
			for( j=28; j<35; j++ ) pasek_LED->set_pixel(pasek_LED, j, red, green, blue);

			// Wy�lij dane z pami�ci do di�d RGB
			pasek_LED->refresh(pasek_LED, 0);
			vTaskDelay(10);	//czas op�nienia 100ms
			hue++;	//Zmiana koloru co jeden obr�t p�tli o 1 warto�� w g�r� zgodnie z palet� barw HSV

		} //else

		if( q_licznik > przejscia[0] && q_licznik <= przejscia[1] ) {

			/*
			 * EFEKT - 2
			 * Jednolite, zmieniaj�ce si� pod�wietlenie grafiki, kt�rego kolor przesuwa si� warstwami
			 * w d� lampki
			 */
			krok = 255 / 7;	//Okre�lenie odst�pu kolor�w na poszczeg�lnych paskach w lampce
			for( j=0; j<255; j++){	//p�tla do p�ynnej zmiany kolor�w na poszczeg�lnych paskach

				krok += j;	//p�ynna zmiana koloru dla tego efektu
				for( i=0; i<7; i++ ){	//Wyliczanie kolejnych di�d w ka�dym z bocznych pask�w

					hue = krok + (255/7)*i;	//Wyliczenie koloru dla ka�dego wiersza
					//Ustawiamy wyliczony kolor dla ka�dego wiersza lampki (Zamiana koloru z HSV na RGB)
					//Gdy jest wybrany odpowiedni tryb wy��cz boczne pod�wietlenie lampki
					if( q_tryb == 0 || q_tryb == 2 ) hsv_to_rgb( hue, 0, 0, &red, &green, &blue);
					//Gdy warunek spe�niony w��cz boczne pod�wietlenie lampki
					if( q_tryb == 1 || q_tryb == 3 ) hsv_to_rgb( hue, 255, MOC1, &red, &green, &blue);
					//Ustawienie danych w pami�ci dla odpowiednich di�d LED na bokach lampki
					for( k=0; k<4; k++ ) pasek_LED->set_pixel(pasek_LED, k*7+i, red, green, blue);
				}
				hue = (255/7)+j;	//Obliczenie koloru dla di�d do pod�wietlenia grafiki
				//Gdy jest wybrany odpowiedni tryb wy��cz pod�wietlenie grafiki w lampce
				if( q_tryb == 0 || q_tryb == 3 ) hsv_to_rgb( hue, 0, 0, &red, &green, &blue);
				//Gdy warunek spe�niony w��cz boczne pod�wietlenie lampki
				if( q_tryb == 1 || q_tryb == 2 ) hsv_to_rgb( hue, 255, MOC2, &red, &green, &blue);
				//Ustawienie danych w pami�ci dla odpowiedniego pasku LED
				for( k=0; k<7; k++ ) pasek_LED->set_pixel(pasek_LED, 28+k, red, green, blue);

				// Wy�lij dane z pami�ci do di�d RGB
				pasek_LED->refresh(pasek_LED, 0);
				xQueueReceive( Q_LED_Blink, &q_tryb, 0 );
				xQueueReceive( Q_Licznik, &q_licznik, 0 );
				if( q_licznik > przejscia[1] ) break;
//				printf( "tryb: %d\n", q_tryb );	//Wy�wietl odebrana warto�� na terminalu
				vTaskDelay(8);	//czas op�nienia 50ms
				krok = 255 / 5;
			}

		}


		if( q_licznik > przejscia[1] && q_licznik <= przejscia[2] ) {

			/*
			 * EFEKT-3
			 * Pod�wietlenie grafiki zmienia si� p�ynnie, a na ka�dym boku lampki jest jednolity kolor inny na ka�dym
			 * boku i te kolory si� zmieniaj�
			 */
			krok = 255 / 5;	//Okre�lenie odst�pu kolor�w na poszczeg�lnych paskach w lampce
			for( j=0; j<255; j++){	//p�tla do p�ynnej zmiany kolor�w na poszczeg�lnych paskach

				krok += j;	//wyliczenie koloru na pasku 0
				for( i=0; i<5; i++ ){	//Wyliczanie kolejnych di�d w ka�dym z pask�w

					hue = krok + (255/5)*i;	//Wyliczenie koloru dla ka�dego paska
					hsv_to_rgb( hue, 255, 25, &red, &green, &blue);	//Zamiana koloru z HSV na RGB

					//Gdy jest wybrany odpowiedni tryb wy��cz pod�wietlenie grafiki w lampce
					if( (q_tryb == 0 || q_tryb == 3) && ( i == 4 )) hsv_to_rgb( hue, 0, 0, &red, &green, &blue);
					//Gdy warunek spe�niony w��cz boczne pod�wietlenie lampki
					if( (q_tryb == 1 || q_tryb == 2) && ( i == 4 )) hsv_to_rgb( hue, 255, MOC2, &red, &green, &blue);

					//Gdy jest wybrany odpowiedni tryb wy��cz boczne pod�wietlenie lampki
					if( (q_tryb == 0 || q_tryb == 2) && !( i == 4 )) hsv_to_rgb( hue, 0, 0, &red, &green, &blue);
					//Gdy warunek spe�niony w��cz boczne pod�wietlenie lampki
					if( (q_tryb == 1 || q_tryb == 3) && !( i == 4 )) hsv_to_rgb( hue, 255, MOC1, &red, &green, &blue);

					//Ustawienie danych w pami�ci dla odpowiedniego pasku LED
					for( k=0; k<7; k++ ) pasek_LED->set_pixel(pasek_LED, k+i*7, red, green, blue);
				}
				// Wy�lij dane z pami�ci do di�d RGB
				pasek_LED->refresh(pasek_LED, 0);
				xQueueReceive( Q_LED_Blink, &q_tryb, 0 );	//Odczytaj dane z kolejki
				xQueueReceive( Q_Licznik, &q_licznik, 0 );
				if( q_licznik > przejscia[2] ) break;
				vTaskDelay(8);	//czas op�nienia 100ms
				krok = 255 / 5;
			}
		}

		if( q_licznik > przejscia[2] && q_licznik <= przejscia[3] ) {

			/*
			 * EFEKT-4
			 * P�ynna zmiana kolor�w na wszystkich diodach lampki
			 */
			//Okre�lenie kroku do p�ynnego przej�cia pomi�dzy kolorami
			krok = 255 / (WS_LED_CNT);
			//Kolorowa t�cza na wy�wietlaczu
			for( j=0; j<255; j++){	//p�tla do p�ynnej zmiany kolor�w na wszystkich diodach led

				for( i=0; i<WS_LED_CNT; i++ ){	//p�tla do uzupe�nienia tablicy LED odpowiednimi kolorami

					hue = krok+j + krok*i;		//wskazywanie kolejnych kolor�w dla kolejnych di�d w �a�cuchu

					//Gdy jest wybrany odpowiedni tryb wy��cz pod�wietlenie grafiki w lampce
					if( (q_tryb == 0 || q_tryb == 3) && ( i > 27 )) hsv_to_rgb( hue, 0, 0, &red, &green, &blue);
					//Gdy warunek spe�niony w��cz pod�wietlenie grafiki w lampce
					if( (q_tryb == 1 || q_tryb == 2) && ( i > 27 )) hsv_to_rgb( hue, 255, MOC2, &red, &green, &blue);
					//Gdy jest wybrany odpowiedni tryb wy��cz boczne pod�wietlenie lampki
					if( (q_tryb == 0 || q_tryb == 2) && ( i < 28 )) hsv_to_rgb( hue, 0, 0, &red, &green, &blue);
					//Gdy warunek spe�niony w��cz boczne pod�wietlenie lampki
					if( (q_tryb == 1 || q_tryb == 3) && ( i < 28 )) hsv_to_rgb( hue, 255, MOC2, &red, &green, &blue);

					pasek_LED->set_pixel(pasek_LED, i, red, green, blue); //Ustawienie danych w pami�ci dla odpowiedniej diody LED
				}
				// Wy�lij dane z pami�ci do di�d RGB
				pasek_LED->refresh(pasek_LED, 0);
				xQueueReceive( Q_LED_Blink, &q_tryb, 0 );	//Odczytaj dane z kolejki
				xQueueReceive( Q_Licznik, &q_licznik, 0 );
				if( q_licznik > przejscia[3] ) break;
				//printf( "tryb: %d\n", q_tryb );	//Wy�wietl odebrana warto�� na terminalu
				vTaskDelay(10);	//czas op�nienia 100ms
			}
		}
		if( q_licznik > przejscia[3] ) {

			q_licznik1 = 0;
			xQueueSendToBack( Q_Licznik1, &q_licznik1, 0 );
		} else {
			q_licznik1 = 1;
			xQueueSendToBack( Q_Licznik1, &q_licznik1, 0 );
		}


	}	//koniec p�tli g�ownej zdarzenia
}	//koniec taska
/*
 * KONIEC OBS�UGI DI�D PROGRMOWALNYCH
 */


/*
 * OBS�UGA PRZYCISKU DOTYKOWEGO
 */

void key_t1( void ) { //-funkcja obs�ugi warto�ci analogowych z przycisku dotykowego

#if TOUCH_1_ADC_CHECK_MODE == 0
	static uint8_t sw, state, time_sw, loop_time, tryb;
	/* OPIS ZMIENNYCH
	 * sw		- s�u�y do jednokrotnej reakcji na zmian� stanu padu dotykowego
	 * state 	- aktualny stan urz�dzenia z uwzgl�dnieniem touch pada i do jednokrotnego wykonywania warunk�w
	 * 		"0" - stan pada poni�ej ustawionej warto�ci minimalnej
	 *		"1"	- stan poda powy�ej ustawionej warto�ci maksymalnej
	 * load		- aktualny rzeczywisty stan diody wyj�ciowej ("0"-OFF, "1"=ON)
	 * time_sw	- zmienna do odczytywania jaki aktualnie stan ma odliczanie czasu po naci�ni�ciu pada
	 * 		"0"-czas nie jest odliczany
	 * 		"1"-czas zosta� za��czony
	 * 		"2"-czas zosta� ponownie za��czony
	 * loop_time- odliczanie czasu na wykonanie prze��czenia di�d LED
	 * tryb		- w jakiej konfiguracji pracuje lampka
	 * 		"0"-lampka si� nie �wieci
	 * 		"1"-�wi�c� si� wszystkie diody lampki
	 * 		"2"-�wieci si� tylko pod�wietlenie grafiki
	 * 		"3"-�wiec� si� tylko boki lampki
	 */
#endif

	uint16_t data;	//Warto�� cyfrowa sygna�u analogowego odczytana z czujnika dotyku

	touch_pad_read(TOUCH_GPIO, &data);

	touch_pad_filter_start( 50 );
	touch_pad_read_filtered(TOUCH_GPIO, &data);

#if TOUCH_ADC_CHECK_MODE == 1
	printf( "touch1 data: %d\n", data );

#else

	//czy warto�� odczytana z pada mniejsza od ustalonej warto�ci oraz jego stan wynosi 0
	//zmienna "state" umo�liwia jednokrtone wykonanie rozkaz�w znajduj�cych si� w warunku
    if( data < ADC_MIN && state == 0 ) {
    	sw = 1;
    	state = 1;	//Ustawienie tej warto�ci zapobiega ci�g�ym wykonywaniu warunku
    }

    //warto�� analogowa z pada wzros�a i uk�ad jest uruchmoiony to ustaw zmienn� sw do wyzerowania
    //zmiennej state, �e prad by� naci�ni�ty
    if( data > ADC_MAX && state == 1 ) {
    	sw = 1;		//Ta linia chyba niepotrzebna
    	state =  0;
    }

    //warunek realizowany tylko jednokrotnie jak przycisk jest za��czany
    if( sw ) {
    	//Czy odmierzanie czasu jest za��czone i nie mien��a ustawiona warto�� czasu
    	switch( time_sw ){
    	case 0:
    		if( state ) {	//sprawd� czy klawisz jest aktualnie naci�ni�ty

	    		if( !tryb ) tryb = 1; else tryb = 0;	//Ustaw "1" w zmiennej pomocnicznej
	    		gpio_set_level( LED_UP_GPIO, tryb );	//Za�acz pod�wietlenie grafiki
	    		gpio_set_level( LED_SIDE_GPIO, tryb ); //Za��cz pod�wietlenie bok�w lampki

	    		//Wys�anie warto�ci wskazanej zmiennej do kolejki
	    		xQueueSendToBack( Q_LED_Blink, &tryb, 0 );

	    		//Je�eli zliczanie czasu nie zosta�o uruchomione to je za��cz
	    		time_sw = 1;	//za��cz zliczanie czasu
	    		loop_time = 100;	//Ustaw jak d�ugo trzeba czeka� na ponowne naci�ni�cie klawisza
	    	}
    		break;

    	case 1:
    		if( state ) {//sprawd� czy klawisz jest aktualnie naci�ni�ty

    			gpio_set_level( LED_UP_GPIO, 1 );	//Za�acz pod�wietlenie grafiki
    			gpio_set_level( LED_SIDE_GPIO, 0 );	//Wy��cz pod�wietlanie bok�w lampki
    			tryb = 2;
    			//Wys�anie warto�ci wskazanej zmiennej do kolejki
    			xQueueSendToBack( Q_LED_Blink, &tryb, 0 );

//    			gpio_set_level( LED_SIDE_GPIO, 0 ); //Wy��cz pod�wietlenie bok�w lampki
    			time_sw = 2;	//Ustaw tryb odliczania na 2
    			loop_time = 100;	//Ustaw jak d�ugo trzeba czeka� na ponowne naci�ni�cie klawisza
    		}
    		break;

    	case 2:
    		if( state ) {

    			gpio_set_level( LED_UP_GPIO, 0 );	//Wy��cz pod�wietlenie grafiki
    			gpio_set_level( LED_SIDE_GPIO, 1 ); //Za��cz pod�wietlenie bok�w lampki
    			tryb = 3;
    			//Wys�anie warto�ci wskazanej zmiennej do kolejki
    			xQueueSendToBack( Q_LED_Blink, &tryb, 0 );

    			time_sw = 3;	//zerowanie odliczania czasu -wy��cz odmierzanie czasu
    		}
    		break;
    	case 3:
    		if( state ) {

    			gpio_set_level( LED_UP_GPIO, 0 );	//Wy��cz pod�wietlenie grafiki
    			gpio_set_level( LED_SIDE_GPIO, 0 ); //Za��cz pod�wietlenie bok�w lampki
    			tryb = 0;
    			//Wys�anie warto�ci wskazanej zmiennej do kolejki
    			xQueueSendToBack( Q_LED_Blink, &tryb, 0 );

    			time_sw = 0;	//zerowanie odliczania czasu -wy��cz odmierzanie czasu
    		}
    		break;
    	}

    	sw = 0;	//Naci�ni�cie przycisku zosta�o wykonane - zeruj zmienn� pomocnicz�
//    	printf( "tryb: %d\n", tryb );
    }

    if( loop_time ) --loop_time; else time_sw = 0;

#endif
}


void touch_task(void *args) {	//-pocz�tek task obs�ugi przycisk�w dotykowych

	/* konfiguracja di�d LED do symulacj dzia�ania przycisku dotykowego*/
    gpio_set_direction( LED_UP_GPIO, GPIO_MODE_OUTPUT );
    gpio_set_direction( LED_SIDE_GPIO, GPIO_MODE_OUTPUT );

    touch_pad_init();
    touch_pad_config( TOUCH_GPIO, 0 );

    while(1) {

        key_t1();

        vTaskDelay( 2 );
    }
}
/*
 * KONIEC OBS�UGI PRZYCISKU DOTYKOWEGO
 */







/*
 * ZDARZENIA I FUNKCJE DO OBS�UGI SIECI WiFi
 */

void mk_got_ip_cb( char * ip ) {

	szachownica = 0;
	glcd_drawRect( 0, 6, 128, 29, 1 );
	setCurrentFont( &DefaultFont5x8 );
	glcd_puts( 7, 10, "STA connected, IP:", 1 );
	setCurrentFont( &font_bitocra5x13 );
	glcd_puts( 7, 20, ip, 1 );

	setCurrentFont( &MkMINI3x6 );
	glcd_puts( 106, 27, "ESP32", 1 );

	glcd_display();

	esp_netif_ip_info_t ip_info;
	esp_netif_get_ip_info(mk_netif_sta, &ip_info);
	printf( "[STA] IP: " IPSTR "\n", IP2STR(&ip_info.ip) );
	printf( "[STA] MASK: " IPSTR "\n", IP2STR(&ip_info.netmask) );
	printf( "[STA] GW: " IPSTR "\n", IP2STR(&ip_info.gw) );

	esp_netif_get_ip_info(mk_netif_ap, &ip_info);
	printf( "[AP] IP: " IPSTR "\n", IP2STR(&ip_info.ip) );
	printf( "[AP] MASK: " IPSTR "\n", IP2STR(&ip_info.netmask) );
	printf( "[AP] GW: " IPSTR "\n", IP2STR(&ip_info.gw) );
}


void mk_sta_disconnected_cb( void ) {

	szachownica = 0;
	glcd_drawRect( 0, 6, 128, 29, 1 );
	glcd_fillRect( 1, 7, 126, 27, 0 );
	setCurrentFont( &DefaultFont5x8 );
	glcd_puts( 7, 10, "STA disconnected", 1 );
	glcd_display();
}


void mk_ap_join_cb( char * mac ) {

	printf("\n************ AP JOIN \n");
}


void mk_ap_leave_cb( char * mac ) {

	printf("\n************ AP LEAVE \n");
}


void display_date_time( void ) {

	if( !is_time_changed() ) return;	// if time not changed - return without display

	static uint8_t sw;

//	gpio_set_level( LED1_GPIO, sw );

	char buf[128];
	buf[0] = 0;

	if( sw ) strcat( buf, "%F %H:%M:%S %Z %z" );
	else strcat( buf, "%F %H:%M.%S %Z %z" );

	mk_get_formatted_system_datetime( buf );

	if( !buf[0] ) sprintf( buf, "WAITING FOR SNTP ..." );


    glcd_fillRect(  5, 0, 128, 6, 0 );
    setCurrentFont( &MkMINI3x6 );
    glcd_puts( 5, 0, buf, 1 );
    glcd_display();

    time_t now;
    time(&now);

    sw ^= 1;
}
/*
 * KONIEC OBS�UGI SIECI WiFi
 */



void time_task(void *args) {

	uint16_t licznik = 0, licznik1;	//przekazywanie danych o up�ywaj�cym czasie
	//    	Wys�anie warto�ci wskazanej zmiennej do kolejki
	while(true){

		xQueueReceive( Q_Licznik1, &licznik1, 0 );
		if( !licznik1 ) licznik = 0; else licznik++;
	   	xQueueSendToBack( Q_Licznik, &licznik, 0 );
	    vTaskDelay( 10 );
	}
}



/*
 * POCZ�TEK PROGRAMU G��WNEGO
 */
void app_main(void) {

	vTaskDelay( 100 );	// tylko �eby �atwiej prze��cza� si� na terminal przy starcie
	printf("\nREADY\n");

	/*........ konfiguracja pin�w dla diod LED ..................................*/
	gpio_set_direction( LED1_GPIO, GPIO_MODE_OUTPUT );
	gpio_set_level( LED1_GPIO, 0 );

	/* ```````` Inicjalizacja I2C `````````````````````````````````````````````` */
//	i2c_init( 0, 22, 21, 800 );	// bitrate podajemy w kHz !!! nie w Hz

	/* ```````` Inicjalizacja OLED ````````````````````````````````````````````` */
//	glcd_init( 220 );

	/* ```````` Inicjalizacja NVS `````````````````````````````````````````````` */
	nvs_flash_init();

	/* ```````` Inicjalizacja WiFi ````````````````````````````````````````````` */
//	mk_wifi_init( WIFI_MODE_APSTA, mk_got_ip_cb, mk_sta_disconnected_cb, mk_ap_join_cb, mk_ap_leave_cb  );
//	mk_wifi_init( WIFI_MODE_AP, NULL, NULL, mk_ap_join_cb, mk_ap_leave_cb );
//	mk_wifi_init( WIFI_MODE_STA, mk_got_ip_cb, mk_sta_disconnected_cb, NULL, NULL );


	/* ```````` Skanowanie dost�pnych sieci  ``````````````````````````````````` */
//	mk_wifi_scan( NULL );

	//Utworzenie kolejki pi�cio elementowej do przechowywania zmiennych typu ca�kowitego
	//do przekazywania informacji o trybie pracy przycisku dotykowego
	Q_LED_Blink = xQueueCreate( 5, sizeof( int ) );

	//Utworzenie kolejki pi�cio elementowej do przechowywania zmiennych typu ca�kowitego
	//do przekazywania informacji o up�ywie czasie do przej�� pomi�dzy efektami �wietlnymi
	Q_Licznik = xQueueCreate( 5, sizeof( uint16_t ) );
	Q_Licznik1 = xQueueCreate( 5, sizeof( uint16_t ) );
//	uint16_t licznik = 0;	//przekazywanie danych o up�ywaj�cym czasie

	//Utworzenie taska do obs�ugi di�d LED na rdzeniu numer 1 procesora
	xTaskCreatePinnedToCore( magicLED, "Obs�uga di�d RGB", 4096, NULL, 1, NULL, 1);

	//Utworzenie taska do obs�ugi przycisku dotykowego na rdzeniu numer 0 procesora
	xTaskCreatePinnedToCore(touch_task, "czujnik_dotyku", 4096, NULL, 1, NULL, 0);

	//Utworzenie taska do obs�ugi przycisku dotykowego na rdzeniu numer 0 procesora
	xTaskCreatePinnedToCore(time_task, "obs�uga czasu", 4096, NULL, 1, NULL, 0);


    while (1) {

//    	display_date_time();
//    	Wys�anie warto�ci wskazanej zmiennej do kolejki
//    	licznik++;
//    	xQueueSendToBack( Q_Licznik, &licznik, 0 );
        vTaskDelay( 10 );
    }
}
