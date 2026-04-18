#ifndef APP_DRIVERS_SYSVIEW_MACROS_H_
#define APP_DRIVERS_SYSVIEW_MACROS_H_

#include <inttypes.h>

uint32_t SEGGER_SYSVIEW_X_GetTimestamp(void);

#if SEGGER_SYSTEMVIEW
  #include <SEGGER_SYSVIEW.h>

  #define SYSVIEW_START()            SEGGER_SYSVIEW_Start()
  #define SYSVIEW_CONF()             SEGGER_SYSVIEW_Conf()
  #define SYSVIEW_MARK_STOP(x)       SEGGER_SYSVIEW_MarkStop(x)
  #define SYSVIEW_MARK_START(x)      SEGGER_SYSVIEW_MarkStart(x)
  #define SYSVIEW_MARK(x)            SEGGER_SYSVIEW_Mark(x)
  #define SYSVIEW_NAME_MARK(x, text) SEGGER_SYSVIEW_NameMarker(x, text)
  #define SYSVIEW_ON_TASK_START(x)   SEGGER_SYSVIEW_OnTaskStartExec(x)
  #define SYSVIEW_ON_TASK_STOP(x)    SEGGER_SYSVIEW_OnTaskStopReady(x, 0)
  #define SYSVIEW_ON_TASK_EXEC(x)    SEGGER_SYSVIEW_OnTaskStartExec(x)
  #define SYSVIEW_ON_IDLE()          SEGGER_SYSVIEW_OnIdle()
  #define SYSVIEW_SEND_TASK_INFO(task_id, name, priority, stack_base, stack_size) \
      SEGGER_SYSVIEW_SendTaskInfo(task_id, name, priority, stack_base, stack_size)
  #define SYSVIEW_ADD_TASK(p_task, name, priority) SEGGER_SYSVIEW_AddTask(p_task, name, priority)
  #define SYSVIEW_RECORD_ENTER_ISR()               SEGGER_SYSVIEW_RecordEnterISR()
  #define SYSVIEW_RECORD_EXIT_ISR()                SEGGER_SYSVIEW_RecordExitISR()
#else
  #define SYSVIEW_START()                                                         ((void)0)
  #define SYSVIEW_CONF()                                                          ((void)0)
  #define SYSVIEW_MARK_STOP(x)                                                    ((void)0)
  #define SYSVIEW_MARK_START(x)                                                   ((void)0)
  #define SYSVIEW_MARK(x)                                                         ((void)0)
  #define SYSVIEW_NAME_MARK(x, text)                                              ((void)0)
  #define SYSVIEW_ON_TASK_START(x)                                                ((void)0)
  #define SYSVIEW_ON_TASK_STOP(x)                                                 ((void)0)
  #define SYSVIEW_ON_TASK_EXEC(x)                                                 ((void)0)
  #define SYSVIEW_ON_IDLE()                                                       ((void)0)
  #define SYSVIEW_SEND_TASK_INFO(task_id, name, priority, stack_base, stack_size) ((void)0)
  #define SYSVIEW_ADD_TASK(p_task, name, priority)                                ((void)0)
  #define SYSVIEW_RECORD_ENTER_ISR()                                              ((void)0)
  #define SYSVIEW_RECORD_EXIT_ISR()                                               ((void)0)
#endif

#endif  // APP_DRIVERS_SYSVIEW_MACROS_H_
