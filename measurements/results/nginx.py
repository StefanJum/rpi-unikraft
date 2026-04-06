import matplotlib.pyplot as plt
import re
from matplotlib.transforms import Affine2D
import matplotlib.transforms as transforms
from matplotlib.font_manager import FontProperties
import matplotlib.patches as mpatches

file_sizes = ['64', '256', '1024', '4096', '16384', '65536', '262144']

class HandlerTuple(object):
    def __init__(self, ndivide=1):
        self.ndivide = ndivide

    def create_artists(self, legend, orig_handle, xdescent, ydescent, width, height, fontsize, trans):
        patches = []
        for i, patch in enumerate(orig_handle):
            if self.ndivide:
                width_div = width / self.ndivide
                xdescent_div = xdescent + i * width_div
            else:
                width_div = width
                xdescent_div = xdescent
            p = mpatches.Rectangle(xy=(xdescent_div, ydescent), width=width_div, height=height, 
                                   facecolor=patch.get_facecolor(), edgecolor=patch.get_edgecolor())
            patches.append(p)
        return patches


def parse_file(filename):
    with open(filename, 'r') as file:
        content = file.read()
    
    tests = re.split(r'\n\n+', content.strip())
    results = {}
    
    for test in tests:
        lines = test.split('\n')
        size = lines[0].split()[0]
        req_sec = float(re.search(r'Requests/sec:\s+([\d.]+)', test).group(1))
        transfer_sec = re.search(r'Transfer/sec:\s+([\d.]+)(\w+)', test)
        transfer_value = float(transfer_sec.group(1))
        transfer_unit = transfer_sec.group(2)
        
        # Extract standard deviation and 90% latency
        stdev = float(re.search(r'Req/Sec\s+[\d.]+k?\s+([\d.]+)k?', test).group(1))
        latency_90_match = re.search(r'90%\s+([\d.]+)([a-z]+)', test)
        latency_90 = float(latency_90_match.group(1))
        latency_90_unit = latency_90_match.group(2)
        
        # Convert latency to milliseconds if necessary
        if latency_90_unit == 'us':
            latency_90 /= 1000  # Convert microseconds to milliseconds
        
        # Convert transfer speed to Mbps
        if transfer_unit == 'KB':
            transfer_value = transfer_value * 8 / 1000  # KB/s to Mbps
        elif transfer_unit == 'MB':
            transfer_value = transfer_value * 8  # MB/s to Mbps
        elif transfer_unit == 'GB':
            transfer_value = transfer_value * 8 * 1000  # GB/s to Mbps

        results[size] = {'req_sec': req_sec, 'transfer_sec': transfer_value, 'req_sec_stdev': stdev, 'latency_90': latency_90}
    
    return results

def create_latency_graph(unikraft_data, linux_data, connection_count):
    x = range(len(file_sizes))
    
    fig, ax1 = plt.subplots(figsize=(10, 6))
    
    unikraft_latency = [unikraft_data[size]['latency_90'] for size in file_sizes]
    linux_latency = [linux_data[size]['latency_90'] for size in file_sizes]

    unikraft_latency_color = '#007c2c'
    linux_latency_color = '#d866cd'

    unikraft_line = ax1.plot(x, unikraft_latency, '-o', label='Unikraft', color=unikraft_latency_color, 
                             linewidth=1.5, markersize=12)
    linux_line = ax1.plot(x, linux_latency, '-o', label='Linux', color=linux_latency_color, 
                          linewidth=1.5, markersize=12)

    axis_font_size = 21

    ax1.set_xlabel('File Size')
    ax1.xaxis.label.set_fontsize(axis_font_size)

    ax1.set_ylabel('90% Latency (ms)')
    ax1.yaxis.label.set_fontsize(axis_font_size)

    ax1.set_xticks(x)
    file_sizes_labels = ['64B', '256B', '1kiB', '4kiB', '16kiB', '64kiB', '256kiB']
    ax1.set_xticklabels(file_sizes_labels)

    ax1.set_yscale('log')
    extend_y = 3  # Extend above the highest point
    max_height_ax1 = max(max(unikraft_latency), max(linux_latency))  # Highest point
    ax1.set_ylim(0.5, max_height_ax1 * extend_y)

    # Custom y-axis tick labels
    y_ticks = [0.5, 1, 10, 100]
    ax1.set_yticks(y_ticks)
    ax1.set_yticklabels([f'{tick}' for tick in y_ticks])

    ax1.tick_params(axis='y', which='major', labelsize=18)
    ax1.tick_params(axis='x', which='major', labelsize=18)

    # Create custom legend elements
    unikraft_latency_patch = mpatches.Patch(color=unikraft_latency_color, label='Unikraft latency')
    linux_latency_patch = mpatches.Patch(color=linux_latency_color, label='Linux latency')

    # Combine legend elements
    legend_elements = [unikraft_latency_patch, linux_latency_patch]

    # Add the custom legend
    ax1.legend(handles=legend_elements, loc='upper right', ncol=2, prop=FontProperties(size=19))
    
    plt.tight_layout()
    plt.savefig(f'nginx-latency-{connection_count}.pdf', format="pdf")
    plt.close()

