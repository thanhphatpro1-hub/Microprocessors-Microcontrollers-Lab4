/*
 * scheduler.c
 *
 *  Created on: Nov 11, 2026
 *      Author: Admin
 */

/* Includes ------------------------------------------------------------------*/
#include "scheduler.h"

/* Global variables ----------------------------------------------------------*/
sTask SCH_tasks_G[SCH_MAX_TASKS];
uint8_t Error_code_G = ERROR_SCH_NO_ERROR;

/* Private variables ---------------------------------------------------------*/
static uint32_t s_next_task_id = 0U;
static uint8_t Last_error_code_G = ERROR_SCH_NO_ERROR;
static uint32_t Error_tick_count_G = 0U;
static volatile uint32_t s_overdue_ticks = 0U;

/* Private function prototypes -----------------------------------------------*/
static void SCH_Clear_Slot(uint32_t index);
static void SCH_Shift_Left(uint32_t start_index);
static uint32_t SCH_Generate_Task_ID(void);
static uint8_t SCH_Insert_Task_To_Delta_Queue(void (*pFunction)(void),
                                              uint32_t DELAY,
                                              uint32_t PERIOD,
                                              uint32_t taskID,
                                              uint8_t initialRunMe);
static void SCH_Apply_Overdue_Ticks(void);

/* Private functions ---------------------------------------------------------*/

static void SCH_Clear_Slot(uint32_t index) {
    if (index < SCH_MAX_TASKS) {
        SCH_tasks_G[index].pTask  = NULL;
        SCH_tasks_G[index].Delay  = 0U;
        SCH_tasks_G[index].Period = 0U;
        SCH_tasks_G[index].RunMe  = 0U;
        SCH_tasks_G[index].TaskID = NO_TASK_ID;
    }
}

static void SCH_Shift_Left(uint32_t start_index) {
    if (start_index >= SCH_MAX_TASKS) {
        return;
    }
    for (uint32_t i = start_index; i < (SCH_MAX_TASKS - 1U); i++) {
        SCH_tasks_G[i] = SCH_tasks_G[i + 1U];
    }
    SCH_Clear_Slot(SCH_MAX_TASKS - 1U);
}

static uint32_t SCH_Generate_Task_ID(void) {
    uint8_t is_duplicate;
    do {
        s_next_task_id++;
        if ((s_next_task_id == NO_TASK_ID) || (s_next_task_id == SCH_MAX_TASKS)) {
            s_next_task_id++;
        }
        if (s_next_task_id == SCH_MAX_TASKS) {
            s_next_task_id++;
        }

        is_duplicate = 0U;
        for (uint32_t i = 0U; i < SCH_MAX_TASKS; i++) {
            if ((SCH_tasks_G[i].pTask != NULL) && (SCH_tasks_G[i].TaskID == s_next_task_id)) {
                is_duplicate = 1U;
                break;
            }
        }
    } while (is_duplicate != 0U);

    return s_next_task_id;
}

static uint8_t SCH_Insert_Task_To_Delta_Queue(void (*pFunction)(void),
                                              uint32_t DELAY,
                                              uint32_t PERIOD,
                                              uint32_t taskID,
                                              uint8_t initialRunMe) {
    if (pFunction == NULL) {
        return RETURN_ERROR;
    }

    if (SCH_tasks_G[SCH_MAX_TASKS - 1U].pTask != NULL) {
        Error_code_G = ERROR_SCH_TOO_MANY_TASKS;
        return RETURN_ERROR;
    }

    uint32_t sum_delay = 0U;
    uint32_t insert_index = 0U;

    while ((insert_index < SCH_MAX_TASKS) && (SCH_tasks_G[insert_index].pTask != NULL)) {
        if ((sum_delay + SCH_tasks_G[insert_index].Delay) <= DELAY) {
            sum_delay += SCH_tasks_G[insert_index].Delay;
            insert_index++;
        } else {
            break;
        }
    }

    uint32_t relative_delay = DELAY - sum_delay;

    for (uint32_t j = (SCH_MAX_TASKS - 1U); j > insert_index; j--) {
        SCH_tasks_G[j] = SCH_tasks_G[j - 1U];
    }

    if (((insert_index + 1U) < SCH_MAX_TASKS) && (SCH_tasks_G[insert_index + 1U].pTask != NULL)) {
        SCH_tasks_G[insert_index + 1U].Delay -= relative_delay;
    }

    SCH_tasks_G[insert_index].pTask  = pFunction;
    SCH_tasks_G[insert_index].Delay  = relative_delay;
    SCH_tasks_G[insert_index].Period = PERIOD;
    SCH_tasks_G[insert_index].RunMe  = initialRunMe;
    SCH_tasks_G[insert_index].TaskID = taskID;

    return RETURN_NORMAL;
}

static void SCH_Apply_Overdue_Ticks(void) {
    while ((s_overdue_ticks > 0U) && (SCH_tasks_G[0].pTask != NULL)) {
        if (SCH_tasks_G[0].Delay > s_overdue_ticks) {
            SCH_tasks_G[0].Delay -= s_overdue_ticks;
            s_overdue_ticks = 0U;
        } else if (SCH_tasks_G[0].Delay > 0U) {
            s_overdue_ticks -= SCH_tasks_G[0].Delay;
            SCH_tasks_G[0].Delay = 0U;
            SCH_tasks_G[0].RunMe = 1U;
            break;
        } else {
            SCH_tasks_G[0].RunMe = 1U;
            break;
        }
    }
}

/* Exported functions --------------------------------------------------------*/

