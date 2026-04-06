import numpy as np
import matplotlib.pyplot as plt
from matplotlib.font_manager import FontProperties

def load_data(filename):
    with open(filename, 'r') as f:
        data = eval(f.read())
    return np.array(data)

def calculate_stats(data, file_size_mb):
    speeds = (file_size_mb * 8) / data  # Convert each time to speed in Mbps
    avg_speed = np.mean(speeds)
    std_dev_speed = np.std(speeds)
    return avg_speed, std_dev_speed

# Load data
linux_10mb = load_data('linux/tcp-100.txt')
unikraft_10mb = load_data('unikraft/tcp-100.txt')
linux_100mb = load_data('linux/tcp-50.txt')
unikraft_100mb = load_data('unikraft/tcp-50.txt')

# Calculate stats
linux_10mb_stats = calculate_stats(linux_10mb, 10)
unikraft_10mb_stats = calculate_stats(unikraft_10mb, 10)
linux_100mb_stats = calculate_stats(linux_100mb, 100)
unikraft_100mb_stats = calculate_stats(unikraft_100mb, 100)

# Print stats
print(f"Linux 10MB: Avg: {linux_10mb_stats[0]:.2f} Mbps, Std Dev: {linux_10mb_stats[1]:.2f} Mbps")
print(f"Unikraft 10MB: Avg: {unikraft_10mb_stats[0]:.2f} Mbps, Std Dev: {unikraft_10mb_stats[1]:.2f} Mbps")
print(f"Linux 100MB: Avg: {linux_100mb_stats[0]:.2f} Mbps, Std Dev: {linux_100mb_stats[1]:.2f} Mbps")
print(f"Unikraft 100MB: Avg: {unikraft_100mb_stats[0]:.2f} Mbps, Std Dev: {unikraft_100mb_stats[1]:.2f} Mbps")

print(f"\nUnikraft is x% slower compared to linux:")
# Print the % speedup from average Linux to average Unikraft
speedup_10mb = ((linux_10mb_stats[0] - unikraft_10mb_stats[0]) / linux_10mb_stats[0]) * 100
print(f"10mb: {speedup_10mb:.0f}%")

speedup_100mb = ((linux_100mb_stats[0] - unikraft_100mb_stats[0]) / linux_100mb_stats[0]) * 100
print(f"100mb: {speedup_100mb:.0f}%")

combined_speedup = (speedup_10mb + speedup_100mb) / 2
print(f"Average: {combined_speedup:.0f}%")
# Prepare data for plotting
labels = ['Linux\n10MB', 'Unikraft\n10MB', 'Linux\n100MB', 'Unikraft\n100MB']
means = [linux_10mb_stats[0], unikraft_10mb_stats[0], linux_100mb_stats[0], unikraft_100mb_stats[0]]
std_devs = [linux_10mb_stats[1], unikraft_10mb_stats[1], linux_100mb_stats[1], unikraft_100mb_stats[1]]

# Create plot
fig, ax = plt.subplots(figsize=(10, 6))

# Plot bars
width = 0.5
first_group_start = 0
second_group_start = 1.3
x = [first_group_start, first_group_start + width, second_group_start, second_group_start + width]  # Adjust these values to set bar positions
# Blue for Linux, red for Unikraft
l_color = '#f4995a'
u_color = '#007575'
colors = [l_color, u_color, l_color, u_color]
rects = ax.bar(x, means, width, yerr=std_devs, capsize=10, color=colors,
               edgecolor='black', linewidth=3,
               error_kw=dict(elinewidth=3, capthick=3))

# Customize plot
axis_font_size = 30

ax.set_ylabel('Transfer Speed (Mbps)')
ax.yaxis.label.set_fontsize(axis_font_size)  # Adjust size as needed

ax.set_xlabel('Test Size (MB)')
ax.xaxis.label.set_fontsize(axis_font_size)  # Adjust size as needed

ax.set_xticks(x)
ax.set_xticks([(x[1] - x[0])/2, x[2] + ((x[3] - x[2])/2)])  # Center of each group
ax.tick_params(axis='both', which='major', labelsize=20)

ax.set_xticklabels(['10', '100'])
max_height = max(means) + max(std_devs)  # Highest point including error bars
ax.set_ylim(0, max_height * 1.2)  # Extend 20% above the highest point

linux_patch = plt.Rectangle((0, 0), 1, 1, fc=l_color, edgecolor='none')
unikraft_patch = plt.Rectangle((0, 0), 1, 1, fc=u_color, edgecolor='none')
legend = ax.legend([linux_patch, unikraft_patch], ['Linux', 'Unikraft'], 
                   loc='upper right', 
                   prop=FontProperties(size=22))


# Add value labels on top of bars
for rect in rects:
    height = rect.get_height()
    ax.text(rect.get_x() + rect.get_width()/2. - 0.05, height + 0.5,
            f'{height:.1f}',
            ha='right', va='bottom', fontsize=20)  # Adjust size as needed

# Adjust layout and display
plt.tight_layout()
# plt.show()
plt.savefig('tcp.pdf', format="pdf")
