/*
 * mk_touch.h
 *
 *  Created on: 10 maj 2022
 *      Author: Miros³aw Kardaœ
 */

#ifndef COMPONENTS_MK_TOUCH_MK_TOUCH_H_
#define COMPONENTS_MK_TOUCH_MK_TOUCH_H_


#define TOUCH_ADC_CHECK_MODE		0
//#define TOUCH_2_ADC_CHECK_MODE		0
//#define TOUCH_3_ADC_CHECK_MODE		0

//Zdefioniowanie do jakiego portu pod³¹czony jest czujnik dotyku
#define TOUCH_GPIO	4

//Zdefiniowanie wyjœcia dla diody led, który s³u¿y na razie do testów
#define LED_UP_GPIO		19
#define LED_SIDE_GPIO	21
//#define TLED_2_GPIO		26
//#define TLED_3_GPIO		25


//Zdefiniowanie wartoœæ anlogowych zadzia³ania przycisku dotykowego
#define ADC_MIN			425 // 280	// 399
#define ADC_MAX			435  //380 // 401

//////Utworzenie uchwytu do kolejki obs³uguj¹cej jakie diody w lampce maj¹ byæ aktywne
//extern QueueHandle_t Q_LED_Blink;


extern void touch_task(void *args);

#endif /* COMPONENTS_MK_TOUCH_MK_TOUCH_H_ */
