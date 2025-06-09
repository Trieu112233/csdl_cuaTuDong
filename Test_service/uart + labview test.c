#include "rcc_config.h"
#include "systick_driver.h"
#include "gpio_driver.h"
#include "uart_protocol_service.h" // Service to test

// LED Definitions
#define LED_ID_PORT             GPIOB
#define LED_ID_PIN              8
#define LED_ID_PIN_MASK         (1U << LED_ID_PIN) 

#define LED_NORMAL_PORT        GPIOB
#define LED_NORMAL_PIN         9
#define LED_NORMAL_PIN_MASK    (1U << LED_NORMAL_PIN) 

#define LED_OPEN_PORT        GPIOC
#define LED_OPEN_PIN         8
#define LED_OPEN_PIN_MASK    (1U << LED_OPEN_PIN) 

#define LED_CLOSE_PORT        GPIOC
#define LED_CLOSE_PIN         6
#define LED_CLOSE_PIN_MASK    (1U << LED_CLOSE_PIN) 

// Global state variables for simulation
static uint8_t door_state = PAYLOAD_DOOR_CLOSED;
static uint8_t people_count = 0;
static uint8_t system_mode = PAYLOAD_MODE_NORMAL;
static uint8_t light_state = PAYLOAD_LIGHT_OFF;

// Callback handler for commands received from LabVIEW
bool main_labview_command_handler(const ParsedFrame_t* parsed_frame) {
    bool processed_successfully = false;
    uint8_t ack_payload_id = parsed_frame->id; // For ACK frame

    // Visual indication of command reception/processing attempt
    GPIO_WritePin(LED_ID_PORT, LED_ID_PIN_MASK, GPIO_PIN_SET);
    Delay_ms(30); // Short blink
    GPIO_WritePin(LED_ID_PORT, LED_ID_PIN_MASK, GPIO_PIN_RESET);

    if (parsed_frame->type == FRAME_TYPE_LABVIEW_TO_STM) {
        switch (parsed_frame->id) {
            case FRAME_ID_LABVIEW_SET_MODE:
                if (parsed_frame->length == 1) {
                    uint8_t requested_mode = parsed_frame->payload[0];
                    if (requested_mode == PAYLOAD_MODE_NORMAL ||
                        requested_mode == PAYLOAD_MODE_FORCE_OPEN ||
                        requested_mode == PAYLOAD_MODE_FORCE_CLOSE) {
                        
                        system_mode = requested_mode; // Update global system_mode
                        processed_successfully = true;

                        // Specific LED indication for mode
                        if (system_mode == PAYLOAD_MODE_NORMAL) {
                            GPIO_WritePin(LED_NORMAL_PORT, LED_NORMAL_PIN_MASK, GPIO_PIN_SET); // Turn on Normal LED
                            Delay_ms(50);
                            GPIO_WritePin(LED_NORMAL_PORT, LED_NORMAL_PIN_MASK, GPIO_PIN_RESET); // Turn off Normal LED
                        } else if (system_mode == PAYLOAD_MODE_FORCE_OPEN) {
                            GPIO_WritePin(LED_OPEN_PORT, LED_OPEN_PIN_MASK, GPIO_PIN_SET); // Turn on Open LED
                            Delay_ms(50);
                            GPIO_WritePin(LED_OPEN_PORT, LED_OPEN_PIN_MASK, GPIO_PIN_RESET); // Turn off Open LED
                        } else if (system_mode == PAYLOAD_MODE_FORCE_CLOSE) {
                            GPIO_WritePin(LED_CLOSE_PORT, LED_CLOSE_PIN_MASK, GPIO_PIN_SET); // Turn on Close LED
                            Delay_ms(50);
                            GPIO_WritePin(LED_CLOSE_PORT, LED_CLOSE_PIN_MASK, GPIO_PIN_RESET); // Turn off Close LED
                        }
                    } else {
                    }
                } else {
                }
                break;

            case FRAME_ID_LABVIEW_RESET_COUNT:
                if (parsed_frame->length == 0) {
                    people_count = 0; // Reset global people_count
                    processed_successfully = true;
                    // Blink ID LED multiple times for reset
                    for(int i=0; i<3; ++i) {
                        GPIO_WritePin(LED_ID_PORT, LED_ID_PIN_MASK, GPIO_PIN_SET); Delay_ms(50);
                        GPIO_WritePin(LED_ID_PORT, LED_ID_PIN_MASK, GPIO_PIN_RESET); Delay_ms(50);
                    }
                } else {
                }
                break;

            default:
                break;
        }
    }

    if (processed_successfully) {
        // Send ACK back to LabVIEW
        UARTProto_SendFrame(FRAME_TYPE_STM_TO_LABVIEW, FRAME_ID_STM_COMMAND_ACK, &ack_payload_id, 1);
    }
    return processed_successfully;
}


