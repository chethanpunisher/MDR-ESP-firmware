# MDR Calibration Commands and Process

## Overview
This document describes the calibration commands and process for the MDR (Moving Die Rheometer) firmware running on ESP32.

## Command Structure
All commands are sent via USB-UART as JSON strings with `\r\n` termination.

### General Command Format
```json
{"cmd":"command_name","param1":"value1","param2":value2}
```

## Calibration Commands

### 1. RTD Temperature Calibration
**Purpose**: Calibrate RTD temperature sensors with known reference temperature.

**Command**:
```json
{"cmd":"rtd_calib","dev":1,"known":25.0}
```

**Parameters**:
- `dev`: RTD device number (1 or 2)
- `known`: Known reference temperature in Celsius

**Response**:
```json
{"ok":true,"cmd":"rtd_calib"}
```

**Error Response**:
```json
{"ok":false,"err":"bad_args"}
```

### 2. MDR Offset Calibration
**Purpose**: Calculate ADC zero offset when no load is applied.

**Command**:
```json
{"cmd":"offset_mdr","ms":5000}
```

**Parameters**:
- `ms`: Duration in milliseconds for offset calculation (default: 5000ms)

**Response**:
```json
{"ok":true,"cmd":"offset_mdr","ADC_zero":12345.678}
```

**Process**:
1. Turns off all relays
2. Samples load cell for specified duration
3. Calculates average ADC value as zero offset
4. Stores result in `g_ADC_zero`

### 3. MDR Scale Calibration
**Purpose**: Calculate torque scale factor using known weight and lever arm.

**Command**:
```json
{"cmd":"calibrate_mdr","weight":1.0,"lever":0.1}
```

**Parameters**:
- `weight`: Known weight in kg
- `lever`: Lever arm length in meters

**Response**:
```json
{"ok":true,"cmd":"calibrate_mdr","ADC_zero":12345.678,"K_T":0.000123456}
```

**Process**:
1. Calculates offset (5 seconds with relays off)
2. Sequences relays ON (relay1→relay2→relay3→relay4 with 1s delays)
3. Samples load cell for 5 seconds
4. Calculates scale factor: `K_T = (weight * 9.81 * lever) / amplitude`
5. Turns off all relays

## Device Mode Commands

### Set Device Mode
**Command**:
```json
{"cmd":"set_mode","value":"idle"}
```

**Valid Values**:
- `"powerup"`: Powerup mode (turns off relays, transitions to idle)
- `"idle"`: Idle mode (turns off relays, broadcasts torque)
- `"run"`: Run mode (sequences relays, runs for set duration)
- `"stop"`: Stop mode (turns off relays, transitions to idle)
- `"calib"`: Calibration mode (turns off relays)

**Response**:
```json
{"ok":true,"cmd":"set_mode"}
```

### Set Run Duration
**Command**:
```json
{"cmd":"set_run_time","seconds":60}
```

**Parameters**:
- `seconds`: Run duration in seconds

**Response**:
```json
{"ok":true,"cmd":"set_run_time"}
```

## Data Monitoring Commands

### Get Temperature
**Command**:
```json
{"cmd":"get_temp"}
```

**Response**:
```json
{"t1":25.5,"t2":26.1}
```

### Get Device State
**Command**:
```json
{"cmd":"get_state"}
```

**Response**:
```json
{"mode":"idle","elapsed_s":0,"remaining_s":60,"ADC_zero":12345.678,"K_T":0.000123456}
```

## Continuous Data Streams

### Idle Mode Torque Broadcast
**Format**: Every ~100ms
```json
{"mode":"idle","raw":123456,"torque":0.123456}
```

### Run Mode Torque Broadcast
**Format**: Every ~100ms
```json
{"mode":"run","elapsed_s":15,"raw":123456,"torque":0.123456}
```

### Run Mode Cycle Amplitude
**Format**: Every ~602ms (1.66 Hz cycle)
```json
{"mode":"run","cycle_amp":0.045678,"min":0.100000,"max":0.191356}
```

### Run Completion
**Format**: When run timer expires
```json
{"mode":"run","status":"finished"}
```

### Temperature Broadcast (Load Cell Service)
**Format**: Every ~16ms
```json
{"temp1":25.5,"temp2":26.1}
```

## Complete Calibration Procedure

### Step 1: RTD Temperature Calibration
1. Ensure RTD sensors are at known temperature (e.g., room temperature)
2. Send: `{"cmd":"rtd_calib","dev":1,"known":25.0}`
3. Send: `{"cmd":"rtd_calib","dev":2,"known":25.0}`
4. Verify responses: `{"ok":true,"cmd":"rtd_calib"}`

### Step 2: MDR Offset Calibration
1. Ensure no load on load cell
2. Send: `{"cmd":"offset_mdr","ms":5000}`
3. Verify response contains `ADC_zero` value
4. Store `ADC_zero` for later use

### Step 3: MDR Scale Calibration
1. Apply known weight at known lever arm distance
2. Send: `{"cmd":"calibrate_mdr","weight":1.0,"lever":0.1}`
3. Verify response contains both `ADC_zero` and `K_T` values
4. Store `K_T` (torque scale factor) for later use

### Step 4: Verification
1. Set run time: `{"cmd":"set_run_time","seconds":10}`
2. Start run: `{"cmd":"set_mode","value":"run"}`
3. Monitor torque values during run
4. Verify cycle amplitude values are reasonable
5. Wait for completion: `{"mode":"run","status":"finished"}`

## Error Handling

### Common Error Responses
```json
{"ok":false,"err":"bad_json"}     // Invalid JSON format
{"ok":false,"err":"bad_args"}     // Missing or invalid parameters
{"ok":false,"err":"unknown_cmd"}  // Unrecognized command
```

### QT Software Recommendations

1. **Command Queue**: Implement a command queue to avoid overwhelming the UART
2. **Timeout Handling**: Set timeouts for command responses (5-10 seconds)
3. **Retry Logic**: Retry failed commands up to 3 times
4. **Status Monitoring**: Continuously monitor data streams for device state
5. **Calibration Validation**: Verify calibration values are within expected ranges
6. **User Feedback**: Display calibration progress and results to user
7. **Error Recovery**: Handle communication errors gracefully

## Example QT Implementation Flow

```cpp
// 1. RTD Calibration
sendCommand("{\"cmd\":\"rtd_calib\",\"dev\":1,\"known\":25.0}");
waitForResponse("rtd_calib");

// 2. Offset Calibration
sendCommand("{\"cmd\":\"offset_mdr\",\"ms\":5000}");
waitForResponse("offset_mdr");

// 3. Scale Calibration
sendCommand("{\"cmd\":\"calibrate_mdr\",\"weight\":1.0,\"lever\":0.1}");
waitForResponse("calibrate_mdr");

// 4. Start Test Run
sendCommand("{\"cmd\":\"set_run_time\",\"seconds\":60}");
sendCommand("{\"cmd\":\"set_mode\",\"value\":\"run\"}");

// 5. Monitor Data Streams
// Parse continuous JSON data for torque, cycle amplitude, etc.
```

## Notes

- All floating-point values are sent with 6 decimal places
- Commands are case-sensitive
- JSON must be properly formatted
- Responses include `\r\n` termination
- Device automatically transitions between modes
- Calibration values persist until power cycle
- Run mode automatically stops after set duration
