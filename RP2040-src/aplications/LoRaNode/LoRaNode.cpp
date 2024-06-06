#include <string.h>
#include <pico/stdlib.h>
#include "LoRa-RP2040.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "bmp280_i2c.h"

// Define pin for gpio leds
#define PIN_LED		25
#define PIN_ALARM	14	

// Define status on/off
#define LED_STATUS_ON   1
#define LED_STATUS_OFF  0

// Define Task Names
#define TASK_READ_BMP280_NAME	"Task-Read-bmp280"
#define TASK_SEND_LORA_NAME		"Task-Send-LoRa"
#define TASK_SEND_USB_NAME		"Task-Send-usb"
#define TASK_ALARM_NAME			"Task-Alarm"

// Define Stack Size for tasks
#define UX_STACK_SIZE_TASK_READ		256
#define UX_STACK_SIZE_TASK_LORA		256
#define UX_STACK_SIZE_TASK_USB		128
#define UX_STACK_SIZE_TASK_ALARM	128

// Define priority for tasks
#define IDLE_PRIORITY_TASK_READ		1
#define IDLE_PRIORITY_TASK_LORA		1
#define IDLE_PRIORITY_TASK_USB		1
#define IDLE_PRIORITY_TASK_ALARM	1

// Dedine ID_ADDR for node
#define NODE_ID_ADDR	1001
#define ALARM_TRIGGER_TEMP	2300		// 21.00 ªC
#define ALARM_TRIGGER_HUM	10			// 10 % Humidity
#define ALARM_TRIGGER_PRES	1050000		// 1050 HPa

// Delay for Task
#define HAL_DELAY_READ_SENSOR	4000
#define HAL_DELAY_SEND_LORA		4000
#define HAL_DELAY_SEND_USB 		2000
#define HAL_DELAY_LED_BLINK		 200
#define HAL_DELAY_ALARM			10000

// payload (   gps) = [id][temp][press][hum][gps-lat][gps-long][gps-nsat][Alarm-on/off][counter]
// payload (no gps) = [id][temp][press][hum][Alarm-on/off][counter]

typedef struct
{
	uint32_t temperature;
	uint32_t pressure;
	uint32_t humidity;
}bmp280_t;

typedef struct
{
	//char payload[NODE_PAYLOAD_SIZE];
	uint32_t id;
	uint32_t counter;
	bool alarm;
	bmp280_t bmp280;
	
	//sensor_t gps;
}node_t ;

// shared vtask1 and vtask2 
QueueHandle_t xQueue;  

