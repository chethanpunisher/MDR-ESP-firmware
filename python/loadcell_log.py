import argparse
import csv
import os
import re
import sys
from datetime import datetime

try:
    import serial  # pyserial
except ImportError:
    print("pyserial not installed. Install with: pip install pyserial", file=sys.stderr)
    sys.exit(1)


LINE_REGEX = re.compile(r"mode\d+\s*:\s*(?:raw:)?\s*(\d+)")


def parse_value(line: str):
    match = LINE_REGEX.search(line)
    if match:
        try:
            return int(match.group(1))
        except ValueError:
            return None
    return None


def ensure_csv_with_header(path: str):
    exists = os.path.exists(path)
    f = open(path, "a", newline="", encoding="utf-8")
    writer = csv.writer(f)
    if not exists:
        writer.writerow(["timestamp_iso", "raw_value"])  # header
        f.flush()
    return f, writer


def main():
    parser = argparse.ArgumentParser(description="Log raw loadcell data from ESP32 UART to CSV")
    parser.add_argument("--port", required=True, help="Serial port (e.g., COM5 or /dev/ttyUSB0)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--out", default="loadcell_log.csv", help="Output CSV path")
    parser.add_argument("--print", dest="do_print", action="store_true", help="Print values to console")
    args = parser.parse_args()

    ser = serial.Serial(args.port, args.baud, timeout=1)
    csv_file, writer = ensure_csv_with_header(args.out)

    print(f"Logging raw loadcell values from {args.port} @ {args.baud} to {args.out}. Press Ctrl+C to stop.")
    try:
        while True:
            try:
                line_bytes = ser.readline()
            except serial.SerialException as e:
                print(f"Serial error: {e}", file=sys.stderr)
                break

            if not line_bytes:
                continue
            try:
                line = line_bytes.decode("utf-8", errors="ignore").strip()
            except Exception:
                continue

            val = parse_value(line)
            if val is not None:
                ts = datetime.utcnow().isoformat()
                writer.writerow([ts, val])
                csv_file.flush()
                if args.do_print:
                    print(f"{ts}, {val}")
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        try:
            csv_file.close()
        except Exception:
            pass
        try:
            ser.close()
        except Exception:
            pass


if __name__ == "__main__":
    main()


