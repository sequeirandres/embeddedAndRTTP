#include <string.h>
#include <pico/stdlib.h>
#include "LoRa-RP2040.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

extern "C" {
    #include "ssd1306_i2c.h"
}

// Define pin for gpio leds
#define PIN_LED	25

// Define status on/off
#define LED_STATUS_ON   1
#define LED_STATUS_OFF  0

// Define Stack Names
#define TASK_WRITE_DISPLAY_NAME		"Task-Write-display"
#define TASK_RECIVE_LORA_NAME		"Task-Receved-LoRa"
#define TASK_SEND_USB_NAME			"Task-Send-usb"

// Define Stack Size for tasks
#define UX_STACK_SIZE_TASK_DISPLAY	800
#define UX_STACK_SIZE_TASK_LORA		256
#define UX_STACK_SIZE_TASK_USB		256

// Define priority for tasks
#define IDLE_PRIORITY_TASK_DISPLAY	1
#define IDLE_PRIORITY_TASK_LORA		1
#define IDLE_PRIORITY_TASK_USB		1

// Dedine ID_ADDR for Gateway
#define GATEWAY_ID_ADDR			2001
#define GATEWAY_PAYLOAD_SIZE	64

// Delay for Task
#define HAL_DELAY_WRITE_DISPLAY	6000
#define HAL_DELAY_READ_LORA		10
#define HAL_DELAY_SEND_USB 		3000
#define HAL_DELAY_LED_BLINK		200


// payload = [id(4 bytes)][temp(4 bytes)][press (6 bytes)][hum(4 bytes)][gps-lat][gps-long][gps-nsat][counter(4bytes)]

// payload = payload = [id(4 bytes)][temp(4 bytes)][press (6 bytes)][hum(4 bytes)][alarm(1 bytes)][counter(4bytes)]

// 

/*
typedef struct
{
	uint32_t co2;
	uint32_t no2;
}mq135_t;
*/
// Sensor: Temperatura, Humedad y presion 
typedef struct
{
	uint32_t temperature;
	uint32_t pressure;
	uint32_t humidity;
}bmp280_t;

// Sensor: gps
/*
typedef struct
{
	uint32_t longitude;
	uint32_t latitude;
	uint32_t altitud;
	uint8_t fixed;  // cantidad de satelites  
}gps_t ;
*/
// struct gateway
typedef struct
{
	char payload[GATEWAY_PAYLOAD_SIZE];
	uint32_t id;
	uint32_t idLocal;
	uint32_t counter;
	uint32_t npackage;
	bool alarm; 
	bmp280_t bmp280;
	//mq135_t mq135;
	//gps_t gps;
}gateway_t ;


void payload_get_id(gateway_t *gateway, char *display)
{
	gateway->id = (gateway->payload[0]-'0')*1000 + (gateway->payload[1]-'0')*100 + (gateway->payload[2]-'0')*10 + (gateway->payload[3]-'0')*1 ;	
	display[0] = gateway->payload[0];
	display[1] = gateway->payload[1];
	display[2] = gateway->payload[2];
	display[3] = gateway->payload[3];
	display[4] = '\0'; //  error del ssd1306
}

void payload_get_temp(gateway_t *gateway, char *display) // 4,5,6,7,
{
	gateway->bmp280.temperature = (gateway->payload[4]-'0')*1000 + (gateway->payload[5]-'0')*100 + (gateway->payload[6]-'0')*10 + (gateway->payload[7]-'0')*1 ;
	display[0] = gateway->payload[4];
	display[1] = gateway->payload[5];
	display[2] = '.';
	display[3] = gateway->payload[6];
	display[4] = gateway->payload[7];
	display[5] = '\0'; //  error del ssd1306

}

void payload_get_press(gateway_t *gateway,char *display) // 8,9, 10,11,12,13
{
	gateway->bmp280.pressure = (gateway->payload[8]-'0')*10000 + (gateway->payload[9]-'0')*1000 + (gateway->payload[10]-'0')*100 + (gateway->payload[11]-'0')*10 + (gateway->payload[12]-'0')*1 ;
	display[0] = gateway->payload[8];
	display[1] = gateway->payload[9];
	display[2] = gateway->payload[10];
	display[3] = gateway->payload[11];
	display[4] = '.';
	display[5] = gateway->payload[12];
	display[6] = '\0'; //  error del ssd1306

}

