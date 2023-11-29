/*
 * mk_glcd_config.h
 *
 *  Created on: 25 mar 2022
 *      Author: Miros�aw Karda�
 */

#ifndef COMPONENTS_MK_GLCD_MK_GLCD_CONFIG_H_
#define COMPONENTS_MK_GLCD_MK_GLCD_CONFIG_H_


//^^^^^^^^^^^^^^^^^^^^^^^ KONFIGURACJA BIBLIOTEKI ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

//..... wyb�r chipsetu - rodzaju wy�wietlacza - odkomentuj w�a�ciw� lini� ............................
#define USE_SSD1306						// OLED
//#define USE_SH1106						// OLED --> nowy by rogerpolaris
//#define USE_SSD1309					// OLED
//#define USE_COG_ST7565R				// COG
//#define USE_NOKIA_PCD8544				// NOKIA
//#define USE_UC1611S						// DOGM240N-6
//#define USE_UC1701					// LCX
//#define USE_ST7920					// DIGOLE


//..... wyb�r rozdzielczo�ci - wy�wietlacza - odkomentuj w�a�ciw� lini� ...............................
//#define GLCD_RES_240_64
#define GLCD_RES_128_64						// OLED / COG / LCX / DIGOLE
//#define GLCD_RES_128_32					// OLED
//#define GLCD_RES_84_48					// NOKIA


//..... r�czne ustawienie rozdzielczo�ci - je�li wiesz co robisz ......................................
//..............w przeciwnym wypadku u�yj ustawie� predefiniowanych powy�ej GLCD_RES_xx_xx ............
//..... aby wymusi� r�czne ustawienie rozdzielczo�ci - zakomentuj wszystkie predefiniwane .............
#define GLCD_WIDTH                  128
#define GLCD_HEIGHT                 64



#define SHOW_DEMO_SCREEN			0





#ifdef GLCD_RES_240_64
	#undef GLCD_WIDTH
	#undef GLCD_HEIGHT
	#define GLCD_WIDTH                  240
	#define GLCD_HEIGHT                 64
#endif

#ifdef GLCD_RES_128_64
	#undef GLCD_WIDTH
	#undef GLCD_HEIGHT
	#define GLCD_WIDTH                  128
	#define GLCD_HEIGHT                 64
#endif

#ifdef GLCD_RES_128_32
	#undef GLCD_WIDTH
	#undef GLCD_HEIGHT
	#define GLCD_WIDTH                  128
	#define GLCD_HEIGHT                 32
#endif

#ifdef GLCD_RES_84_48
	#undef GLCD_WIDTH
	#undef GLCD_HEIGHT
	#define GLCD_WIDTH                  84
	#define GLCD_HEIGHT                 48
#endif




#define GLCD_BUF_SIZE 	( GLCD_WIDTH * GLCD_HEIGHT / 8 )



#endif /* COMPONENTS_MK_GLCD_MK_GLCD_CONFIG_H_ */
