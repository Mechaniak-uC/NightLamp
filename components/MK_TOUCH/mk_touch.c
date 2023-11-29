/*
 * mk_touch.c
 *
 *  Created on: 10 maj 2022
 *      Author: Miros³aw Kardaœ
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

#include "driver/touch_sensor.h"

//-----------------------------------------


#include "mk_touch.h"


#include "mk_glcd_base.h"




//Ca³oœæ na razie wyremowa³em
void display_key_onoff( int key_nr, int onoff ) {

//	int x = 32;
//
//	x = x + (32*key_nr);
//
//	setCurrentFont( &font_bitocra5x13 );
//
//	char bf[128];
//	key_nr++;
//	sprintf( bf, "T%d", key_nr );
//
//	glcd_circle( x , 51, 12, 1 );
//	glcd_fillCircle(  x, 51, 9, onoff );
//	glcd_puts( x-6 , 45, bf, !onoff );
//	glcd_display();

}





void key_t1( void ) {

#if TOUCH_1_ADC_CHECK_MODE == 0
	static uint8_t sw, state, time_sw, load, loop_time, tryb;
	/* OPIS ZMIENNYCH
	 * sw		- s³u¿y do jednokrotnej reakcji na zmianê stanu padu dotykowego
	 * state 	- aktualny stan urz¹dzenia z uwzglêdnieniem touch pada i do jednokrotnego wykonywania warunków
	 * 		"0" - stan pada poni¿ej ustawionej wartoœci minimalnej
	 *		"1"	- stan poda powy¿ej ustawionej wartoœci maksymalnej
	 * load		- aktualny rzeczywisty stan diody wyjœciowej ("0"-OFF, "1"=ON)
	 * time_sw	- zmienna do odczytywania jaki aktualnie stan ma odliczanie czasu po naciœniêciu pada
	 * 		"0"-czas nie jest odliczany
	 * 		"1"-czas zosta³ za³¹czony
	 * 		"2"-czas zosta³ ponownie za³¹czony
	 * loop_time- odliczanie czasu na wykonanie prze³¹czenia diód LED
	 * tryb		- w jakiej konfiguracji pracuje lampka
	 * 		"0"-œwiec¹ siê wszystkie diody lampki
	 * 		"1"-œwieci siê tylko podœwietlenie grafiki
	 * 		"2"-œwiec¹ siê tylko boki lampki
	 */
#endif

	uint16_t data;	//Wartoœæ cyfrowa sygna³u analogowego odczytana z czujnika dotyku

	touch_pad_read(TOUCH_GPIO, &data);

	touch_pad_filter_start( 50 );
	touch_pad_read_filtered(TOUCH_GPIO, &data);

#if TOUCH_ADC_CHECK_MODE == 1
	printf( "touch1 data: %d\n", data );

#else

	//czy wartoœæ odczytana z pada mniejsza od ustalonej wartoœci oraz jego stan wynosi 0
	//zmienna "state" umo¿liwia jednokrtone wykonanie rozkazów znajduj¹cych siê w warunku
    if( data < ADC_MIN && state == 0 ) {
    	sw = 1;
    	state = 1;	//Ustawienie tej wartoœci zapobiega ci¹g³ym wykonywaniu warunku
    }

    //wartoœæ analogowa z pada wzros³a i uk³ad jest uruchmoiony to ustaw zmienn¹ sw do wyzerowania
    //zmiennej state, ¿e prad by³ naciœniêty
    if( data > ADC_MAX && state == 1 ) {
    	sw = 1;		//Ta linia chyba niepotrzebna
    	state =  0;
    }

    //warunek realizowany tylko jednokrotnie jak przycisk jest za³¹czany
    if( sw ) {
    	//Czy odmierzanie czasu jest za³¹czone i nie mien¹³a ustawiona wartoœæ czasu
    	switch( time_sw ){
    	case 0:
    		if( state ) {	//sprawdŸ czy klawisz jest aktualnie naciœniêty

	    		load ^= 1;	//Ustaw "1" w zmiennej pomocnicznej
	    		gpio_set_level( LED_UP_GPIO, load );	//Za³acz podœwietlenie grafiki
	    		gpio_set_level( LED_SIDE_GPIO, load ); //Za³¹cz podœwietlenie boków lampki

	    		tryb = 1;
	    		//Wys³anie wartoœci wskazanej zmiennej do kolejki
//	    		xQueueSendToBack( Q_LED_Blink, &tryb, 0 );

	    		//Je¿eli zliczanie czasu nie zosta³o uruchomione to je za³¹cz
	    		time_sw = 1;	//za³¹cz zliczanie czasu
	    		loop_time = 100;	//Ustaw jak d³ugo trzeba czekaæ na ponowne naciœniêcie klawisza
	    	}
    		break;

    	case 1:
    		if( state ) {//sprawdŸ czy klawisz jest aktualnie naciœniêty

    			gpio_set_level( LED_UP_GPIO, 1 );	//Za³acz podœwietlenie grafiki
    			tryb = 1;
    			//Wys³anie wartoœci wskazanej zmiennej do kolejki
//    			xQueueSendToBack( Q_LED_Blink, &tryb, 0 );

//    			gpio_set_level( LED_SIDE_GPIO, 0 ); //Wy³¹cz podœwietlenie boków lampki
    			time_sw = 2;	//Ustaw tryb odliczania na 2
    			loop_time = 100;	//Ustaw jak d³ugo trzeba czekaæ na ponowne naciœniêcie klawisza
    		}
    		break;

    	case 2:
    		if( state ) {

//    			gpio_set_level( LED_UP_GPIO, 0 );	//Wy³¹cz podœwietlenie grafiki
    			gpio_set_level( LED_SIDE_GPIO, 1 ); //Za³¹cz podœwietlenie boków lampki
    			tryb = 2;
    			//Wys³anie wartoœci wskazanej zmiennej do kolejki
//    			xQueueSendToBack( Q_LED_Blink, &tryb, 0 );

    			time_sw = 0;	//zerowanie odliczania czasu -wy³¹cz odmierzanie czasu
    		}
    		break;
    	}

    	sw = 0;	//Naciœniêcie przycisku zosta³o wykonane - zeruj zmienn¹ pomocnicz¹
    }

    if( loop_time ) --loop_time; else time_sw = 0;

    //jednokrotna zmianna stanu diody LED w zale¿noœci do zmiennej load
//    if( led_up ) {
//    	//Wyœwietlanie danych na OLED
////    	display_key_onoff(0,load);
//    	//Ustaw wartoœæ pinu procesora
////    	gpio_set_level( LED_UP_GPIO, load );
//    	printf("touch 1 state:\t%d\n", load);
//    	led_up = 0;
//    }
#endif
}




//void touch_task(void *args) {
//
//	/* konfiguracja diód LED do symulacj dzia³ania przycisku dotykowego*/
//    gpio_set_direction( LED_UP_GPIO, GPIO_MODE_OUTPUT );
//    gpio_set_direction( LED_SIDE_GPIO, GPIO_MODE_OUTPUT );
//
//    touch_pad_init();
//    touch_pad_config( TOUCH_GPIO, 0 );
//
//    while(1) {
//
//        key_t1();
//
//        vTaskDelay( 2 );
//
//    }
//}




























