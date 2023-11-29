/*
 * mk_touch.c
 *
 *  Created on: 10 maj 2022
 *      Author: Miros�aw Karda�
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




//Ca�o�� na razie wyremowa�em
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
	 * 		"0"-�wiec� si� wszystkie diody lampki
	 * 		"1"-�wieci si� tylko pod�wietlenie grafiki
	 * 		"2"-�wiec� si� tylko boki lampki
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

	    		load ^= 1;	//Ustaw "1" w zmiennej pomocnicznej
	    		gpio_set_level( LED_UP_GPIO, load );	//Za�acz pod�wietlenie grafiki
	    		gpio_set_level( LED_SIDE_GPIO, load ); //Za��cz pod�wietlenie bok�w lampki

	    		tryb = 1;
	    		//Wys�anie warto�ci wskazanej zmiennej do kolejki
//	    		xQueueSendToBack( Q_LED_Blink, &tryb, 0 );

	    		//Je�eli zliczanie czasu nie zosta�o uruchomione to je za��cz
	    		time_sw = 1;	//za��cz zliczanie czasu
	    		loop_time = 100;	//Ustaw jak d�ugo trzeba czeka� na ponowne naci�ni�cie klawisza
	    	}
    		break;

    	case 1:
    		if( state ) {//sprawd� czy klawisz jest aktualnie naci�ni�ty

    			gpio_set_level( LED_UP_GPIO, 1 );	//Za�acz pod�wietlenie grafiki
    			tryb = 1;
    			//Wys�anie warto�ci wskazanej zmiennej do kolejki
//    			xQueueSendToBack( Q_LED_Blink, &tryb, 0 );

//    			gpio_set_level( LED_SIDE_GPIO, 0 ); //Wy��cz pod�wietlenie bok�w lampki
    			time_sw = 2;	//Ustaw tryb odliczania na 2
    			loop_time = 100;	//Ustaw jak d�ugo trzeba czeka� na ponowne naci�ni�cie klawisza
    		}
    		break;

    	case 2:
    		if( state ) {

//    			gpio_set_level( LED_UP_GPIO, 0 );	//Wy��cz pod�wietlenie grafiki
    			gpio_set_level( LED_SIDE_GPIO, 1 ); //Za��cz pod�wietlenie bok�w lampki
    			tryb = 2;
    			//Wys�anie warto�ci wskazanej zmiennej do kolejki
//    			xQueueSendToBack( Q_LED_Blink, &tryb, 0 );

    			time_sw = 0;	//zerowanie odliczania czasu -wy��cz odmierzanie czasu
    		}
    		break;
    	}

    	sw = 0;	//Naci�ni�cie przycisku zosta�o wykonane - zeruj zmienn� pomocnicz�
    }

    if( loop_time ) --loop_time; else time_sw = 0;

    //jednokrotna zmianna stanu diody LED w zale�no�ci do zmiennej load
//    if( led_up ) {
//    	//Wy�wietlanie danych na OLED
////    	display_key_onoff(0,load);
//    	//Ustaw warto�� pinu procesora
////    	gpio_set_level( LED_UP_GPIO, load );
//    	printf("touch 1 state:\t%d\n", load);
//    	led_up = 0;
//    }
#endif
}




//void touch_task(void *args) {
//
//	/* konfiguracja di�d LED do symulacj dzia�ania przycisku dotykowego*/
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




