void payload_get_hum(gateway_t *gateway,char *display)
{
	gateway->bmp280.humidity = 0 ;
	display[0] = '0';
	display[1] = '0';
	display[2] = '\0';
}

void payload_get_alarm_status(gateway_t *gateway)
{
	//printf("(alarm = %c )",gateway->payload[15]) ;
	if(gateway->payload[15]!='0'){gateway->alarm=true;}
	else{gateway->alarm=false;}
}

uint16_t payload_get_npackage(gateway_t *gateway,char *display ,uint8_t position)
{
	uint16_t index=0;
	gateway->npackage=0 ; 
	while (gateway->payload[position]!='\0')
	{
		display[index] =  gateway->payload[position]; 
		gateway->npackage = gateway->npackage*10 + (gateway->payload[position]-'0')*1; 
		position++;index++;
	}
	display[index] = '\0';

	return index;
}

// shared vtask1 and vtask2 
QueueHandle_t xQueue;  

// --- task 1 --- 
void vTask_write_display(void *pvParameters)
{    
   
    gateway_t *gateway =  (gateway_t *)pvParameters ;  // retorna un puntero a gateway_t
    
 	BaseType_t xStatus;
   
   	// init Display I2C SSD1306 I2C Display OLED 0.9"
   	
	uint32_t index = 0;
	char displayString[8] ;
  
	SSD1306_gpio_set(SSD1306_PICO_I2C_SDA_PIN,SSD1306_PICO_I2C_SCL_PIN) ;
    
	SSD1306_init();
    // zero the entire display
	SSD1306_clear() ;
	
	vTaskDelay(10);
	SSD1306_Write(1,0, (char *)"NODE",4) ;  // SSD1306_Write(41,0, (char *)"0000",4) ;
	// SSD1306_Write(100,0, (char *)"ON ",3) ;

	vTaskDelay(10);
	SSD1306_Write(1,2, (char *)"TEMP",4) ; // SSD1306_Write(41,2, (char *)"21.18",5) ;
	
	vTaskDelay(10);
	SSD1306_Write(1,4, (char *)"PRES",4) ; // SSD1306_Write(41,4, (char *)"1028.5",6) ;
	
	vTaskDelay(10);
	SSD1306_Write(1,6, (char *)"HUM ",4) ; // SSD1306_Write(41,6, (char *)"00",2) ;
	
   for(;;)
   {
   		// Recibir los datos de la cola
        xStatus = xQueueReceive( xQueue , gateway , portMAX_DELAY);
        
        if(xStatus == pdPASS)
        {
			// get id from payload
			payload_get_id(gateway,displayString);
			// write to display
   	  		SSD1306_Write(41,0, (char *)displayString,4) ;
			
			// get tempeture 
    		payload_get_temp(gateway,displayString);
			// write to display
        	SSD1306_Write(41,2, (char *)displayString,5);
			
			// get pressure
     		payload_get_press(gateway,displayString);
			// write to display
			SSD1306_Write(41,4, (char *)displayString,6);

       		// get Humedity
       		payload_get_hum(gateway,displayString);
			// write to display
			SSD1306_Write(41,6, (char *)displayString,2);
			
			// get alarm
       		payload_get_alarm_status(gateway);
			// write alarm on / off
			if(gateway->alarm)
       		{
       			SSD1306_Write(100,0, (char *)"ON ",3) ;
       		}
       		else
       		{
       			SSD1306_Write(100,0, (char *)"OFF",3) ;
       		}

			// get number of package
			index = payload_get_npackage(gateway, displayString, 16);
			
			SSD1306_Write(80,7, (char *)displayString,index);

        }
        else
        {
        
        }
   		vTaskDelay(HAL_DELAY_WRITE_DISPLAY);
   }
}


