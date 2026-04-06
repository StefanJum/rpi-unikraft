import re
import numpy as np
import pandas as pd
import sys

def convert_time_to_us(time_str):
    if 'ms' in time_str:
        return float(time_str.replace('ms', '')) * 1000
    elif 'us' in time_str:
        return float(time_str.replace('us', ''))
    elif 's' in time_str:
        return float(time_str.replace('s', '')) * 1_000_000
    else:
        raise ValueError(f"Unknown time unit in {time_str}")

def convert_transfer_to_kb(transfer_str):
    if 'MB' in transfer_str:
        return float(transfer_str.replace('MB', '')) * 1024
    elif 'KB' in transfer_str:
        return float(transfer_str.replace('KB', ''))
    else:
        raise ValueError(f"Unknown transfer unit in {transfer_str}")

def convert_req_sec(req_sec_str):
    if 'k' in req_sec_str:
        return float(req_sec_str.replace('k', '')) * 1000
    else:
        return float(req_sec_str)

def parse_file(file_path):
    with open(file_path, 'r') as file:
        content = file.read()
    
    # Regex patterns to extract the required data
    size_pattern = re.compile(r'(\d+ Bytes)')
    latency_pattern = re.compile(r'Latency\s+(\d+\.\d+\w+)\s+(\d+\.\d+\w+)\s+(\d+\.\d+\w+)')
    req_sec_pattern = re.compile(r'Req/Sec\s+(\d+\.\d+\w?)\s+(\d+\.\d+)\s+(\d+\.\d+\w?)')
    latency_distribution_pattern = re.compile(r'(\d+)%\s+(\d+\.\d+\w+)')
    requests_sec_pattern = re.compile(r'Requests/sec:\s+(\d+\.\d+)')
    transfer_sec_pattern = re.compile(r'Transfer/sec:\s+(\d+\.\d+\w+)')
    
    # Find all matches
    sizes = size_pattern.findall(content)
    latencies = latency_pattern.findall(content)
    req_secs = req_sec_pattern.findall(content)
    latency_distributions = latency_distribution_pattern.findall(content)
    requests_secs = requests_sec_pattern.findall(content)
    transfer_secs = transfer_sec_pattern.findall(content)
    
    # Debug prints
    # print(f"Sizes: {sizes}")
    # print(f"Latencies: {latencies}")
    # print(f"Req/Secs: {req_secs}")
    # print(f"Latency Distributions: {latency_distributions}")
    # print(f"Requests/Secs: {requests_secs}")
    # print(f"Transfer/Secs: {transfer_secs}")
    
    if not (len(sizes) == len(latencies) == len(req_secs) == len(requests_secs) == len(transfer_secs)):
        raise ValueError("Mismatch in the number of parsed elements. Check the data format and regex patterns.")
    
    # Prepare the data for the table
    data = []
    for i, size in enumerate(sizes):
        latency = latencies[i]
        req_sec = req_secs[i]
        request_sec = requests_secs[i]
        transfer_sec = transfer_secs[i]
        
        # Extract latency distribution values
        latency_distribution = latency_distributions[i*4:(i+1)*4]
        latency_50 = latency_distribution[0][1]
        latency_75 = latency_distribution[1][1]
        latency_90 = latency_distribution[2][1]
        latency_99 = latency_distribution[3][1]
        
        data.append([
            size, 
            convert_time_to_us(latency[0]), convert_time_to_us(latency[1]), convert_time_to_us(latency[2]), 
            convert_req_sec(req_sec[0]), float(req_sec[1]), convert_req_sec(req_sec[2]),
            convert_time_to_us(latency_50), convert_time_to_us(latency_75), convert_time_to_us(latency_90), convert_time_to_us(latency_99), 
            float(request_sec), convert_transfer_to_kb(transfer_sec)
        ])
    
    # Create a DataFrame for better readability
    columns = [
        'Size', 'Avg Latency (us)', 'Stdev Latency (us)', 'Max Latency (us)', 
        'Avg Req/Sec', 'Stdev Req/Sec', 'Max Req/Sec',
        '50% Latency (us)', '75% Latency (us)', '90% Latency (us)', '99% Latency (us)', 
        'Requests/sec', 'Transfer/sec (KB)'
    ]
    df = pd.DataFrame(data, columns=columns)
    
    return df

import matplotlib.pyplot as plt

