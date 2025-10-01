# Moving Die Rheometer (MDR) Torque Measurement - Firmware Reference

This document summarizes the calibration and runtime torque measurement process for implementing MDR logic in firmware.

---

## 1. Constants and Definitions

- **Oscillation Frequency**: `1.66 Hz` (Period ≈ 0.602 s)
- **Lever Arm (r)**: Distance from die axis to loadcell force point (meters)
- **g**: Gravitational acceleration = 9.81 m/s²

---

## 2. Offset Calibration (Zeroing)

**Purpose**: Remove baseline drift and electronics offset.

### Procedure
1. Run system at **rest (no oscillation, no sample)**.
2. Record N samples of loadcell ADC.
3. Compute average:

```c
ADC_zero = (1/N) * sum(ADC[i]);
```

4. Store `ADC_zero` for use in runtime correction.

---

## 3. Scale Calibration (Gain Factor)

**Purpose**: Map ADC readings to actual torque.

### Known Torque Setup
- Apply known torque using a weight `W` at lever arm `r`:

```c
T_cal = W * g * r; // in N·m
```

### Procedure
1. Enable oscillation at 1.66 Hz.
2. Record loadcell samples for multiple oscillation cycles.
3. Compute raw amplitude in counts:

```c
ADC_pp  = ADC_max - ADC_min;   // peak-to-peak
ADC_amp = ADC_pp / 2.0;        // amplitude
```

4. Compute calibration constant:

```c
K_T = T_cal / ADC_amp;   // N·m per ADC count
```

---

## 4. Runtime Torque Calculation

### Instantaneous Torque
At each sample:

```c
ADC_corr = ADC[n] - ADC_zero;
T[n]     = ADC_corr * K_T; // Torque in N·m
```

### Cycle Amplitude (per oscillation cycle)
Over one cycle (≈0.602 s):

```c
T_amp = (T_max - T_min) / 2.0; // N·m
```

### RMS (optional)
For sinusoidal torque:

```c
T_rms = sqrt((1/N) * sum(T[i]^2));
```

---

## 5. Firmware Flow

```c
// --- Initialization ---
Calibrate_Offset();    // get ADC_zero
Calibrate_Scale();     // compute K_T

// --- Runtime Loop ---
while (1) {
    Read_ADC();
    Torque = (ADC - ADC_zero) * K_T;
    Update_MaxMin(Torque);   // per cycle
    if (Cycle_Complete()) {
        T_amp = (T_max - T_min) / 2.0;
        Output(T_amp);
        Reset_MaxMin();
    }
}
```

---

## 6. Notes

- Offset calibration must be done with **no oscillation**.
- Scale calibration must be done with **oscillation ON and known torque**.
- Repeat offset calibration periodically to remove drift.
- Outlier rejection (spikes) recommended using median filter or thresholding.
