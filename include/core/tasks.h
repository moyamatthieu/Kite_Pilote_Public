// filepath: /workspaces/Kite_Pilote_Public/include/core/tasks.h
#ifndef TASKS_H
#define TASKS_H

#include "core/config.h"
#include "core/data_types.h"
#include "modules/sensor_module.h"
#include "modules/servo_module.h"
#include "modules/autopilot_module.h"
#include "modules/lcd_module.h"
#include <freertos/queue.h>
#include <freertos/event_groups.h>

// Message contenant les données capteurs pour les autres tâches
typedef struct {
    IMUData imuData;
    LineData lineData;
    WindData windData;
} SensorMessage_t;

// Handles des queues et de l'event group
extern QueueHandle_t xQueueSensorToControl;
extern QueueHandle_t xQueueSensorToDisplay;
extern EventGroupHandle_t xDiagEventGroup;

// Création des tâches et initialisation des ressources FreeRTOS
void vCreateTasks();

#endif // TASKS_H