def generate_bar_chart(df):
    # Extract the necessary columns
    sizes = df['Size']
    requests_sec = df['Requests/sec']
    transfer_sec = df['Transfer/sec (KB)']

    fig, ax1 = plt.subplots(figsize=(10, 6))

    # Plot Requests/sec on the primary Y-axis
    ax1.bar(sizes, requests_sec, color='skyblue', label='Requests/sec')
    ax1.set_xlabel('Request Size')
    ax1.set_ylabel('Requests/sec', color='skyblue')
    ax1.tick_params(axis='y', labelcolor='skyblue')

    # Create a secondary Y-axis to plot Transfer/sec
    ax2 = ax1.twinx()
    ax2.plot(sizes, transfer_sec, color='green', marker='o', label='Transfer/sec (KB)')
    ax2.set_ylabel('Transfer/sec (KB)', color='green')
    ax2.tick_params(axis='y', labelcolor='green')

    # Add a title and labels
    plt.title('Requests/sec and Transfer/sec by Size')

    # Rotate the x labels for better readability
    plt.xticks(rotation=45, ha='right')

    # Add legends
    ax1.legend(loc='upper left')
    ax2.legend(loc='upper right')

    # Display the chart
    plt.tight_layout()
    # plt.show()
    plt.savefig(sys.argv[1].replace('/', '-').rsplit('.', 1)[0] + '.pdf', format="pdf")


def generate_comparison_chart(df, avg_column, stdev_column, max_column=None, y_label='', title=''):
    # Extract the necessary columns
    sizes = df['Size']
    avg_values = df[avg_column]
    stdev_values = df[stdev_column]
    max_values = df[max_column] if max_column else None

    fig, ax = plt.subplots(figsize=(10, 6))

    # Plot Avg and Stdev values on the same Y-axis
    bar_width = 0.3 if max_column else 0.4
    x = np.arange(len(sizes))

    ax.bar(x - bar_width/2, avg_values, bar_width, color='skyblue', label=f'Avg {y_label}')
    ax.bar(x + bar_width/2, stdev_values, bar_width, color='orange', label=f'Stdev {y_label}')

    if max_column:
        ax.bar(x + bar_width * 1.5, max_values, bar_width, color='green', label=f'Max {y_label}')

    # Set the x and y labels
    ax.set_xlabel('Request Size')
    ax.set_ylabel(y_label)
    ax.set_title(title)

    # Set the x-ticks and labels
    ax.set_xticks(x)
    ax.set_xticklabels(sizes, rotation=45, ha='right')

    # Add a legend
    ax.legend()

    # Display the chart
    plt.tight_layout()
    # plt.show()
    plt.savefig(sys.argv[1].replace('/', '-' + sys.argv[2] + '-').rsplit('.', 1)[0] + '.pdf', format="pdf")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python script.py <file_path> <rt | l | r>")
        sys.exit(1)
    
    file_path = sys.argv[1]
    df = parse_file(file_path)
    # print(df)

    type = sys.argv[2]

    # # Generate the bar chart for Requests/sec and Transfer/sec
    # generate_bar_chart(df)
    
    # Generate the latency chart for Avg Latency, Stdev Latency, and optionally Max Latency
    # generate_comparison_chart(df, 'Avg Latency (us)', 'Stdev Latency (us)', 'Max Latency (us)', 'Latency (us)', 'Avg, Stdev, and Max Latency by Size')
    
    # Generate the req/sec chart for Avg Req/Sec, Stdev Req/Sec, and optionally Max Req/Sec
    # generate_comparison_chart(df, 'Avg Req/Sec', 'Stdev Req/Sec', 'Max Req/Sec', 'Req/Sec', 'Avg, Stdev, and Max Req/Sec by Size')

    # Generate the bar chart for Requests/sec and Transfer/sec
    if type == 'rt':
        generate_bar_chart(df)

    # Generate the latency chart for Avg Latency, Stdev Latency
    if type == 'l':
        generate_comparison_chart(df, 'Avg Latency (us)', 'Stdev Latency (us)', None, 'Latency (us)', 'Avg, Stdev, and Max Latency by Size')

    # Generate the req/sec chart for Avg Req/Sec, Stdev Req/Sec
    if type == 'r':
        generate_comparison_chart(df, 'Avg Req/Sec', 'Stdev Req/Sec', None, 'Req/Sec', 'Avg, Stdev, and Max Req/Sec by Size')