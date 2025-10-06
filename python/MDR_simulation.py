import argparse
import csv
import math
import os
import sys
from datetime import datetime, timedelta

try:
    import matplotlib.pyplot as plt  # optional
except Exception:
    plt = None


def read_csv_series(path):
    ts = []
    y = []
    with open(path, newline="", encoding="utf-8") as f:
        r = csv.reader(f)
        header = next(r, None)
        # Expect columns: timestamp_iso, raw_value
        for row in r:
            if not row or len(row) < 2:
                continue
            try:
                t = datetime.fromisoformat(row[0])
                v = float(row[1])
            except Exception:
                continue
            ts.append(t)
            y.append(v)
    return ts, y


def mean(values):
    if not values:
        return 0.0
    return sum(values) / float(len(values))


def pp_amplitude(values):
    if not values:
        return 0.0
    return (max(values) - min(values)) / 2.0


def cycle_amplitudes(ts, values, freq_hz):
    if not ts or not values or len(ts) != len(values) or freq_hz <= 0:
        return []
    period = 1.0 / freq_hz
    out = []
    if len(ts) < 2:
        return out
    start_t = ts[0]
    bucket = []
    for t, v in zip(ts, values):
        while (t - start_t).total_seconds() >= period:
            if bucket:
                out.append((start_t, pp_amplitude(bucket)))
            start_t += timedelta(seconds=period)
            bucket = []
        bucket.append(v)
    if bucket:
        out.append((start_t, pp_amplitude(bucket)))
    return out

def cycle_rms(ts, values, freq_hz):
    if not ts or not values or len(ts) != len(values) or freq_hz <= 0:
        return []
    period = 1.0 / freq_hz
    rms_list = []
    starts = []
    if len(ts) < 2:
        return []
    start_t = ts[0]
    bucket = []
    for t, v in zip(ts, values):
        while (t - start_t).total_seconds() >= period:
            if bucket:
                s2 = sum([b*b for b in bucket]) / float(len(bucket))
                rms_list.append(math.sqrt(s2))
                starts.append(start_t)
            start_t += timedelta(seconds=period)
            bucket = []
        bucket.append(v)
    if bucket:
        s2 = sum([b*b for b in bucket]) / float(len(bucket))
        rms_list.append(math.sqrt(s2))
        starts.append(start_t)
    return list(zip(starts, rms_list))


