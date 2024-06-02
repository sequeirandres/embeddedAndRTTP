#include <string.h>
#include <pico/stdlib.h>
#include "LoRa-RP2040.h"
#include "ssd1306_i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"


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
#define UX_STACK_SIZE_TASK_DISPLAY	256
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


// payload = [id][temp][press][hum][gps-lat][gps-long][gps-nsat][counter]

// sensor: Calidad de aire
typedef struct
{
	uint32_t co2;
	uint32_t no2;
}mq135_t;

// Sensor: Temperatura, Humedad y presion 
typedef struct
{
	uint32_t temperature;
	uint32_t pressure;
	uint32_t humidity;
}bmp280_t;

// Sensor: gps
typedef struct
{
	uint32_t longitude;
	uint32_t latitude;
	uint32_t altitud;
	uint8_t fixed;  // cantidad de satelites  
}gps_t ;

// struct gateway
typedef struct
{
	char payload[GATEWAY_PAYLOAD_SIZE];
	uint32_t id;
	uint32_t idLocal;
	uint32_t counter;
	bmp280_t bmp280;
	//mq135_t mq135;
	//gps_t gps;
	
}gateway_t ;


// shared vtask1 and vtask2 
QueueHandle_t xQueue;  

// --- task 1 --- 

void vTask_write_display(void *pvParameters)
{    
   
    gateway_t *gateway =  (gateway_t *)pvParameters ;  // retorna un puntero a gateway_t
    
 	BaseType_t xStatus;
   
   	// init Display I2C SSD1306 I2C Display OLED 0.9"
  
  /*
	SSD1306_gpio_set(SSD1306_PICO_I2C_SDA_PIN,SSD1306_PICO_I2C_SCL_PIN) ;
    SSD1306_init();

    // zero the entire display
	SSD1306_clear() ;

	sleep_ms(100) ;
	
	//fil_of_string = 0,..., 7;
	
	//SSD1306_Write((int)8*4,0, "FREERTOS",8) ;
	
	sleep_ms(100) ;	
	
	SSD1306_Write(40,0, "BMP280",6) ;
	
	sleep_ms(100) ;
	
	SSD1306_Write(20,2, "TEMP Y PRES",11) ; */
   
   for(;;)
   {
   	
   		// Recibir los datos de la cola
        xStatus = xQueueReceive( xQueue , gateway , portMAX_DELAY);
        
        if(xStatus == pdPASS)
        {

   	  		// get tempeture 
    		gateway->bmp280.temperature = 0 ;
        	// get pressure
     		gateway->bmp280.pressure = 0 ;
       		// get Humedity
       		gateway->bmp280.humidity = 0 ;
       		
       		// for debug
       		printf("Temp = %d , Press = %d  , Hum = 0\n", gateway->bmp280.temperature, gateway->bmp280.pressure) ;
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

		// Enviar los datos que le llegan por xQueue   	
        xStatus = xQueueSend( xQueue , gateway , portMAX_DELAY);
        
        if(xStatus == pdPASS)
        {
         	// Algoritmos para leer buffer from 
         	packetSize = LoRa.parsePacket();
      		//printf("Packet size : %d \n", packetSize) ;
      		if(packetSize)
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
			 	printf("Received package : %s \n", gateway->payload ) ;
			 

			  // Blink Led for send data
			 	gpio_put(PIN_LED, LED_STATUS_ON);
			 	vTaskDelay(HAL_DELAY_LED_BLINK);
	 		 	gpio_put(PIN_LED, LED_STATUS_OFF);
			
			}
		}
		else
		{
		}
		//vTaskDelay(HAL_DELAY_READ_LORA); // 
		
   	}	
}


void vTask_send_usb(void *pvParameters)
{

	


}


// vTask_send_lora()
// vTask_send_usb()
// vTask_read_sensor()	
// vTask_led_blink() 

int main(void) {

	// Init SDK Pico
	stdio_init_all();
	
	// New gateway gateway_ID_ADDR
	gateway_t gateway;
	gateway.idLocal = GATEWAY_ID_ADDR ;
	gateway.counter = 0 ;
	
	// create xQueue 
    xQueue = xQueueCreate(sizeof(gateway_t), 1);
	
	if(xQueue)
	{
		
		xTaskCreate(vTask_read_lora,TASK_RECIVE_LORA_NAME, UX_STACK_SIZE_TASK_LORA, &gateway, IDLE_PRIORITY_TASK_DISPLAY, NULL);
		
		xTaskCreate(vTask_write_display,TASK_WRITE_DISPLAY_NAME, UX_STACK_SIZE_TASK_DISPLAY, &gateway, IDLE_PRIORITY_TASK_LORA, NULL);
		
		//xTaskCreate(vTask_send_usb,"Task send usb", UX_STACK_SIZE_TASK_USB, &gateway, IDLE_PRIORITY_TASK_USB, NULL);
		
		//XTaskAlive()
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