void vTask_read_lora(void *pvParameters)
{
	gateway_t *gateway =  (gateway_t *)pvParameters ;  // retorna un puntero a gateway_t
    
 	BaseType_t xStatus;
 	uint8_t index=0;
 	uint8_t packetSize=0;
	gateway->counter = 0 ;
	
	// Led Indicator for tx sussesfully 
	gpio_init(PIN_LED);
    gpio_set_dir(PIN_LED,GPIO_OUT);
	
	// Init LoRa for 433 MHz
	if (!LoRa.begin(433E6)) {
		printf("Starting LoRa failed!\n");
		while (1);
	}

   for(;;)
   {
	    // Algoritmos para leer buffer from 
        packetSize = LoRa.parsePacket();
     	//printf("Packet size : %d \n", packetSize) ;
      	if(packetSize)
      	{
      	    // Enviar los datos que le llegan por xQueue   	
        	xStatus = xQueueSend( xQueue , gateway , portMAX_DELAY);
      	    if(xStatus == pdPASS)
        	{
      	   		// read packet
      	   	 	index = 0 ;
        	 	while (LoRa.available())
        	 	{
            		gateway->payload[index]=(char) LoRa.read();
            		index++;
         	 	}
			 	gateway->payload[index] = '\0';
			 	gateway->counter +=1 ;
			 
			 	// for debug
			 	printf("Node LoRa Received package [%s]: %s \n", &gateway->payload[16],gateway->payload ) ;
			 
			  	// Blink Led for send data :
			 	gpio_put(PIN_LED, LED_STATUS_ON);
			 	vTaskDelay(HAL_DELAY_LED_BLINK);
	 		 	gpio_put(PIN_LED, LED_STATUS_OFF);
        		
        	}
			else{}
        }         
		else{}
		vTaskDelay(HAL_DELAY_READ_LORA); 		
   	}	
}


void vTask_send_usb(void *pvParameters)
{
	
	gateway_t *gateway =  (gateway_t *)pvParameters ;  // retorna un puntero a gateway_t
 	BaseType_t xStatus;

	for(;;)
	{
   		// Recibir los datos de la cola
        xStatus = xQueueReceive( xQueue , gateway , portMAX_DELAY);
        
        if(xStatus == pdPASS)
        {	
			printf("Node id=%d, Temp=%.2f ºC, Press=%.2f HPa, Hum=%d ",gateway->id, (float)gateway->bmp280.temperature/100, (float)gateway->bmp280.pressure/10,gateway->bmp280.humidity) ;

			if(gateway->alarm)
       		{
       			printf(" ,Alarm=on") ;
       		}
       		else
       		{
       			printf(" ,Alarm=off") ;
       		}

			printf(" ,Package=%d\n",gateway->npackage);
		}
   	}
}

int main(void)
{
	// Init SDK Pico
	stdio_init_all();
	
	// New gateway gateway_ID_ADDR
	gateway_t gateway;
	gateway.idLocal = GATEWAY_ID_ADDR ;

	gateway.npackage = 0 ;
	gateway.alarm = false ;
	
	for (gateway.counter = 0; gateway.counter < GATEWAY_PAYLOAD_SIZE-1; gateway.counter++)
	{
		gateway.payload[gateway.counter] = '0';
	}
	gateway.payload[gateway.counter] = '\0';

	gateway.counter = 0 ;

	// create xQueue 
    xQueue = xQueueCreate(sizeof(gateway_t), 1);
	
	if(xQueue)
	{	
		xTaskCreate(vTask_read_lora,TASK_RECIVE_LORA_NAME, UX_STACK_SIZE_TASK_LORA, &gateway, IDLE_PRIORITY_TASK_DISPLAY, NULL);
		xTaskCreate(vTask_write_display,TASK_WRITE_DISPLAY_NAME, UX_STACK_SIZE_TASK_DISPLAY, &gateway, IDLE_PRIORITY_TASK_LORA, NULL);
		xTaskCreate(vTask_send_usb,"Task send usb", UX_STACK_SIZE_TASK_USB, &gateway, IDLE_PRIORITY_TASK_USB, NULL);
	   	vTaskStartScheduler();
	}
	else
	{
		for(;;)
		{
			printf("Error al crear xQueue! \n");
		}
    }
  return 0;

}