int main(void) {
    SystemClock_Config(); 
    SysTick_Init();       

    // Initialize LEDs
    GPIO_InitPin(LED_ID_PORT, LED_ID_PIN_MASK, GPIO_MODE_OUTPUT, GPIO_PULL_NO, GPIO_SPEED_LOW, GPIO_OTYPE_PUSHPULL, 0);
    GPIO_InitPin(LED_NORMAL_PORT, LED_NORMAL_PIN_MASK, GPIO_MODE_OUTPUT, GPIO_PULL_NO, GPIO_SPEED_LOW, GPIO_OTYPE_PUSHPULL, 0);
    GPIO_InitPin(LED_OPEN_PORT, LED_OPEN_PIN_MASK, GPIO_MODE_OUTPUT, GPIO_PULL_NO, GPIO_SPEED_LOW, GPIO_OTYPE_PUSHPULL, 0);
    GPIO_InitPin(LED_CLOSE_PORT, LED_CLOSE_PIN_MASK, GPIO_MODE_OUTPUT, GPIO_PULL_NO, GPIO_SPEED_LOW, GPIO_OTYPE_PUSHPULL, 0);
    
    // Turn off all LEDs initially
    GPIO_WritePin(LED_ID_PORT, LED_ID_PIN_MASK, GPIO_PIN_RESET);
    GPIO_WritePin(LED_NORMAL_PORT, LED_NORMAL_PIN_MASK, GPIO_PIN_RESET);
    GPIO_WritePin(LED_OPEN_PORT, LED_OPEN_PIN_MASK, GPIO_PIN_RESET);
    GPIO_WritePin(LED_CLOSE_PORT, LED_CLOSE_PIN_MASK, GPIO_PIN_RESET);

    // UARTProto_Init calls UART2_Init internally
    UARTProto_Init(main_labview_command_handler);

    uint32_t last_sent_tick = GetTick();

    while (1) {
        UARTProto_Process(); // Process incoming UART data and commands

        if(UARTProto_CheckErrors()){
        }

        // Periodically send a full system snapshot (every 2 seconds)
        if (GetTick() - last_sent_tick >= 2000) {
            last_sent_tick = GetTick();

            ParsedFrame_t status_frame;
            status_frame.type = FRAME_TYPE_STM_TO_LABVIEW;
            status_frame.id = FRAME_ID_STM_FULL_SNAPSHOT;
            status_frame.length = 4; 
            status_frame.payload[0] = system_mode;  // Current system mode
            status_frame.payload[1] = door_state;   // Current door state
            status_frame.payload[2] = people_count; // Current people count
            status_frame.payload[3] = light_state;  // Current light state

            if(UARTProto_SendFrame(status_frame.type, status_frame.id, status_frame.payload, status_frame.length)){
            }

            // Cycle through door_state for testing snapshot (0-4)
            if (door_state < PAYLOAD_DOOR_ERROR) { // PAYLOAD_DOOR_ERROR is 0x04
                door_state++;
            } else {
                door_state = PAYLOAD_DOOR_CLOSED; // Reset to 0x00
            }

            if (people_count < 19) { // Max 19 to make it 0-19 cycle
                people_count++;
            } else {
                people_count = 0; 
            }
            light_state = (light_state + 1) % 2;
        }
        
        Delay_ms(10); // Small delay to reduce CPU load
    }
}