def main():
    p = argparse.ArgumentParser(description="Simulate MDR torque from loadcell logs")
    p.add_argument("--offset", default="python/offset_loadcell_log.csv", help="Offset CSV path")
    p.add_argument("--cal", default="python/calibration_loadcell_log.csv", help="Calibration CSV path")
    p.add_argument("--sample", default="python/actuall_sample_loadcell_log.csv", help="Actual sample CSV path")
    p.add_argument("--freq", type=float, default=1.66, help="Oscillation frequency Hz (default 1.66)")
    g = p.add_mutually_exclusive_group(required=True)
    g.add_argument("--torque", type=float, help="Known torque at calibration in N·m")
    g.add_argument("--weight", type=float, help="Known weight in kg for calibration (with --lever)")
    p.add_argument("--lever", type=float, default=None, help="Lever arm in meters (used with --weight)")
    p.add_argument("--out", default="python/torque_output.csv", help="Output CSV for torque series")
    p.add_argument("--cycles_out", default="python/torque_cycles.csv", help="Optional CSV for per-cycle metrics")
    p.add_argument("--plot", default=None, help="Save plot to file (PNG). If omitted, shows interactive plot if available.")
    args = p.parse_args()

    # Read series
    ts_off, y_off = read_csv_series(args.offset)
    ts_cal, y_cal = read_csv_series(args.cal)
    ts_sam, y_sam = read_csv_series(args.sample)
    if not y_off or not y_cal or not y_sam:
        print("One or more CSV files are empty or invalid.", file=sys.stderr)
        sys.exit(1)

    # Offset calibration
    ADC_zero = mean(y_off)

    # Calibration amplitude (after offset removal)
    cal_corr = [v - ADC_zero for v in y_cal]

    # Compute amplitude; prefer per-cycle average if timestamps present
    if args.freq and ts_cal:
        amps = cycle_amplitudes(ts_cal, cal_corr, args.freq)  # list of (start_time, amp)
        ADC_amp = mean([amp for _, amp in amps if amp > 0]) if amps else pp_amplitude(cal_corr)
    else:
        ADC_amp = pp_amplitude(cal_corr)
    if ADC_amp <= 0:
        print("Calibration amplitude is zero; check calibration CSV.", file=sys.stderr)
        sys.exit(1)

    # Known torque
    if args.torque is not None:
        T_cal = float(args.torque)
    else:
        if args.lever is None:
            print("--lever is required with --weight", file=sys.stderr)
            sys.exit(1)
        T_cal = float(args.weight) * 9.81 * float(args.lever)

    K_T = T_cal / ADC_amp  # N·m per ADC count

    # Actual sample torque
    sam_corr = [v - ADC_zero for v in y_sam]
    torque = [v * K_T for v in sam_corr]

    # Write output CSV
    os.makedirs(os.path.dirname(args.out), exist_ok=True) if os.path.dirname(args.out) else None
    with open(args.out, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["timestamp_iso", "raw_value", "adc_corr", "torque_nm"])  # header
        for t, rv, ac, tq in zip(ts_sam, y_sam, sam_corr, torque):
            w.writerow([t.isoformat(), f"{rv:.0f}", f"{ac:.3f}", f"{tq:.6f}"])

        # Also append per-cycle metrics of torque: amplitude and RMS
        rms_cycles = cycle_rms(ts_sam, torque, args.freq)  # list of (start, rms)
        amp_cycles = cycle_amplitudes(ts_sam, torque, args.freq)  # list of (start, amp)
        if rms_cycles or amp_cycles:
            w.writerow([])
            w.writerow(["cycle_start_iso", "torque_amp_nm", "torque_rms_nm"])  # header for cycle metrics
            n = min(len(rms_cycles), len(amp_cycles)) if (rms_cycles and amp_cycles) else (len(rms_cycles) or len(amp_cycles))
            for i in range(n):
                if rms_cycles and amp_cycles:
                    ct = rms_cycles[i][0]
                    rms = rms_cycles[i][1]
                    amp = amp_cycles[i][1]
                elif rms_cycles:
                    ct = rms_cycles[i][0]
                    rms = rms_cycles[i][1]
                    amp = ''
                else:
                    ct = amp_cycles[i][0]
                    amp = amp_cycles[i][1]
                    rms = ''
                w.writerow([ct.isoformat(), f"{amp:.6f}" if amp != '' else '', f"{rms:.6f}" if rms != '' else ''])

        # Write separate cycles CSV if requested
        if args.cycles_out:
            if rms_cycles or amp_cycles:
                os.makedirs(os.path.dirname(args.cycles_out), exist_ok=True) if os.path.dirname(args.cycles_out) else None
                with open(args.cycles_out, "w", newline="", encoding="utf-8") as fc:
                    wc = csv.writer(fc)
                    wc.writerow(["cycle_start_iso", "torque_amp_nm", "torque_rms_nm"])
                    # merge by index as above
                    n = min(len(rms_cycles), len(amp_cycles)) if (rms_cycles and amp_cycles) else (len(rms_cycles) or len(amp_cycles))
                    for i in range(n):
                        if rms_cycles and amp_cycles:
                            ct = rms_cycles[i][0]
                            rms = rms_cycles[i][1]
                            amp = amp_cycles[i][1]
                        elif rms_cycles:
                            ct = rms_cycles[i][0]
                            rms = rms_cycles[i][1]
                            amp = ''
                        else:
                            ct = amp_cycles[i][0]
                            amp = amp_cycles[i][1]
                            rms = ''
                        wc.writerow([ct.isoformat(), f"{amp:.6f}" if amp != '' else '', f"{rms:.6f}" if rms != '' else ''])
                print(f"Wrote per-cycle metrics -> {args.cycles_out}")
            else:
                print("Warning: No cycles computed. Check timestamps and --freq.")

    print(f"ADC_zero (offset): {ADC_zero:.3f}")
    print(f"ADC_amp (calibration): {ADC_amp:.3f}")
    print(f"K_T (Nm/count): {K_T:.9f}")
    print(f"Wrote torque series -> {args.out}")

    # Plot
    if plt is not None:
        plt.figure(figsize=(10, 6))
        plt.subplot(2, 1, 1)
        plt.plot(ts_sam, y_sam, label="raw counts", alpha=0.7)
        plt.plot(ts_sam, sam_corr, label="adc_corr", alpha=0.7)
        plt.grid(True, alpha=0.3)
        plt.legend()
        plt.title("Loadcell Raw and Corrected")

        plt.subplot(2, 1, 2)
        plt.plot(ts_sam, torque, label="torque [N·m]", color="tab:red")
        rms_cycles = cycle_rms(ts_sam, torque, args.freq)
        amp_cycles = cycle_amplitudes(ts_sam, torque, args.freq)
        if rms_cycles:
            rms_t = [t for t,_ in rms_cycles]
            rms_v = [v for _,v in rms_cycles]
            plt.plot(rms_t, rms_v, 'o-', label="torque RMS [N·m]", color="tab:green", alpha=0.8)
        if amp_cycles:
            amp_t = [t for t,_ in amp_cycles]
            amp_v = [v for _,v in amp_cycles]
            plt.plot(amp_t, amp_v, 's-', label="torque Amp [N·m]", color="tab:orange", alpha=0.8)
        plt.grid(True, alpha=0.3)
        plt.legend()
        plt.tight_layout()
        if args.plot:
            plt.savefig(args.plot, dpi=150)
            print(f"Saved plot -> {args.plot}")
        else:
            plt.show()


if __name__ == "__main__":
    main()


