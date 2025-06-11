#include <stdio.h>
#include <string.h>

#include "rcc_config.h"
#include "systick_driver.h"
// Application Logic
#include "system_manager.h"       // Module điều phối chính
#include "people_counter.h"
#include "door_fsm.h"
#include "lighting_logic.h"

int main(void) {
   // Khởi tạo hệ thống
   SystemClock_Config();
   SysTick_Init();
   SystemManager_Init();

   // Vòng lặp chính
   while (1) {
       SystemManager_Process();
       Delay_ms(200);
   }

   return 0;
}