def create_graph(unikraft_data, linux_data, connection_count):
    x = range(len(file_sizes))
    width = 0.4
    
    fig, ax1 = plt.subplots(figsize=(10, 6))
    
    unikraft_req = [unikraft_data[size]['req_sec']/1000 for size in file_sizes]
    linux_req = [linux_data[size]['req_sec']/1000 for size in file_sizes]

    unikraft_req_color = '#ca1017'
    linux_req_color = '#419ada'

    unikraft_line = ax1.plot(x, unikraft_req, '-o', label='Unikraft', color=unikraft_req_color, 
                             linewidth=1.5, markersize=12)
    linux_line = ax1.plot(x, linux_req, '-o', label='Linux', color=linux_req_color, 
                          linewidth=1.5, markersize=12)

    axis_font_size = 21

    ax1.set_xlabel('File Size')
    ax1.xaxis.label.set_fontsize(axis_font_size)

    ax1.set_ylabel('Average Throughput (x1000 req/s)')
    ax1.yaxis.label.set_fontsize(axis_font_size)

    ax1.set_xticks(x)
    file_sizes_labels = ['64B', '256B', '1kiB', '4kiB', '16kiB', '64kiB', '256kiB']
    ax1.set_xticklabels(file_sizes_labels)

    extend_y = 1.2 # Extend above the highest point
    max_height_ax1 = max(max(unikraft_req), max(linux_req))  # Highest point
    ax1.set_ylim(0, max_height_ax1 * extend_y)

    ax1.tick_params(axis='y', which='major', labelsize=18)
    ax1.tick_params(axis='x', which='major', labelsize=18)

    # Create custom legend elements
    unikraft_req_patch = mpatches.Patch(color=unikraft_req_color, label='Unikraft req/s')
    linux_req_patch = mpatches.Patch(color=linux_req_color, label='Linux req/s')

    # Combine legend elements
    legend_elements = [unikraft_req_patch, linux_req_patch]

    # Add the custom legend
    ax1.legend(handles=legend_elements, loc='upper right', ncol=2, prop=FontProperties(size=19))
    
    plt.tight_layout()
    plt.savefig(f'nginx-reqs-{connection_count}.pdf', format="pdf")
    plt.close()

# Main execution
connection_counts = [1, 10]

for count in connection_counts:
    unikraft_file = f'unikraft/wrk-{count}.txt'
    linux_file = f'linux/wrk-{count}.txt'
    
    unikraft_data = parse_file(unikraft_file)
    linux_data = parse_file(linux_file)
    
    create_graph(unikraft_data, linux_data, count)
    create_latency_graph(unikraft_data, linux_data, count)

    uk_reqs = [unikraft_data[size]['req_sec'] for size in file_sizes]
    linux_reqs = [linux_data[size]['req_sec'] for size in file_sizes]
    
    print(f"Unikraft speed increase (or decrease) for {count} connection(s) compared to Linux:")
    for i in range(len(uk_reqs)):
        print(f"{file_sizes[i]}: {((uk_reqs[i] - linux_reqs[i]) / linux_reqs[i]) * 100:.0f}%")
    print("")

print("Graphs have been generated and saved.")