// Task 1 -> Read sensor Bmp280 
void vTask_read_bmp280(void *pvParameters)
{    
   
    node_t *node =  (node_t *)pvParameters ;  // retorna un puntero a node_t
    
 	BaseType_t xStatus;
   
   	//BMP280 
    int32_t raw_temperature=0;
    int32_t raw_pressure=0;

	int32_t temperature=0;
	int32_t pressure=0; 

    // Init port i2c   i2c0 - GPIO_FUNC I2C 
    bi_decl(bi_2pins_with_func(PICO_DEFAULT_I2C_SDA_PIN, PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C));
    bi_decl(bi_program_description("BMP280-I2C"));

    // I2C is "open drain", pull ups to keep signal high when no data is being sent
    i2c_init(i2c_default, 100 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
	
    // New variable sensor 
    BMP280 sensorBmp280;
    
    // Init sensor bmp280
    sensorBmp280.init();

	// Set Up calibration bmp280 fixed compensation to level "0" 
	sensorBmp280.getCalibParams();

   
   for(;;)
   {
   	
   		// Recibir los datos de la cola
        xStatus = xQueueSend( xQueue , node , portMAX_DELAY);
        
        if(xStatus == pdPASS)
        {
        	// get tempetura and pressure :
   	  		sensorBmp280.readRaw(&raw_temperature, &raw_pressure);
   	  		// get tempeture 
        	node->bmp280.temperature = sensorBmp280.convertTemp(raw_temperature);
        	// get pressure
      		node->bmp280.pressure = sensorBmp280.convertPressure(raw_pressure, raw_temperature);
       		// get Humedity
       		node->bmp280.humidity = 0 ;
    
        }
        else
        {
        
        }
   		vTaskDelay(HAL_DELAY_READ_SENSOR);

   }
}
// Task 2 -> Send data to lora 
void vTask_send_lora(void *pvParameters)
{
	node_t *node =  (node_t *)pvParameters ;  // retorna un puntero a node_t
    
 	BaseType_t xStatus;

	node->counter = 0 ;
	
	// Led Indicator for tx sussesfully 
	gpio_init(PIN_LED);
    gpio_set_dir(PIN_LED,GPIO_OUT);
	
	if (!LoRa.begin(433E6)) {
		printf("Starting LoRa failed!\n");
		while (1);
	}

	
   for(;;)
   {
		// receive from queue
        xStatus = xQueueReceive( xQueue , node , portMAX_DELAY);
        
        if(xStatus == pdPASS)
        {
			LoRa.beginPacket();
			LoRa.print(node->id);					// 4 bytes
			LoRa.print(node->bmp280.temperature) ;  // 4 bytes
			LoRa.print(node->bmp280.pressure) ;		// 6 bytes
			LoRa.print(node->bmp280.humidity) ;		// 1 byte
			if (node->alarm){LoRa.print(1);}		// 1 byte
			else{LoRa.print(0);}
			LoRa.print(node->counter) ;				// 4 bytes
			LoRa.endPacket(true);
					
			// Blink Led for send data
			gpio_put(PIN_LED, LED_STATUS_ON);
			vTaskDelay(HAL_DELAY_LED_BLINK);
	 		gpio_put(PIN_LED, LED_STATUS_OFF);
	
			// add couter
			node->counter+=1; 
		}
		else
		{
		
		}
		vTaskDelay(HAL_DELAY_SEND_LORA); // 
   }
}
// Task 3 -> Send data to ubs - for debug no realice 
void vTask_send_usb(void *pvParameters)
{
	node_t *node =  (node_t *)pvParameters ;  // retorna un puntero a node_t
 	BaseType_t xStatus;
	
	while(true)
	{
		
		xStatus = xQueueReceive( xQueue , node , portMAX_DELAY);
        
        if(xStatus == pdPASS)
        {
			//for debug
			printf("LoRa Node Send Package [%d] :",node->counter);
			printf("%d",node->id);
			printf("%d",node->bmp280.temperature);
			printf("%d",node->bmp280.pressure);
			printf("%d",node->bmp280.humidity);
			if (node->alarm)
			{
				printf("%d",1) ;
			}
			else printf("%d",0) ;
			printf("%d",node->counter);			
			printf("\n") ;
		}
		else
		{

		}
		vTaskDelay(HAL_DELAY_SEND_USB);
	}

}
// Task 4 -> Read sensor DHT11
/* 
void vTask_read_dh11(void *pVParameters)
{

 	dht_t dht;
    dht_init(&dht, DHT_MODEL, pio0, DATA_PIN, true );
    
    for(;;)
    {
		
		float humidity;
        float temperature_c;
		
        dht_start_measurement(&dht);
        
        dht_result_t result = dht_finish_measurement_blocking(&dht, &humidity, &temperature_c);
        
        if (result == DHT_RESULT_OK)
        {
            printf("%.1f C (%.1f F), %.1f%% humidity\n", temperature_c, celsius_to_fahrenheit(temperature_c), humidity);
        } else if (result == DHT_RESULT_TIMEOUT)
        {
            puts("DHT sensor not responding. Please check your wiring.");
        } 
        else
        {
            assert(result == DHT_RESULT_BAD_CHECKSUM);
            puts("Bad checksum");
        }
		vTaskDelay(500);
	}	
}
*/

// Task 5 -> Turn on/off local alarm 
void vTask_alarm(void *pvParameters)
{
	node_t *node =  (node_t *)pvParameters ;  // retorna un puntero a node_t
 	BaseType_t xStatus;
	
	// Led Indicator for active alarm
	gpio_init(PIN_ALARM);
    gpio_set_dir(PIN_ALARM,GPIO_OUT);

	while(true)
	{
		// Recibir los datos de la cola
        xStatus = xQueueReceive( xQueue , node , portMAX_DELAY);
        
        if(xStatus == pdPASS)
        {	
    		//if( ALARM_TRIGGER_TEMP < node->bmp280.temperature || ALARM_TRIGGER_PRES < node->bmp280.pressure || ALARM_TRIGGER_HUM < node->bmp280.humidity  )  
    		if( ALARM_TRIGGER_TEMP < node->bmp280.temperature )  	
        	{
				gpio_put(PIN_ALARM, LED_STATUS_ON);  // Turn on Alarm
				node->alarm = true;
				printf("Alarm: %d < %d \n", ALARM_TRIGGER_TEMP, node->bmp280.temperature ) ;

        	}
			else
			{
				gpio_put(PIN_ALARM, LED_STATUS_OFF); // Turn Off Alarm
				node->alarm = false;
			}
        }
        else
        {
        }     
        vTaskDelay(HAL_DELAY_ALARM);
	}
}

int main(void)
{
	// Init SDK Pico
	stdio_init_all();
	
	// New Node NODE_ID_ADDR
	node_t node;
	node.id = NODE_ID_ADDR ;
	node.counter = 0 ;
	node.alarm = false ;
	
	// create xQueue 
    xQueue = xQueueCreate(sizeof(node_t), 1);
	
	if(xQueue)
	{
		xTaskCreate(vTask_read_bmp280,TASK_READ_BMP280_NAME, UX_STACK_SIZE_TASK_READ, &node, IDLE_PRIORITY_TASK_READ, NULL);
		xTaskCreate(vTask_send_lora,TASK_SEND_LORA_NAME	, UX_STACK_SIZE_TASK_LORA, &node, IDLE_PRIORITY_TASK_LORA, NULL);
		xTaskCreate(vTask_alarm,TASK_ALARM_NAME	, UX_STACK_SIZE_TASK_ALARM, &node, IDLE_PRIORITY_TASK_ALARM, NULL);
		xTaskCreate(vTask_send_usb,TASK_ALARM_NAME, UX_STACK_SIZE_TASK_USB, &node, IDLE_PRIORITY_TASK_USB, NULL);
		//xTaskCreate(vTask_read_dhTASK_ALARM_NAME	11,TASK_SEND_LORA_NAME	, UX_STACK_SIZE_TASK_LORA, NULL, IDLE_PRIORITY_TASK_LORA, NULL);
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
