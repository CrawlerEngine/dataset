#!/usr/bin/env python3
"""
Utility script for working with Parquet datasets from the C++ crawler.
"""

import argparse
import pandas as pd
import sys
from pathlib import Path


def read_parquet(filepath: str) -> pd.DataFrame:
    """Read Parquet file and return DataFrame."""
    try:
        df = pd.read_parquet(filepath)
        return df
    except FileNotFoundError:
        print(f"Error: File '{filepath}' not found")
        sys.exit(1)
    except Exception as e:
        print(f"Error reading Parquet file: {e}")
        sys.exit(1)


def show_info(filepath: str) -> None:
    """Display information about the dataset."""
    df = read_parquet(filepath)
    
    print(f"\n{'='*60}")
    print(f"Dataset Information: {filepath}")
    print(f"{'='*60}\n")
    
    print(f"Total records: {len(df)}")
    print(f"Columns: {', '.join(df.columns)}")
    print(f"\nMemory usage: {df.memory_usage(deep=True).sum() / 1024 / 1024:.2f} MB")
    
    print(f"\n{'Status Code Distribution:':^60}")
    print(df['status_code'].value_counts().to_string())
    
    print(f"\n{'Content Statistics:':^60}")
    df['content_length'] = df['content'].str.len()
    print(df['content_length'].describe())
    
    print(f"\nFirst 5 records:")
    print(df[['url', 'title', 'status_code']].head(5).to_string())


def convert_to_csv(parquet_file: str, csv_file: str) -> None:
    """Convert Parquet to CSV."""
    df = read_parquet(parquet_file)
    df.to_csv(csv_file, index=False)
    print(f"✓ Converted {parquet_file} to {csv_file}")


def convert_to_json(parquet_file: str, json_file: str) -> None:
    """Convert Parquet to JSON."""
    df = read_parquet(parquet_file)
    df.to_json(json_file, orient='records', indent=2)
    print(f"✓ Converted {parquet_file} to {json_file}")


def merge_parquet_files(input_files: list, output_file: str) -> None:
    """Merge multiple Parquet files."""
    dfs = []
    
    for file in input_files:
        df = read_parquet(file)
        dfs.append(df)
    
    merged_df = pd.concat(dfs, ignore_index=True)
    merged_df.to_parquet(output_file, index=False)
    
    print(f"✓ Merged {len(input_files)} files into {output_file}")
    print(f"  Total records: {len(merged_df)}")


def filter_by_status(parquet_file: str, output_file: str, status_code: int) -> None:
    """Filter records by HTTP status code."""
    df = read_parquet(parquet_file)
    filtered_df = df[df['status_code'] == status_code]
    
    filtered_df.to_parquet(output_file, index=False)
    
    print(f"✓ Filtered records with status {status_code}")
    print(f"  Records found: {len(filtered_df)}")
    print(f"  Saved to: {output_file}")


def sample_dataset(parquet_file: str, output_file: str, n: int) -> None:
    """Create a sample of the dataset."""
    df = read_parquet(parquet_file)
    
    sample_df = df.sample(n=min(n, len(df)), random_state=42)
    sample_df.to_parquet(output_file, index=False)
    
    print(f"✓ Created sample dataset")
    print(f"  Original records: {len(df)}")
    print(f"  Sample records: {len(sample_df)}")
    print(f"  Saved to: {output_file}")


def main():
    parser = argparse.ArgumentParser(
        description='Utility for working with Parquet datasets from C++ crawler'
    )
    
    subparsers = parser.add_subparsers(dest='command', help='Commands')
    
    # Info command
    info_parser = subparsers.add_parser('info', help='Show dataset information')
    info_parser.add_argument('file', help='Parquet file')
    
    # Convert to CSV
    csv_parser = subparsers.add_parser('to-csv', help='Convert to CSV')
    csv_parser.add_argument('input', help='Input Parquet file')
    csv_parser.add_argument('output', help='Output CSV file')
    
    # Convert to JSON
    json_parser = subparsers.add_parser('to-json', help='Convert to JSON')
    json_parser.add_argument('input', help='Input Parquet file')
    json_parser.add_argument('output', help='Output JSON file')
    
    # Merge files
    merge_parser = subparsers.add_parser('merge', help='Merge multiple Parquet files')
    merge_parser.add_argument('files', nargs='+', help='Input Parquet files')
    merge_parser.add_argument('-o', '--output', default='merged.parquet', help='Output file')
    
    # Filter by status
    filter_parser = subparsers.add_parser('filter', help='Filter by HTTP status code')
    filter_parser.add_argument('input', help='Input Parquet file')
    filter_parser.add_argument('status', type=int, help='HTTP status code')
    filter_parser.add_argument('-o', '--output', required=True, help='Output file')
    
    # Sample
    sample_parser = subparsers.add_parser('sample', help='Create a sample dataset')
    sample_parser.add_argument('input', help='Input Parquet file')
    sample_parser.add_argument('n', type=int, help='Number of samples')
    sample_parser.add_argument('-o', '--output', required=True, help='Output file')
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return
    
    try:
        if args.command == 'info':
            show_info(args.file)
        elif args.command == 'to-csv':
            convert_to_csv(args.input, args.output)
        elif args.command == 'to-json':
            convert_to_json(args.input, args.output)
        elif args.command == 'merge':
            merge_parquet_files(args.files, args.output)
        elif args.command == 'filter':
            filter_by_status(args.input, args.output, args.status)
        elif args.command == 'sample':
            sample_dataset(args.input, args.output, args.n)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
