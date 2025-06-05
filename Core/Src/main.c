#include "systick_driver.h"
#include "gpio_driver.h"           // For GPIOA, GPIOB, GPIOC definitions
#include "light_control_service.h"
#include "rcc_config.h"
#include "limit_switch_service.h"

#define PIR1_PORT	GPIOB
#define PIR1_PIN	8
#define PIR2_PORT	GPIOB
#define PIR2_PIN	9

#define RELAY1_PORT						GPIOC
#define RELAY1_PIN						9
#define PIR_NVIC_PRIORITY				2

static bool opened = true; // Theo dõi trạng thái trước đó của PIR1
static bool	closed = false;

int main(void){
  SystemClock_Config(); // Configure system clocks using your rcc_config.c
  SysTick_Init();         // Initialize SysTick for Delay_ms and GetTick


  LimitSwitchService_Init(PIR1_PORT, PIR1_PIN, PIR2_PORT, PIR2_PIN, PIR_NVIC_PRIORITY);
  LightService_Init(RELAY1_PORT, RELAY1_PIN);
  LightService_TurnOff();

  while (1){
	  opened = LimitSwitchService_IsDoorFullyOpen();
	  closed = LimitSwitchService_IsDoorFullyClosed();

	  if(opened == true){
		  LightService_TurnOn();
	  } else {
		  LightService_TurnOff();
	  }
	  Delay_ms(100); // Kiểm tra trạng thái PIR mỗi 100ms
  }
  /* USER CODE END 3 */
}
