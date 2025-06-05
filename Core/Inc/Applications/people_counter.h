/*
 * people_counter.h
 *
 *  Created on: Jun 6, 2025
 *      Author: Admin
 */

#ifndef INC_APPLICATIONS_PEOPLE_COUNTER_H_
#define INC_APPLICATIONS_PEOPLE_COUNTER_H_

#include <stdint.h>
#include <stdbool.h>
#include "pir_sensor_service.h"

// Thời gian tối đa (ms) giữa việc kích hoạt 2 PIR để được coi là một lượt di chuyển hợp lệ
#define PERSON_COUNTER_MAX_TRANSITION_TIME_MS  1500

// Thời gian "cooldown" sau khi một người được đếm
#define PERSON_COUNTER_COOLDOWN_MS             2000


// Callback để thông báo có người đi qua
typedef enum {
    PERSON_PASSED_NONE,
    PERSON_PASSED_ENTERED,
    PERSON_PASSED_EXITED
} PersonPassedDirection_t;

typedef void (*person_passed_callback_t)(PersonPassedDirection_t direction);


/**
 * @brief Initializes the People Counter module.
 * @param callback: Optional callback function to be called when a person is detected passing.
 *                  Pass NULL if no callback is needed.
 */
void PeopleCounter_Init(person_passed_callback_t callback);

/**
 * @brief Processes the PIR sensor states to detect direction and update count.
 *        This function should be called periodically in the main application loop.
 */
void PeopleCounter_Process(void);

/**
 * @brief Gets the current estimated number of people in the room.
 * @return The number of people.
 */
int16_t PeopleCounter_GetCount(void);

/**
 * @brief Resets the people counter to zero.
 */
void PeopleCounter_Reset(void);

#endif /* INC_APPLICATIONS_PEOPLE_COUNTER_H_ */
