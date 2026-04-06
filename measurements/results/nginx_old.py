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
        
        # Extract standard deviation
        stdev = float(re.search(r'Req/Sec\s+[\d.]+k?\s+([\d.]+)k?', test).group(1))
        
        # Convert transfer speed to Mbps
        if transfer_unit == 'KB':
            transfer_value = transfer_value * 8 / 1000  # KB/s to Mbps
        elif transfer_unit == 'MB':
            transfer_value = transfer_value * 8  # MB/s to Mbps
        elif transfer_unit == 'GB':
            transfer_value = transfer_value * 8 * 1000  # GB/s to Mbps

        results[size] = {'req_sec': req_sec, 'transfer_sec': transfer_value, 'req_sec_stdev': stdev}
    
    return results

def create_graph(unikraft_data, linux_data, connection_count):
    x = range(len(file_sizes))
    width = 0.2
    
    fig, ax1 = plt.subplots(figsize=(10, 6))
    ax2 = ax1.twinx()
    
    unikraft_req = [unikraft_data[size]['req_sec']/1000 for size in file_sizes]
    linux_req = [linux_data[size]['req_sec']/1000 for size in file_sizes]
    unikraft_stdev = [unikraft_data[size]['req_sec_stdev']/1000 for size in file_sizes]
    linux_stdev = [linux_data[size]['req_sec_stdev']/1000 for size in file_sizes]

    def add_labels(rects, data, errors, ax):
        for rect, value, error in zip(rects, data, errors):
            height = rect.get_height()
            ax.text(rect.get_x() + rect.get_width()/2. + 0.015, height + error + 0.02,  # Position above error bar
                    f'{value:.2f}',
                    ha='center', va='bottom',
                    rotation='vertical',
                    fontsize=15)

    unikraft_req_color = '#ca1017'
    linux_req_color = '#419ada'
    unikraft_transfer_color = '#007c2c'
    linux_transfer_color = '#d866cd'

    unikraft_req_bars = ax1.bar([i - 1.5*width for i in x], unikraft_req, width, yerr=unikraft_stdev, 
            error_kw=dict(capsize=5), label='Unikraft', color=unikraft_req_color, edgecolor='black', linewidth=1.5)
    linux_req_bars = ax1.bar([i - 0.5*width for i in x], linux_req, width, yerr=linux_stdev, 
            error_kw=dict(capsize=5), label='Linux', color=linux_req_color, edgecolor='black', linewidth=1.5)
    unikraft_transfer_bars = ax2.bar([i + 0.5*width for i in x], [unikraft_data[size]['transfer_sec'] for size in file_sizes], width, 
            label='Unikraft', color=unikraft_transfer_color, edgecolor='black', linewidth=1.5)
    linux_transfer_bars = ax2.bar([i + 1.5*width for i in x], [linux_data[size]['transfer_sec'] for size in file_sizes], width, 
            label='Linux', color=linux_transfer_color, edgecolor='black', linewidth=1.5)

        # Add vertical separator lines
    for i in range(len(file_sizes) - 1):
        x_position = i + 0.5  # Position between each group of bars
        ax1.axvline(x=x_position, color='black', linestyle='--', linewidth=0.5, alpha=0.5)


    # Add labels to bars
    add_labels(unikraft_req_bars, unikraft_req, unikraft_stdev, ax1)
    add_labels(linux_req_bars, linux_req, linux_stdev, ax1)
    add_labels(unikraft_transfer_bars, [unikraft_data[size]['transfer_sec'] for size in file_sizes], [1]*len(file_sizes), ax2)
    add_labels(linux_transfer_bars, [linux_data[size]['transfer_sec'] for size in file_sizes], [1]*len(file_sizes), ax2)

    axis_font_size = 21

    ax1.set_xlabel('File Size')
    ax1.xaxis.label.set_fontsize(axis_font_size)

    ax1.set_ylabel('Average Throughput (x1000 req/s)')
    ax1.yaxis.label.set_fontsize(axis_font_size)

    ax2.set_ylabel('Transfer Speed (Mbps)')
    ax2.yaxis.label.set_fontsize(axis_font_size)

    ax1.set_xticks(x)
    file_sizes_labels = ['64B', '256B', '1kiB', '4kiB', '16kiB', '64kiB', '256kiB']
    ax1.set_xticklabels(file_sizes_labels)

    extend_y = 1.5 # Extend above the highest point
    max_height_ax1 = max(max(unikraft_req), max(linux_req)) + max(max(unikraft_stdev), max(linux_stdev))  # Highest point including error bars
    ax1.set_ylim(0, max_height_ax1 * extend_y)

    max_height_ax1 = max(max([unikraft_data[size]['transfer_sec'] for size in file_sizes]), max([linux_data[size]['transfer_sec'] for size in file_sizes]))  # Highest point including error bars
    ax2.set_ylim(0, max_height_ax1 * extend_y)

    ax1.tick_params(axis='y', which='major', labelsize=18)
    ax1.tick_params(axis='x', which='major', labelsize=15)
    ax2.tick_params(axis='y', which='major', labelsize=18)

    # Create custom legend elements
    unikraft_req_patch = mpatches.Patch(color=unikraft_req_color, label='Unikraft req/s')
    unikraft_transfer_patch = mpatches.Patch(color=unikraft_transfer_color, label='Unikraft transfer')
    linux_req_patch = mpatches.Patch(color=linux_req_color, label='Linux req/s')
    linux_transfer_patch = mpatches.Patch(color=linux_transfer_color, label='Linux transfer')

    # Combine legend elements
    legend_elements = [unikraft_req_patch, unikraft_transfer_patch, linux_req_patch, linux_transfer_patch]

    # Add the custom legend
    ax1.legend(handles=legend_elements, loc='upper left', ncol=2, prop=FontProperties(size=19))
    
    plt.tight_layout()
    plt.savefig(f'nginx-{connection_count}.pdf', format="pdf")
    plt.close()

# Main execution
connection_counts = [1, 10]

for count in connection_counts:
    unikraft_file = f'unikraft/wrk-{count}.txt'
    linux_file = f'linux/wrk-{count}.txt'
    
    unikraft_data = parse_file(unikraft_file)
    linux_data = parse_file(linux_file)
    
    create_graph(unikraft_data, linux_data, count)

    uk_reqs = [unikraft_data[size]['req_sec'] for size in file_sizes]
    linux_reqs = [linux_data[size]['req_sec'] for size in file_sizes]
    
    print(f"Unikraft speed increase (or decrease) for {count} connection(s) compared to Linux:")
    for i in range(len(uk_reqs)):
        print(f"{file_sizes[i]}: {((uk_reqs[i] - linux_reqs[i]) / linux_reqs[i]) * 100:.0f}%")
    print("")

print("Graphs have been generated and saved.")
