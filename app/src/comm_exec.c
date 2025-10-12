#include "comm_exec.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>  // For uint16_t
#include "esp_log.h"
#include "driver/uart.h"
#include "eeprom.h"

/* Private variables */
TaskHandle_t CommTaskHandle;

// --- Global runtime state for modes and MDR ---
static float g_ADC_zero = 0.0f;          // offset
static float g_K_T = 0.0f;               // Nm per count
static uint32_t g_run_time_s = 60;       // default run duration (seconds)
static uint32_t g_run_start_ms = 0;

// Global variable for idle amplitude tare request
double g_idle_amp_tare_request = 0.0;

// Command parsing disabled on ESP32 build for now

/* Private function prototypes */
static void CommTask_Function(void *argument);
static void ModeTask_Function(void *argument);
// static void ParseCommand(char *cmd);

/**
  * @brief Initialize the communication task
  * @param None
  * @retval None
  */
static TaskHandle_t ModeTaskHandle;

void CommTask_Init(void)
{
  /* Configure UART0 for USB-UART bridge echo */
  const uart_config_t uart_config = {
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_APB,
  };
  uart_driver_install(UART_NUM_0, 2048, 2048, 0, NULL, 0);
  uart_param_config(UART_NUM_0, &uart_config);
  uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

  /* Load MDR calibration data from EEPROM */
  eeprom_calibration_data_t data = {0};
  uint8_t valid = 0;
  if (EEPROM_LoadAllCalibrationData(&data, &valid) == ESP_OK && valid) {
    g_ADC_zero = data.mdr_adc_zero;
    g_K_T = data.mdr_k_t;
    UART_Printf("Loaded MDR calibration: ADC_zero=%.3f, K_T=%.9f\r\n", g_ADC_zero, g_K_T);
  } else {
    UART_Printf("No MDR calibration found in EEPROM\r\n");
  }

  /* Create the task */
  xTaskCreate(CommTask_Function, "CommTask", 4096, NULL, tskIDLE_PRIORITY+1, &CommTaskHandle);
  xTaskCreate(ModeTask_Function, "ModeTask", 4096, NULL, tskIDLE_PRIORITY+1, &ModeTaskHandle);
}

/**
  * @brief Start the communication task
  * @param None
  * @retval None
  */
void CommTask_Start(void)
{
  /* No-op on ESP32 */
}

/* UART_Printf is provided in config.c */

/**
  * @brief Parse incoming command
  * @param cmd: Command string to parse
  * @retval None
  */
/* Command parsing removed in ESP32 build. */

/**
  * @brief Communication task function
  * @param argument: Not used
  * @retval None
  */
/* ---- Internal helpers for modularity ---- */

#define COMM_UART                 UART_NUM_0
#define COMM_UART_BAUD            115200
#define COMM_RXBUF_SIZE           256
#define COMM_LINEBUF_SIZE         256

