#include <Arduino.h>
#include <stdio.h>
#include "freertos/task.h"
#include "esp_timer.h"

volatile uint32_t idleCounter = 0;
uint32_t measureIntervalMs = 1000;  // 1 second measurement interval
uint32_t lastIdleCounter = 0;
uint32_t lastTotalTicks = 0;

// Custom idle task to count idle time
void idleTask(void* pvParameters) {
  for (;;) {
    idleCounter++;  // Increment idle counter when idle task is running (1ms)
    delay(1);       // Simulate idle work. 1 tick equals 1 ms
  }
}


void measure_idle_cpu() {
// Simulate workload in the main loop


  // Get the current tick count
  uint32_t currentTicks = xTaskGetTickCount();

  // Calculate time spent in idle since the last loop iteration
  uint32_t idleDiff = idleCounter - lastIdleCounter;
  uint32_t totalTickDiff = currentTicks - lastTotalTicks;

  // Update the last counters for next calculation
  lastIdleCounter = idleCounter;
  lastTotalTicks = currentTicks;

  // Avoid division by zero
  if (totalTickDiff == 0) {
    totalTickDiff = 1;
  }

  // Calculate the idle time as a percentage of the total time
  float idlePercentage = ((float)idleDiff / (float)totalTickDiff) * 100.0;

  // Print the estimated CPU idle time percentage
  printf("Estimated CPU Idle Time: %.2f%%\n", idlePercentage);
  printf("Estimated CPU Idle Time: %.2f%%\n", idlePercentage);

  // Wait for the next measurement interval
  //delay(measureIntervalMs);

}
