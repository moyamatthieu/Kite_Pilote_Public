// filepath: /workspaces/Kite_Pilote_Public/src/tasks.cpp
#include "core/tasks.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>
#include <WiFi.h>
#include "modules/sensor_module.h"
#include "modules/servo_module.h"
#include "modules/autopilot_module.h"
#include "modules/diagnostic_module.h"
#include "utils/logger.h"

// Déclaration des instances externes
extern SensorModule sensors;
extern ServoModule servos;
extern AutopilotModule autopilot;
extern void mettreAJourAffichage(uint8_t screen);

// Handles des queues et event group
QueueHandle_t xQueueSensorToControl = NULL;
QueueHandle_t xQueueSensorToDisplay = NULL;
EventGroupHandle_t xDiagEventGroup = NULL;

static void vTaskSensor(void* pvParameters) {
    SensorMessage_t msg;
    for (;;) {
        sensors.update();
        msg.imuData = sensors.getIMUData();
        msg.lineData = sensors.getLineData();
        msg.windData = sensors.getWindData();
        xQueueOverwrite(xQueueSensorToControl, &msg);
        xQueueOverwrite(xQueueSensorToDisplay, &msg);
        xEventGroupSetBits(xDiagEventGroup, BIT_SENSOR_OK);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void vTaskControl(void* pvParameters) {
    SensorMessage_t msg;
    for (;;) {
        if (xQueueReceive(xQueueSensorToControl, &msg, portMAX_DELAY) == pdPASS) {
            autopilot.update(msg.imuData, msg.lineData, msg.windData);
            servos.setDirectionAngle(autopilot.getTargetDirectionAngle());
            servos.setTrimAngle(autopilot.getTargetTrimAngle());
            servos.setWinchMode(autopilot.getTargetWinchMode());
            if (autopilot.getTargetWinchMode() == WINCH_MODE_GENERATOR) {
                servos.setWinchPower(autopilot.getTargetWinchPower());
            }
            servos.update();
            xEventGroupSetBits(xDiagEventGroup, BIT_CONTROL_OK);
        }
    }
}

static void vTaskDisplay(void* pvParameters) {
    SensorMessage_t msg;
    for (;;) {
        if (xQueueReceive(xQueueSensorToDisplay, &msg, portMAX_DELAY) == pdPASS) {
            mettreAJourAffichage(1);
            mettreAJourAffichage(2);
            xEventGroupSetBits(xDiagEventGroup, BIT_DISPLAY_OK);
        }
    }
}

static void vTaskWiFi(void* pvParameters) {
    uint8_t retryCount = 0;
    constexpr uint8_t maxRetries = 5;
    
    for (;;) {
        if (WiFi.status() != WL_CONNECTED) {
            if (retryCount < maxRetries) {
                WiFi.reconnect();
                retryCount++;
            } else {
                // Réinitialisation du WiFi si trop de tentatives échouées
                WiFi.disconnect();
                delay(100);
                WiFi.begin(WIFI_SSID, WIFI_PASS);
                retryCount = 0;
            }
        } else {
            xEventGroupSetBits(xDiagEventGroup, BIT_WIFI_OK);
            retryCount = 0; // Réinitialiser le compteur en cas de connexion réussie
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void vTaskDiag(void* pvParameters) {
    DiagnosticModule diag;
    for (;;) {
        EventBits_t bits = xEventGroupWaitBits(
            xDiagEventGroup,
            BIT_ALL_OK,
            pdTRUE,
            pdTRUE,
            pdMS_TO_TICKS(DIAG_CHECK_INTERVAL_MS)
        );
        if ((bits & BIT_ALL_OK) != BIT_ALL_OK) {
            // diag.runSensorsDiagnostic(); // Appel supprimé car méthode privée
            // TODO: Ajouter ici un diagnostic global public si besoin
        }
    }
}

void vCreateTasks() {
    xQueueSensorToControl = xQueueCreate(QUEUE_SENSOR_LENGTH, sizeof(SensorMessage_t));
    xQueueSensorToDisplay = xQueueCreate(QUEUE_CONTROL_LENGTH, sizeof(SensorMessage_t));
    xDiagEventGroup = xEventGroupCreate();
    xTaskCreatePinnedToCore(vTaskSensor, "TaskSensor", TASK_SENSOR_STACK, NULL, TASK_SENSOR_PRIORITY, NULL, 1);
    xTaskCreatePinnedToCore(vTaskControl, "TaskControl", TASK_CONTROL_STACK, NULL, TASK_CONTROL_PRIORITY, NULL, 1);
    xTaskCreatePinnedToCore(vTaskDisplay, "TaskDisplay", TASK_DISPLAY_STACK, NULL, TASK_DISPLAY_PRIORITY, NULL, 0);
    xTaskCreatePinnedToCore(vTaskWiFi, "TaskWiFi", TASK_WIFI_STACK, NULL, TASK_WIFI_PRIORITY, NULL, 1);
    xTaskCreatePinnedToCore(vTaskDiag, "TaskDiag", TASK_DIAG_STACK, NULL, TASK_DIAG_PRIORITY, NULL, 1);
}
