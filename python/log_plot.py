import argparse
import csv
import sys
from datetime import datetime
import os

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


def read_excel_file(path):
    """Read Excel file and return x-axis indices and first column values"""
    try:
        import pandas as pd
        df = pd.read_excel(path)
        
        if len(df.columns) == 0:
            raise ValueError("Excel file has no columns")
        
        # Use first column only
        col1 = df.columns[0]  # 1st column (0-indexed)
        
        # Remove rows with invalid data
        df = df.dropna(subset=[col1])
        
        # Convert values to float, skip non-numeric values
        vals = []
        x_indices = []
        
        for i, val in enumerate(df[col1]):
            try:
                float_val = float(val)
                vals.append(float_val)
                x_indices.append(i)
            except (ValueError, TypeError):
                continue
        
        if not vals:
            raise ValueError("No valid numeric data found in first column")
        
        return x_indices, vals
        
    except ImportError:
        raise ImportError("pandas and openpyxl are required to read Excel files. Install with: pip install pandas openpyxl")
    except Exception as e:
        raise Exception(f"Error reading Excel file: {str(e)}")


def detect_file_format(file_path):
    """Detect if file is CSV or Excel based on extension"""
    _, ext = os.path.splitext(file_path.lower())
    if ext in ['.xlsx', '.xls']:
        return 'excel'
    elif ext == '.csv':
        return 'csv'
    else:
        # Default to CSV if extension is unknown
        return 'csv'


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
    p = argparse.ArgumentParser(description="Plot loadcell raw_value from CSV or Excel file")
    p.add_argument("--file", default="loadcell_log.csv", help="CSV or Excel file path")
    p.add_argument("--save", default=None, help="Save figure to path instead of showing")
    p.add_argument("--ma", type=int, default=1, help="Moving average window (samples)")
    args = p.parse_args()

    # Detect file format
    file_format = detect_file_format(args.file)
    
    try:
        if file_format == 'excel':
            # Read Excel file - returns x, y (1st column only)
            x, y = read_excel_file(args.file)
            if args.ma > 1:
                y = moving_average(y, args.ma)
        else:
            # Try pandas first for CSV
            try:
                import pandas as pd
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
            
    except Exception as e:
        print(f"Error reading file: {str(e)}", file=sys.stderr)
        sys.exit(1)

    plt.figure(figsize=(10, 5))
    
    # Plot data based on file format
    if file_format == 'excel':
        # Plot 1st column only
        plt.plot(x, y, label="Column 1", color='red', linewidth=1.5)
        
        if args.ma > 1:
            plt.title(f"Excel Data - Column 1 (MA={args.ma})")
        else:
            plt.title("Excel Data - Column 1")
        plt.xlabel("Data Point Index")
    else:
        # Plot single column for CSV
        plt.plot(x, y, label="Data", color='blue', linewidth=1.5)
        
        if args.ma > 1:
            plt.title(f"Loadcell Data (MA={args.ma})")
        else:
            plt.title("Loadcell Data")
        plt.xlabel("Time")
    
    plt.ylabel("Value")
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


