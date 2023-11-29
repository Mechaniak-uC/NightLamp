/*
 * mk_i2c.h		RTOS ESP8266
 *
 *  Created on: 12 mar 2022
 *      Author: Miros�aw Karda�
 *
 *	!!!    U W A G A    !!!
 *	nale�y wprowadzi� poprawk� do oryginalnego pliku i2c.c w folderze:
 *
 *	\msys32\home\admin\ESP8266_RTOS\components\esp8266\driver
 *
 *	Jest to ORYGINALNY plik systemowy Espressif (i2c component) i zawiera
 *	paskudny b��d. B��d polega na tym, �e nie jest generowany sygna� I2C STOP !!!
 *	gdy podczas wys�ania adresu urz�dzenia I2C Slave na magistral� nie otrzymamy
 *	ACK (potwierdzenia) to tylko w tym wypadku nie jest generowany I2C STOP.
 *	Wi�kszo�� uk�ad�w I2C radzi sobie z tym problemem, ale niekt�re niestety nie
 *	i zaczynaj� dzia�a� niepoprawnie. Skopana jest bowiem funkcja:
 *
 *	i2c_master_cmd_begin()
 *
 *	dlatego trzeba edytowa� oryginalny plik i2c.h i znale�� taki fragment kodu:
 *
 	     if (cmd->ack.en == 1) {			<--------- wystarczy wyszuka� t� lini�
              if ((retVal & 0x01) != cmd->ack.exp) {
                    p_i2c->status = I2C_STATUS_ACK_ERROR;
                    return ;
              }
         }
 *
 * i przed return ; wprowadzi� kod do wygenerowania sygna�u I2C STOP jak ni�ej
 *
 *
  	     if (cmd->ack.en == 1) {			<--------- wystarczy wyszuka� t� lini�
              if ((retVal & 0x01) != cmd->ack.exp) {
                    p_i2c->status = I2C_STATUS_ACK_ERROR;

                    //........................ NAPRAWA sygna�u I2C STOP
                    i2c_master_wait(1);
                    i2c_master_set_dc(i2c_num, 0, i2c_last_state[i2c_num]->scl);
                    i2c_master_set_dc(i2c_num, 0, 1);
                    i2c_master_wait(2);     // sda 0, scl 1
                    i2c_master_set_dc(i2c_num, 1, 1);
                    i2c_master_wait(2);     // sda 1, scl 1

                    return ;
              }
         }
 *
 *  UWAGA! nale�y o tym pami�ta� zawsze gdy na nowo wygenerowane zostanie �rodowisko
 *  RTOS dla ESP8266, albo gdy zostanie wgrane/rozpakowane nowe ale nie zawiera tej
 *  poprawki.
 *
 *  Gdy zapomni si� o tej poprawce - to niestety nowo albo na nowo skompilowane
 *  projekty mog� zacz�� dzia�a� niepoprawnie.
 */

#ifndef COMPONENTS_MK_I2C_INCLUDE_MK_I2C_H_
#define COMPONENTS_MK_I2C_INCLUDE_MK_I2C_H_


//******** Wywo�ania Mutex�w z funkcj� return ***************
#define I2C_TAKE_MUTEX do { \
		esp_err_t _err_ = i2c_take_mutex(); \
        if( _err_ != ESP_OK ) return _err_;\
    } while (0)

#define I2C_GIVE_MUTEX do { \
		esp_err_t _err_ = i2c_give_mutex(); \
        if (_err_ != ESP_OK) return _err_;\
    } while (0)


//******** Wywo�ania Mutex�w bez funkcji return ***************
#define I2C_TAKE_MUTEX_NORET do { \
        i2c_take_mutex(); \
    } while (0)

#define I2C_GIVE_MUTEX_NORET do { \
        i2c_give_mutex(); \
    } while (0)






extern esp_err_t i2c_take_mutex( void );
extern esp_err_t i2c_give_mutex( void );





extern esp_err_t  i2c_init( uint8_t port_nr, uint8_t scl_pin_nr, uint8_t sda_pin_nr, int kHz  );

extern esp_err_t i2c_check_dev( uint8_t port_nr, uint8_t slave_addr );

extern esp_err_t i2c_write_byte_to_dev( uint8_t port_nr, uint8_t slave_addr, uint8_t byte );
extern esp_err_t i2c_write_word_to_dev( uint8_t port_nr, uint8_t slave_addr, uint16_t word );

extern esp_err_t i2c_read_byte_from_dev( uint8_t port_nr, uint8_t slave_addr, uint8_t * byte );
extern esp_err_t i2c_read_word_from_dev( uint8_t port_nr, uint8_t slave_addr, uint16_t * word );


extern esp_err_t i2c_dev_read( uint8_t port_nr, uint8_t slave_addr,
		const void *out_data, size_t out_size,
		void *in_data, size_t in_size );


extern esp_err_t i2c_dev_write( uint8_t port_nr, uint8_t slave_addr,
		const void *out_reg, size_t out_reg_size,
		const void *out_data, size_t out_size);


extern esp_err_t i2c_dev_read_reg( uint8_t port_nr, uint8_t slave_addr, uint8_t reg,
        void *in_data, size_t in_size);

extern esp_err_t i2c_dev_write_reg( uint8_t port_nr, uint8_t slave_addr, uint8_t reg,
        const void *out_data, size_t out_size);


#endif /* COMPONENTS_MK_I2C_INCLUDE_MK_I2C_H_ */
