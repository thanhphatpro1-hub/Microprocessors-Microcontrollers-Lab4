/*
 * scheduler.h
 *
 *  Created on: Nov 11, 2026
 *      Author: Admin
 */

#ifndef INC_SCHEDULER_H_
#define INC_SCHEDULER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>
#include <stddef.h>

/* Exported types ------------------------------------------------------------*/
/**
 * @brief Cấu trúc dữ liệu của một Task trong Delta-Queue
 */
typedef struct {
    void (*pTask)(void);    /*!< Con trỏ hàm trỏ tới tác vụ cần thực thi (void (void)) */
    uint32_t Delay;         /*!< Thời gian trễ tương đối (Delta ticks) so với task đứng trước */
    uint32_t Period;        /*!< Chu kỳ lặp lại (ticks); bằng 0 nếu là One-shot task */
    uint8_t RunMe;          /*!< Cờ báo hiệu task đã đến hạn thực thi (tăng bởi SCH_Update) */
    uint32_t TaskID;        /*!< Định danh duy nhất của task để quản lý và xóa */
} sTask;

/* Exported constants & macros -----------------------------------------------*/
#define SCH_MAX_TASKS                                   10U
#define NO_TASK_ID                                      0U

#define RETURN_NORMAL                                   0U
#define RETURN_ERROR                                    1U

#define ERROR_SCH_NO_ERROR                              0U
#define ERROR_SCH_TOO_MANY_TASKS                        1U
#define ERROR_SCH_CANNOT_DELETE_TASK                    2U
#define ERROR_SCH_WAITING_FOR_SLAVE_TO_ACK              3U
#define ERROR_SCH_WAITING_FOR_START_COMMAND_FROM_MASTER 4U
#define ERROR_SCH_ONE_OR_MORE_SLAVES_DID_NOT_START      5U
#define ERROR_SCH_LOST_SLAVE                            6U
#define ERROR_SCH_CAN_BUS_ERROR                         7U
#define ERROR_I2C_WRITE_BYTE_AT24C64                    8U

/* Exported variables --------------------------------------------------------*/
extern sTask SCH_tasks_G[SCH_MAX_TASKS];
extern uint8_t Error_code_G;

/* Exported functions prototypes ---------------------------------------------*/
void SCH_Init(void);
void SCH_Update(void);
uint32_t SCH_Add_Task(void (*pFunction)(void), uint32_t DELAY, uint32_t PERIOD);
void SCH_Dispatch_Tasks(void);
uint8_t SCH_Delete_Task(uint32_t taskID);
void SCH_Report_Status(void);
void SCH_Go_To_Sleep(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_SCHEDULER_H_ */