void SCH_Init(void) {
    for (uint32_t i = 0U; i < SCH_MAX_TASKS; i++) {
        SCH_Clear_Slot(i);
    }
    Error_code_G = ERROR_SCH_NO_ERROR;
    Last_error_code_G = ERROR_SCH_NO_ERROR;
    Error_tick_count_G = 0U;
    s_next_task_id = 0U;
    s_overdue_ticks = 0U;
}

/**
 * @brief Cập nhật nhịp đếm ngắt Timer 10ms - Đạt O(1) tuyệt đối.
 */
void SCH_Update(void) {
    if (SCH_tasks_G[0].pTask != NULL) {
        if (SCH_tasks_G[0].Delay > 0U) {
            SCH_tasks_G[0].Delay--;
            if (SCH_tasks_G[0].Delay == 0U) {
                SCH_tasks_G[0].RunMe = 1U;
            }
        } else {
            SCH_tasks_G[0].RunMe = 1U;
            s_overdue_ticks++;
        }
    }
}

uint32_t SCH_Add_Task(void (*pFunction)(void), uint32_t DELAY, uint32_t PERIOD) {
    if (pFunction == NULL) {
        return SCH_MAX_TASKS;
    }

    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    if (SCH_tasks_G[SCH_MAX_TASKS - 1U].pTask != NULL) {
        Error_code_G = ERROR_SCH_TOO_MANY_TASKS;
        __set_PRIMASK(primask);
        return SCH_MAX_TASKS;
    }

    uint32_t new_task_id = SCH_Generate_Task_ID();
    uint8_t initial_run_me = (DELAY == 0U) ? 1U : 0U;

    if (SCH_Insert_Task_To_Delta_Queue(pFunction, DELAY, PERIOD, new_task_id, initial_run_me) != RETURN_NORMAL) {
        __set_PRIMASK(primask);
        return SCH_MAX_TASKS;
    }

    __set_PRIMASK(primask);
    return new_task_id;
}

void SCH_Dispatch_Tasks(void) {
    while ((SCH_tasks_G[0].pTask != NULL) && (SCH_tasks_G[0].RunMe > 0U)) {
        uint32_t primask = __get_PRIMASK();
        __disable_irq();

        void (*pTask_To_Run)(void) = SCH_tasks_G[0].pTask;
        uint32_t saved_period      = SCH_tasks_G[0].Period;
        uint32_t saved_task_id     = SCH_tasks_G[0].TaskID;

        /* Dịch hàng đợi sang trái TRƯỚC KHI chạy task để không bị ngắt Timer ghi đè */
        SCH_Shift_Left(0U);

        if (saved_period > 0U) {
            (void)SCH_Insert_Task_To_Delta_Queue(pTask_To_Run,
                                                 saved_period,
                                                 saved_period,
                                                 saved_task_id,
                                                 0U);
        }

        if ((SCH_tasks_G[0].pTask != NULL) && (SCH_tasks_G[0].Delay == 0U)) {
            SCH_tasks_G[0].RunMe = 1U;
        } else {
            SCH_Apply_Overdue_Ticks();
        }

        __set_PRIMASK(primask);

        /* Thực thi tác vụ ngoài vùng cấm ngắt */
        (*pTask_To_Run)();
    }

    SCH_Report_Status();
    SCH_Go_To_Sleep();
}

uint8_t SCH_Delete_Task(uint32_t taskID) {
    if (taskID == NO_TASK_ID) {
        Error_code_G = ERROR_SCH_CANNOT_DELETE_TASK;
        return RETURN_ERROR;
    }

    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    uint32_t task_index = SCH_MAX_TASKS;
    for (uint32_t i = 0U; i < SCH_MAX_TASKS; i++) {
        if ((SCH_tasks_G[i].pTask != NULL) && (SCH_tasks_G[i].TaskID == taskID)) {
            task_index = i;
            break;
        }
    }

    if (task_index == SCH_MAX_TASKS) {
        Error_code_G = ERROR_SCH_CANNOT_DELETE_TASK;
        __set_PRIMASK(primask);
        return RETURN_ERROR;
    }

    /* Bù trừ thời gian trễ cho task đứng sau */
    if (((task_index + 1U) < SCH_MAX_TASKS) && (SCH_tasks_G[task_index + 1U].pTask != NULL)) {
        SCH_tasks_G[task_index + 1U].Delay += SCH_tasks_G[task_index].Delay;
    }

    SCH_Shift_Left(task_index);

    if ((task_index == 0U) && (SCH_tasks_G[0].pTask != NULL) && (SCH_tasks_G[0].Delay == 0U)) {
        SCH_tasks_G[0].RunMe = 1U;
    }

    __set_PRIMASK(primask);
    return RETURN_NORMAL;
}

void SCH_Report_Status(void) {
#ifdef SCH_REPORT_ERRORS
    if (Error_code_G != Last_error_code_G) {
#ifdef Error_port
        Error_port = (uint8_t)(255U - Error_code_G);
#endif
        Last_error_code_G = Error_code_G;
        if (Error_code_G != ERROR_SCH_NO_ERROR) {
            Error_tick_count_G = 60000U;
        } else {
            Error_tick_count_G = 0U;
        }
    } else {
        if (Error_tick_count_G != 0U) {
            Error_tick_count_G--;
            if (Error_tick_count_G == 0U) {
                Error_code_G = ERROR_SCH_NO_ERROR;
            }
        }
    }
#else
    (void)Last_error_code_G;
    (void)Error_tick_count_G;
#endif
}

void SCH_Go_To_Sleep(void) {
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
}
