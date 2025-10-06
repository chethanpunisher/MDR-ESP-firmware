import argparse
import csv
import sys
from datetime import datetime

import matplotlib.pyplot as plt

def read_csv_manual(path):
    ts = []
    vals = []
    with open(path, newline="", encoding="utf-8") as f:
        r = csv.reader(f)
        header = next(r, None)
        for row in r:
            if not row or len(row) < 2:
                continue
            try:
                t = datetime.fromisoformat(row[0])
                v = float(row[1])
            except Exception:
                continue
            ts.append(t)
            vals.append(v)
    return ts, vals


def moving_average(values, window):
    if window <= 1:
        return values
    out = []
    s = 0.0
    q = []
    for v in values:
        q.append(v)
        s += v
        if len(q) > window:
            s -= q.pop(0)
        out.append(s / len(q))
    return out


def main():
    p = argparse.ArgumentParser(description="Plot loadcell raw_value from CSV")
    p.add_argument("--file", default="loadcell_log.csv", help="CSV file path")
    p.add_argument("--save", default=None, help="Save figure to path instead of showing")
    p.add_argument("--ma", type=int, default=1, help="Moving average window (samples)")
    args = p.parse_args()

    try:
        import pandas as pd  # optional
        df = pd.read_csv(args.file)
        if "timestamp_iso" not in df.columns or "raw_value" not in df.columns:
            print("CSV must have columns: timestamp_iso, raw_value", file=sys.stderr)
            sys.exit(1)
        df["timestamp_iso"] = pd.to_datetime(df["timestamp_iso"], errors="coerce")
        df = df.dropna(subset=["timestamp_iso", "raw_value"])  # clean
        y = df["raw_value"].astype(float).tolist()
        if args.ma > 1:
            y = moving_average(y, args.ma)
        x = df["timestamp_iso"].tolist()
    except Exception:
        # Fallback without pandas
        x, y = read_csv_manual(args.file)
        if not x:
            print("No data to plot.", file=sys.stderr)
            sys.exit(1)
        if args.ma > 1:
            y = moving_average(y, args.ma)

    plt.figure(figsize=(10, 5))
    plt.plot(x, y, label="raw_value")
    if args.ma > 1:
        plt.title(f"Loadcell raw_value (MA={args.ma})")
    else:
        plt.title("Loadcell raw_value")
    plt.xlabel("Time")
    plt.ylabel("Raw Value")
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()

    if args.save:
        plt.savefig(args.save, dpi=150)
        print(f"Saved plot -> {args.save}")
    else:
        plt.show()


if __name__ == "__main__":
    main()


