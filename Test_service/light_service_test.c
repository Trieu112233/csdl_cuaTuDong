#include "rcc_config.h"
#include "systick_driver.h"
#include "gpio_driver.h"
#include "light_control_service.h" // Service cần kiểm tra

#define LIGHT_RELAY_PORT         GPIOB
#define LIGHT_RELAY_PIN          8
#define LIGHT_RELAY_PIN_NUM      (1U << LIGHT_RELAY_PIN) 

int main(void) {
    SystemClock_Config(); 
    SysTick_Init();       

    LightService_Init(LIGHT_RELAY_PORT, LIGHT_RELAY_PIN_NUM);

    while (1) {
        LightService_TurnOn();
        Delay_ms(2000); // Bật đèn trong 2 giây
        LightService_TurnOff();
        Delay_ms(2000); // Tắt đèn trong 2 giây
    }

}