static void uart_init_defaults(void)
{
  const uart_config_t uart_config = {
    .baud_rate = COMM_UART_BAUD,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    .source_clk = UART_SCLK_APB,
  };
  uart_driver_install(COMM_UART, 2048, 2048, 0, NULL, 0);
  uart_param_config(COMM_UART, &uart_config);
  uart_set_pin(COMM_UART, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

static void reply_ok(const char *cmd)
{
  if (cmd) {
    UART_Printf("{\"ok\":true,\"cmd\":\"%s\"}\r\n", cmd);
  } else {
    UART_Printf("{\"ok\":true}\r\n");
  }
}

static void reply_err(const char *err)
{
  UART_Printf("{\"ok\":false,\"err\":\"%s\"}\r\n", err ? err : "error");
}

static void relays_all_off(void)
{
  Relay_SSR_SetRelay(1, OFF);
  Relay_SSR_SetRelay(2, OFF);
  Relay_SSR_SetRelay(3, OFF);
  Relay_SSR_SetRelay(4, OFF);
}

static void relays_sequence_on(void)
{
  Relay_SSR_SetRelay(1, ON); vTaskDelay(pdMS_TO_TICKS(1000));
  Relay_SSR_SetRelay(2, ON); vTaskDelay(pdMS_TO_TICKS(1000));
  Relay_SSR_SetRelay(3, ON); vTaskDelay(pdMS_TO_TICKS(1000));
  Relay_SSR_SetRelay(4, ON);
}

static void compute_offset_over_ms(uint32_t ms)
{
  const uint32_t end = (uint32_t)(xTaskGetTickCount()) + pdMS_TO_TICKS(ms);
  double s = 0.0; uint32_t n = 0;
  while ((uint32_t)xTaskGetTickCount() < end) {
    int32_t raw = LoadCell_GetRaw();
    s += (double)raw;
    n++;
    vTaskDelay(pdMS_TO_TICKS(5));
  }
  if (n > 0) g_ADC_zero = (float)(s / (double)n);
}

static void compute_KT_over_ms(uint32_t ms, float known_torque_nm)
{
  const uint32_t end = (uint32_t)(xTaskGetTickCount()) + pdMS_TO_TICKS(ms);
  double vmin = 1e300, vmax = -1e300;
  while ((uint32_t)xTaskGetTickCount() < end) {
    int32_t raw = LoadCell_GetRaw();
    double corr = (double)raw - (double)g_ADC_zero;
    if (corr < vmin) vmin = corr;
    if (corr > vmax) vmax = corr;
    vTaskDelay(pdMS_TO_TICKS(5));
  }
  double amp = (vmax - vmin) / 2.0;
  if (amp > 1e-6) {
    g_K_T = (float)(known_torque_nm / amp);
  }
}

/* Lightweight JSON helpers (simple extractors) */
static int find_key_str(const char *json, const char *key, char *out, size_t out_sz)
{
  char pattern[32];
  snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);
  const char *p = strstr(json, pattern);
  if (!p) return 0;
  p += strlen(pattern);
  const char *q = strchr(p, '"');
  if (!q || (size_t)(q - p) >= out_sz) return 0;
  memcpy(out, p, (size_t)(q - p));
  out[q - p] = '\0';
  return 1;
}

static int find_key_num(const char *json, const char *key, double *val)
{
  char pattern[32];
  snprintf(pattern, sizeof(pattern), "\"%s\":", key);
  const char *p = strstr(json, pattern);
  if (!p) return 0;
  p += strlen(pattern);
  *val = atof(p);
  return 1;
}

static void handle_line(const char *line)
{
  ESP_LOGI("UART", "Received %s", line);
  char cmd[32];
  if (!find_key_str(line, "cmd", cmd, sizeof(cmd))) { reply_err("bad_json"); return; }
  ESP_LOGI("UART", "cmd: %s", cmd);
  if (strcmp(cmd, "rtd_calib") == 0) {
    double dev_d = 0, known_d = 0;
    if (find_key_num(line, "dev", &dev_d) && find_key_num(line, "known", &known_d)) {
      RTD_Temp_CalibrateAndSave((uint8_t)dev_d, (float)known_d);
      reply_ok("rtd_calib");
    } else {
      reply_err("bad_args");
    }
        return;
    }
    
  if (strcmp(cmd, "set_temp") == 0) {
    double v = 0;
    if (find_key_num(line, "value", &v)) {
      RTD_Temp_SetTempSetPoint((float)v);
      reply_ok("set_temp");
    } else {
      reply_err("bad_args");
    }
        return;
    }

  if (strcmp(cmd, "set_temp_rtd") == 0) {
    double dev = 0, temp = 0;
    if (find_key_num(line, "dev", &dev) && find_key_num(line, "temp", &temp)) {
      int device = (int)dev;
      
      // Validate device number (1-2)
      if (device >= 1 && device <= 2) {
        RTD_Temp_SetTempSetPointIndividual((uint8_t)device, (float)temp);
        UART_Printf("{\"ok\":true,\"cmd\":\"set_temp_rtd\",\"dev\":%d,\"temp\":%.2f}\r\n", device, temp);
      } else {
        reply_err("invalid_device");
      }
    } else {
      reply_err("bad_args");
    }
    return;
  }
    
  if (strcmp(cmd, "get_temp") == 0) {
    float t1 = RTD_Temp_GetTemperature(1);
    float t2 = RTD_Temp_GetTemperature(2);
    UART_Printf("{\"t1\":%.2f,\"t2\":%.2f}\r\n", t1, t2);
        return;
    }
    
  if (strcmp(cmd, "set_mode") == 0) {
    char val[16];
    ESP_LOGI("UART", "val: %s", val);
    if (find_key_str(line, "value", val, sizeof(val))) {
      if (strcmp(val, "powerup") == 0) { mode = 0; relays_all_off(); reply_ok("set_mode"); return; }
      if (strcmp(val, "idle") == 0)    { mode = 0; relays_all_off(); reply_ok("set_mode"); return; }
      if (strcmp(val, "run") == 0)     { mode = 1; triggerFlg = 1; g_run_start_ms = (uint32_t)(xTaskGetTickCount()); reply_ok("set_mode"); return; }
      if (strcmp(val, "stop") == 0)    { mode = 0; relays_all_off(); UART_Printf("{\"mode\":\"run\",\"status\":\"finished\"}\r\n"); return; }
      if (strcmp(val, "calib") == 0)   { mode = 3; reply_ok("set_mode"); return; }
    }
    reply_err("bad_args");
        return;
    }
    
  if (strcmp(cmd, "set_run_time") == 0) {
    double sec = 0;
    if (find_key_num(line, "seconds", &sec) && sec > 0) {
      g_run_time_s = (uint32_t)sec;
      UART_Printf("set_run_time: %u seconds\r\n", (uint32_t)sec);
      offTime = (uint32_t)sec; // Also update legacy variable for compatibility
      reply_ok("set_run_time");
    } else {
      reply_err("bad_args");
    }
        return;
    }
    
  if (strcmp(cmd, "calibrate_mdr") == 0) {
    double w=0, lever=0;
    if (find_key_num(line, "weight", &w) && find_key_num(line, "lever", &lever) && w>0 && lever>0) {
      compute_offset_over_ms(60000);
      relays_sequence_on();
      float T_cal = (float)(w * 9.81 * lever);
      compute_KT_over_ms(60000, T_cal);
      relays_all_off();
      
      // Save MDR calibration to EEPROM
      if (EEPROM_SaveMDRCalibration(g_ADC_zero, g_K_T) == ESP_OK) {
        UART_Printf("Saved MDR calibration to EEPROM\r\n");
      } else {
        UART_Printf("Failed to save MDR calibration to EEPROM\r\n");
      }
      
      UART_Printf("{\"ok\":true,\"cmd\":\"calibrate_mdr\",\"ADC_zero\":%.3f,\"K_T\":%.9f}\r\n", g_ADC_zero, g_K_T);
    } else {
      reply_err("bad_args");
    }
        return;
    }

  if (strcmp(cmd, "offset_mdr") == 0) {
    double ms = 5000;
    (void)find_key_num(line, "ms", &ms);
    relays_all_off();
    compute_offset_over_ms((uint32_t)ms);
    
    // Save MDR offset to EEPROM
    if (EEPROM_SaveMDRCalibration(g_ADC_zero, g_K_T) == ESP_OK) {
      UART_Printf("Saved MDR offset to EEPROM\r\n");
    } else {
      UART_Printf("Failed to save MDR offset to EEPROM\r\n");
    }
    
    UART_Printf("{\"ok\":true,\"cmd\":\"offset_mdr\",\"ADC_zero\":%.3f}\r\n", g_ADC_zero);
    return;
  }

  if (strcmp(cmd, "tare_idle_amp") == 0) {
    // This command will be handled by setting a flag that the ModeTask will read
    // We'll use a global variable to communicate between CommTask and ModeTask
    extern double g_idle_amp_tare_request;
    g_idle_amp_tare_request = 1.0; // Set flag to request tare
    reply_ok("tare_idle_amp");
    return;
  }

  if (strcmp(cmd, "set_relay") == 0) {
    double relay_num = 0, state = 0;
    if (find_key_num(line, "relay", &relay_num) && find_key_num(line, "state", &state)) {
      int relay = (int)relay_num;
      int relay_state = (int)state;
      
      // Validate relay number (1-4) and state (0-1)
      if (relay >= 1 && relay <= 4 && (relay_state == 0 || relay_state == 1)) {
        Relay_SSR_SetRelay((uint8_t)relay, (uint8_t)relay_state);
        UART_Printf("{\"ok\":true,\"cmd\":\"set_relay\",\"relay\":%d,\"state\":%d}\r\n", relay, relay_state);
      } else {
        reply_err("invalid_relay_or_state");
      }
    } else {
      reply_err("bad_args");
    }
    return;
  }

  if (strcmp(cmd, "get_relays") == 0) {
    uint8_t relay1 = Relay_SSR_GetRelayState(1);
    uint8_t relay2 = Relay_SSR_GetRelayState(2);
    uint8_t relay3 = Relay_SSR_GetRelayState(3);
    uint8_t relay4 = Relay_SSR_GetRelayState(4);
    UART_Printf("{\"ok\":true,\"cmd\":\"get_relays\",\"relay1\":%d,\"relay2\":%d,\"relay3\":%d,\"relay4\":%d}\r\n", 
                relay1, relay2, relay3, relay4);
    return;
  }
    
  reply_err("unknown_cmd");
}

static void uart_poll_and_process_lines(void)
{
  uint8_t rxbuf[COMM_RXBUF_SIZE];
  static char line[COMM_LINEBUF_SIZE];
  static size_t line_len;

  int len = uart_read_bytes(COMM_UART, rxbuf, sizeof(rxbuf), pdMS_TO_TICKS(20));
  if (len <= 0) return;
  ESP_LOGI("UART", "Received %d bytes", len);
  ESP_LOGI("UART", "Received %s", rxbuf);
  for (int i = 0; i < len; i++) {
    char ch = (char)rxbuf[i];
    if (ch == '\r') continue;
    if (ch == '\n') {
      line[line_len] = '\0';
      if (line_len > 0) handle_line(line);
      line_len = 0;
    } else if (line_len < sizeof(line) - 1) {
      line[line_len++] = ch;
    }
  }
}

static void CommTask_Function(void *argument)
{
  for(;;) {
    uart_poll_and_process_lines();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

static void ModeTask_Function(void *argument)
{
  int last_mode = -1;
  uint32_t last_broadcast = 0;
  uint32_t last_print = 0;
  // Cycle amplitude tracking (per MDR reference)
  const float cycle_freq_hz = 1.66f; // default
  const uint32_t cycle_period_ms = (uint32_t)(1000.0f / cycle_freq_hz + 0.5f); // ≈602 ms
  const TickType_t cycle_period_ticks = pdMS_TO_TICKS(cycle_period_ms);
  uint32_t cycle_start_ms = 0;
  double cycle_tmin = 1e300, cycle_tmax = -1e300;
  int run_started = 0;
  
  // Moving average filter for amplitude (5-window)
  double amp_filter_buffer[5] = {0.0, 0.0, 0.0, 0.0, 0.0};
  int amp_filter_index = 0;
  int amp_filter_count = 0;
  
  // Amplitude tracking for idle mode
  const uint32_t idle_cycle_period_ms = (uint32_t)(1000.0f / cycle_freq_hz + 0.5f); // Same as run mode: ≈602 ms
  const TickType_t idle_cycle_period_ticks = pdMS_TO_TICKS(idle_cycle_period_ms);
  uint32_t idle_cycle_start_ms = 0;
  double idle_tmin = 1e300, idle_tmax = -1e300;
  
  // Moving average filter for idle mode amplitude (2-window)
  double idle_amp_filter_buffer[2] = {0.0, 0.0};
  int idle_amp_filter_index = 0;
  int idle_amp_filter_count = 0;
  
  // Idle mode amplitude offset/tare value
  double idle_amp_offset = 0.0;
  for (;;) {
    int current_mode = mode; // from config.c

    // On mode transition
    if (current_mode != last_mode) {
      if (current_mode == 0) { // idle/stop
        relays_all_off();
        // Reset idle mode amplitude tracking
        idle_cycle_start_ms = (uint32_t)xTaskGetTickCount();
        idle_tmin = 1e300; idle_tmax = -1e300;
        idle_amp_filter_buffer[0] = 0.0; idle_amp_filter_buffer[1] = 0.0;
        idle_amp_filter_index = 0; idle_amp_filter_count = 0;
        // Note: idle_amp_offset is preserved across mode transitions
      } else if (current_mode == 1) { // run
        // Prepare run: relays sequencing will be done just before starting timer
        offTime = g_run_time_s; // keep legacy var in sync (seconds)
        run_started = 0;
        cycle_tmin = 1e300; cycle_tmax = -1e300;
        // Reset moving average filter
        amp_filter_buffer[0] = 0.0; amp_filter_buffer[1] = 0.0; amp_filter_buffer[2] = 0.0; amp_filter_buffer[3] = 0.0; amp_filter_buffer[4] = 0.0;
        amp_filter_index = 0; amp_filter_count = 0;
      } else if (current_mode == 3) { // calibration mode (idle here)
        relays_all_off();
        run_started = 0;
      }
      last_mode = current_mode;
    }

    // State behaviors
    if (current_mode == 0) { // idle mode
      // Check for idle amplitude tare request
      if (g_idle_amp_tare_request > 0.0) {
        // Calculate current filtered amplitude
        double current_amp = 0.0;
        if (idle_amp_filter_count > 0) {
          double sum = 0.0;
          for (int i = 0; i < idle_amp_filter_count; i++) {
            sum += idle_amp_filter_buffer[i];
          }
          current_amp = sum / (double)idle_amp_filter_count;
        }
        
        // Set offset to current amplitude value
        idle_amp_offset = current_amp;
        g_idle_amp_tare_request = 0.0; // Clear request flag
        
        UART_Printf("{\"ok\":true,\"cmd\":\"tare_idle_amp\",\"offset\":%.6f}\r\n", (float)idle_amp_offset);
      }
      
      // Broadcast torque at ~100 Hz in idle mode
      if ((uint32_t)(xTaskGetTickCount()) - last_broadcast >= pdMS_TO_TICKS(10)) {
        last_broadcast = (uint32_t)(xTaskGetTickCount());
        int32_t raw = LoadCell_GetRaw();
        float torque = (g_K_T > 0.0f) ? (float)((double)raw - (double)g_ADC_zero) * g_K_T : 0.0f;
        
        // Update idle mode amplitude tracking
        double t = (double)torque;
        if (t < idle_tmin) idle_tmin = t;
        if (t > idle_tmax) idle_tmax = t;
      }
      
      // Print idle mode data at 10Hz
      if ((uint32_t)(xTaskGetTickCount()) - last_print >= pdMS_TO_TICKS(100)) {
        last_print = (uint32_t)(xTaskGetTickCount());
        UART_Printf("{\"mode\":\"idle\",\"raw\":%ld,\"torque\":%.6f}\r\n", (long)LoadCell_GetRaw(), (g_K_T > 0.0f) ? (float)((double)LoadCell_GetRaw() - (double)g_ADC_zero) * g_K_T : 0.0f);
      }
      
      // When idle cycle elapses, compute and print amplitude
      TickType_t now_ticks_idle = xTaskGetTickCount();
      if ((now_ticks_idle - (TickType_t)idle_cycle_start_ms) >= idle_cycle_period_ticks) {
        double amp = (idle_tmax - idle_tmin) / 2.0;
        
        // Apply 2-window moving average filter
        idle_amp_filter_buffer[idle_amp_filter_index] = amp;
        idle_amp_filter_index = (idle_amp_filter_index + 1) % 2;
        if (idle_amp_filter_count < 2) idle_amp_filter_count++;
        
        // Calculate filtered amplitude
        double filtered_amp = 0.0;
        if (idle_amp_filter_count > 0) {
          double sum = 0.0;
          for (int i = 0; i < idle_amp_filter_count; i++) {
            sum += idle_amp_filter_buffer[i];
          }
          filtered_amp = sum / (double)idle_amp_filter_count;
        }
        
        // Apply offset to filtered amplitude
        double offset_amp = filtered_amp - idle_amp_offset;
        
        // Print filtered amplitude with offset applied
        if(filtered_amp > 0.0) {
           UART_Printf("{\"mode\":\"idle\",\"cycle_amp\":%.6f,\"cycle_amp_filtered\":%.6f,\"cycle_amp_offset\":%.6f,\"min\":%.6f,\"max\":%.6f}\r\n", 
                   (float)amp, (float)offset_amp, (float)idle_amp_offset, (float)idle_tmin, (float)idle_tmax);
        }
        else {
          UART_Printf("{\"mode\":\"idle\",\"cycle_amp\":1.0,\"cycle_amp_filtered\":1.0,\"cycle_amp_offset\":%.6f,\"min\":%.6f,\"max\":%.6f}\r\n", (float)idle_amp_offset, (float)idle_tmin, (float)idle_tmax);
        }
        
        // Advance to next cycle window
        idle_cycle_start_ms = (uint32_t)now_ticks_idle;
        idle_tmin = 1e300; idle_tmax = -1e300;
      }
    } else if (current_mode == 1) { // run mode
      // If not started, sequence relays then start the timer
      if (!run_started) {
        relays_sequence_on();
        g_run_start_ms = (uint32_t)xTaskGetTickCount();
        cycle_start_ms = g_run_start_ms;
        run_started = 1;
      }

      // Update remaining time (convert ticks to ms) only after start
      TickType_t now_ticks = xTaskGetTickCount();
      TickType_t elapsed_ticks = now_ticks - (TickType_t)g_run_start_ms;
      uint32_t elapsed_ms = (uint32_t)elapsed_ticks * (uint32_t)portTICK_PERIOD_MS;
      uint32_t elapsed_s = elapsed_ms / 1000U;
      if (elapsed_s <= g_run_time_s) {
        offTime = g_run_time_s - elapsed_s;
      }

      // Broadcast torque at ~100 Hz
      if ((uint32_t)(xTaskGetTickCount()) - last_broadcast >= pdMS_TO_TICKS(10)) {
        last_broadcast = (uint32_t)(xTaskGetTickCount());
        int32_t raw = LoadCell_GetRaw();
        float torque = (g_K_T > 0.0f) ? (float)((double)raw - (double)g_ADC_zero) * g_K_T : 0.0f;
        // Update cycle min/max for amplitude
        double t = (double)torque;
        if (t < cycle_tmin) cycle_tmin = t;
        if (t > cycle_tmax) cycle_tmax = t;
      }
      
      // Print run mode data at 10Hz
      if ((uint32_t)(xTaskGetTickCount()) - last_print >= pdMS_TO_TICKS(10)) {
        last_print = (uint32_t)(xTaskGetTickCount());
        UART_Printf("{\"mode\":\"run\",\"elapsed_s\":%u,\"raw\":%ld,\"torque\":%.6f}\r\n", (unsigned)elapsed_s, (long)LoadCell_GetRaw(), (g_K_T > 0.0f) ? (float)((double)LoadCell_GetRaw() - (double)g_ADC_zero) * g_K_T : 0.0f);
      }

      // When a cycle elapses (tick-based), compute and print amplitude
      TickType_t now_ticks2 = xTaskGetTickCount();
      if ((now_ticks2 - (TickType_t)cycle_start_ms) >= cycle_period_ticks) {
        double amp = (cycle_tmax - cycle_tmin) / 2.0;
        
        // Apply 5-window moving average filter
        amp_filter_buffer[amp_filter_index] = amp;
        amp_filter_index = (amp_filter_index + 1) % 5;
        if (amp_filter_count < 5) amp_filter_count++;
        
        // Calculate filtered amplitude
        double filtered_amp = 0.0;
        if (amp_filter_count > 0) {
          double sum = 0.0;
          for (int i = 0; i < amp_filter_count; i++) {
            sum += amp_filter_buffer[i];
          }
          filtered_amp = sum / (double)amp_filter_count;
        }
        
        // Print filtered amplitude
        if(filtered_amp > 0.0) {
           UART_Printf("{\"mode\":\"run\",\"cycle_amp\":%.6f,\"cycle_amp_filtered\":%.6f,\"min\":%.6f,\"max\":%.6f}\r\n", 
                   (float)filtered_amp, (float)filtered_amp, (float)cycle_tmin, (float)cycle_tmax);
        }
        else {
          UART_Printf("{\"mode\":\"run\",\"cycle_amp\":1.0,\"cycle_amp_filtered\":1.0,\"min\":%.6f,\"max\":%.6f}\r\n", (float)cycle_tmin, (float)cycle_tmax);
        }
        
        // Advance to next cycle window
        cycle_start_ms = (uint32_t)now_ticks2;
        cycle_tmin = 1e300; cycle_tmax = -1e300;
      }

      // Stop condition
      if (elapsed_s >= g_run_time_s) {
        UART_Printf("{\"mode\":\"run\",\"status\":\"finished\"}\r\n");
        mode = 0; // stop -> idle
        relays_all_off();
        run_started = 0;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
