/*
 * door_fsm.h
 *
 *  Created on: Jun 6, 2025
 *      Author: Admin
 */

#ifndef INC_APPLICATIONS_DOOR_FSM_H_
#define INC_APPLICATIONS_DOOR_FSM_H_

#include <stdint.h>
#include <stdbool.h>

// Định nghĩa các trạng thái của cửa
typedef enum {
    DOOR_STATE_CLOSED,
    DOOR_STATE_OPENING,
    DOOR_STATE_OPEN,
    DOOR_STATE_CLOSING,
    DOOR_STATE_ERROR,
	DOOR_STATE_INIT
} DoorState_t;

typedef enum {
    SYSTEM_MODE_NORMAL,
    SYSTEM_MODE_FORCE_OPEN,
    SYSTEM_MODE_FORCE_CLOSE,
} SystemOpMode_t;

#define DOOR_AUTO_CLOSE_TIMEOUT_MS  5000
#define DOOR_MAX_TRAVEL_TIME_MS   	10000

void DoorFSM_Init(void);
void DoorFSM_Process(void);
DoorState_t DoorFSM_GetState(void);
void DoorFSM_NotifySystemModeChange(SystemOpMode_t new_mode);
void DoorFSM_NotifyPersonDetectedPassing(void);

#endif /* INC_APPLICATIONS_DOOR_FSM_H_ */
