#include <string.h>
#include <pico/stdlib.h>
#include "LoRa-RP2040.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "bmp280_i2c.h"


// Define pin for gpio leds
#define PIN_LED	25

// Define status on/off
#define LED_STATUS_ON   1
#define LED_STATUS_OFF  0

// Define Stack Names
#define TASK_READ_BMP280_NAME	"Task-Read-bmp280"
#define TASK_SEND_LORA_NAME		"Task-Send-LoRa"
#define TASK_SEND_USB_NAME		"Task-Send-usb"

// Define Stack Size for tasks
#define UX_STACK_SIZE_TASK_READ		256
#define UX_STACK_SIZE_TASK_LORA		256
#define UX_STACK_SIZE_TASK_USB		256

// Define priority for tasks
#define IDLE_PRIORITY_TASK_READ		1
#define IDLE_PRIORITY_TASK_LORA		1
#define IDLE_PRIORITY_TASK_USB		1

// Dedine ID_ADDR for node
#define NODE_ID_ADDR	1001

// Delay for Task
#define HAL_DELAY_READ_SENSOR	3000
#define HAL_DELAY_SEND_LORA	6000
#define HAL_DELAY_SEND_USB 6000
#define HAL_DELAY_LED_BLINK	200


// payload = [id][temp][press][hum][gps-lat][gps-long][gps-nsat][counter]

typedef struct
{
	uint32_t temperature;
	uint32_t pressure;
	uint32_t humidity;
}sensor_t;

typedef struct
{
	//char payload[NODE_PAYLOAD_SIZE];
	uint32_t id;
	uint32_t counter;
	sensor_t bmp280;
	//sensor_t gps;
}node_t ;


// shared vtask1 and vtask2 
QueueHandle_t xQueue;  

// --- task 1 --- 

void vTask_read_sensor(void *pvParameters)
{    
   
    node_t *node =  (node_t *)pvParameters ;  // retorna un puntero a node_t
    
 	BaseType_t xStatus;
   
   	//****** BMP280 **************************************
    int32_t raw_temperature=0;
    int32_t raw_pressure=0;

	int32_t temperature=0;
	int32_t pressure=0; 
   	

	//#else
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
       		//node->bmp280.pressure
       		
       		// for debug
       		printf("Temp = %d , Press = %d  , Hum = 0\n", node->bmp280.temperature, node->bmp280.pressure) ;
        }
        else
        {
        
        }
   		vTaskDelay(HAL_DELAY_READ_SENSOR);
   	  
   	  // get tempeture 
      //temperature = sensorBmp280.convertTemp(raw_temperature);
      
      // get pressure
      // pressure = sensorBmp280.convertPressure(raw_pressure, raw_temperature);
   
   	  //printf("Temp = %d , Press = %d  \n", temperature, pressure) ;
   	  //printf("Raw Temp = %d , Raw Press = %d  \n", raw_temperature, raw_pressure) ;
   	//	printf("Task 1 Led ON!\n");
        //gpio_put(PIN_LED, LED_STATUS_ON);
        //vTaskDelay(HAL_DELAY);
        
     //   printf("Task 1 Led OFF!\n");
        //gpio_put(PIN_LED, LED_STATUS_OFF);
        //vTaskDelay(HAL_DELAY);
   }
}

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
		//printf("Sending packet: ");
		//printf("[%d] = ",counter);
		//printf("123456789 \n") ;
		// send packet

		   		// Recibir los datos de la cola
        xStatus = xQueueReceive( xQueue , node , portMAX_DELAY);
        
        if(xStatus == pdPASS)
        {
			LoRa.beginPacket();
			LoRa.print(node->id);
			LoRa.print(node->bmp280.temperature) ;
			LoRa.print(node->bmp280.pressure) ;
			LoRa.print(node->counter) ;
			LoRa.endPacket(true);

					
			// Blink Led for send data
			gpio_put(PIN_LED, LED_STATUS_ON);
			vTaskDelay(HAL_DELAY_LED_BLINK);
	 		gpio_put(PIN_LED, LED_STATUS_OFF);
		
			//for debug
			printf("LoRa Node Package [%d] :",node->counter);
			printf("%d",node->id);
			printf("%d",node->bmp280.temperature);
			printf("%d",node->bmp280.pressure);
			printf("%d",node->counter);
			printf("\n") ;
			// add couter
			node->counter+=1; 
		}
		else
		{
		
		}
		vTaskDelay(HAL_DELAY_SEND_LORA); // 
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
	
	// New Node NODE_ID_ADDR
	node_t node;
	node.id = NODE_ID_ADDR ;
	node.counter = 0 ;
	
	// create xQueue 
    xQueue = xQueueCreate(sizeof(node_t), 1);
	
	if(xQueue)
	{
		xTaskCreate(vTask_read_sensor,TASK_READ_BMP280_NAME, UX_STACK_SIZE_TASK_READ, &node, IDLE_PRIORITY_TASK_READ, NULL);
		xTaskCreate(vTask_send_lora,TASK_SEND_LORA_NAME	, UX_STACK_SIZE_TASK_LORA, &node, IDLE_PRIORITY_TASK_LORA, NULL);
		//xTaskCreate(vTask_send,"Task send usb", UX_STACK_SIZE_TASK_USB, &node, IDLE_PRIORITY_TASK_USB, NULL);
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
