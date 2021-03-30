#ifndef DEBUGTASKS_H_
#define DEBUGTASKS_H_

// Debug tasks functions. Mostly copied from FreeRTOS documentation examples page
// for the functions used.
//
// This show the tasks CPU percentage and their minimal free stack size.
// Run DebugGetTasks() once or twice a minute to get the stats.
//
// All functions are in this header. No need to alter makefiles.
//
// This needs the FreeRTOS trace facilty and runtime stats enabled in menuconfig.
// Disable these when not using this function.
//

void DebugGetTasks() {
  TaskStatus_t *pxTaskStatusArray;
  volatile UBaseType_t uxArraySize, x;
  unsigned int ulStatsAsPercentage;
  unsigned int ulTotalRunTime;

  // Take a snapshot of the number of tasks in case it changes while this
  // function is executing.
  uxArraySize = uxTaskGetNumberOfTasks();

  // Allocate a TaskStatus_t structure for each task.  An array could be
  // allocated statically at compile time.
  pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

  if (pxTaskStatusArray != NULL) {
    // Generate raw status information about each task.
    uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);

    // For percentage calculations.
    ulTotalRunTime /= 100UL;

    ESP_LOGI(TAG, "Task list:");
    // Avoid divide by zero errors.
    if (ulTotalRunTime > 0) {
      // For each populated position in the pxTaskStatusArray array,
      // format the raw data as human readable ASCII data.
      for (x = 0; x < uxArraySize; x++) {
        // What percentage of the total run time has the task used?
        // This will always be rounded down to the nearest integer.
        // ulTotalRunTimeDiv100 has already been divided by 100.
        ulStatsAsPercentage = pxTaskStatusArray[x].ulRunTimeCounter / ulTotalRunTime;

        if (ulStatsAsPercentage > 0UL) {
          ESP_LOGI(TAG, "Task name: %s, CPU%%  %u, Runs %u, Rem Stack %u, Prio %u",
                   pxTaskStatusArray[x].pcTaskName, ulStatsAsPercentage,
                   pxTaskStatusArray[x].ulRunTimeCounter,
                   pxTaskStatusArray[x].usStackHighWaterMark,
                   pxTaskStatusArray[x].uxCurrentPriority);
        } else {
          // If the percentage is zero here then the task has
          // consumed less than 1% of the total run time.
          ESP_LOGI(TAG, "Task name: %s, CPU%% 0, Runs %u, Rem Stack %u, Prio %u",
                   pxTaskStatusArray[x].pcTaskName, pxTaskStatusArray[x].ulRunTimeCounter,
                   pxTaskStatusArray[x].usStackHighWaterMark,
                   pxTaskStatusArray[x].uxCurrentPriority);
        }
      }
    }

    // The array is no longer needed, free the memory it consumes.
    vPortFree(pxTaskStatusArray);
  }
}
#endif
