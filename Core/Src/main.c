#include <stdio.h>  
#include <string.h> 

// Application Logic
#include "system_manager.h"       // Module điều phối chính
#include "people_counter.h"
#include "door_fsm.h"
#include "lighting_logic.h"

int main(void) {
    // Khởi tạo hệ thống
    SystemClock_Config(); // Cấu hình clock hệ thống (từ rcc_config.c)
    SysTick_Init();       // Khởi tạo SysTick cho tick 1ms và Delay_ms()
    SystemManager_Init();

    // Vòng lặp chính
    while (1) {
        // Xử lý các module
        SystemManager_Process();
    }

    return 0; 
}    
