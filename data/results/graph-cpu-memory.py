import pandas as pd
import matplotlib.pyplot as plt

# Define the file path for the CSV file
file = 'ip-alloc-200-30/lb_amf_resource.csv'  # Path to the CSV file

# Output directory to save the plot and CSV files
output_dir = 'Graphs/Graphs-200'  # Change to your desired directory
cpu_output_file = f'{output_dir}/cpu-usage-cpu_usage.csv'  # File to store CPU usage data
memory_output_file = f'{output_dir}/memory_usage.csv'  # File to store Memory usage data

# Load the CSV data into a pandas DataFrame
df = pd.read_csv(file)

# Convert the Timestamp column to datetime for better plotting
df['Timestamp'] = pd.to_datetime(df['Timestamp'])

# Normalize the Timestamp to start from 0
df['Timestamp'] = (df['Timestamp'] - df['Timestamp'].iloc[0]).dt.total_seconds()

# Extract the relevant columns for plotting
cpu_usage = df[['Timestamp', 'TOTAL-AMF-CPU']]
memory_usage = df[['Timestamp', 'TOTAL-AMF-MEM']]

# Save CPU usage data to CSV
cpu_usage.to_csv(cpu_output_file, index=False)
print(f"CPU usage data saved to: {cpu_output_file}")

# Save Memory usage data to CSV
memory_usage.to_csv(memory_output_file, index=False)
print(f"Memory usage data saved to: {memory_output_file}")

# Plot 1: Total CPU usage vs Timestamp
plt.figure(figsize=(10, 6))
plt.plot(cpu_usage['Timestamp'], cpu_usage['TOTAL-AMF-CPU'], label='CPU Usage', linestyle='-', markersize=4)
plt.title('Total CPU Usage vs Timestamp')
plt.xlabel('Timestamp (seconds)')
plt.ylabel('Total CPU Usage (m)')
plt.grid(True)
plt.tight_layout()
plt.legend(loc='upper left')

# Save the CPU plot as a PNG file
plt.savefig(f'{output_dir}/cpu-usage/ip-alloc-200-30.png')
plt.close()

# Plot 2: Total Memory usage vs Timestamp
plt.figure(figsize=(10, 6))
plt.plot(memory_usage['Timestamp'], memory_usage['TOTAL-AMF-MEM'], label='Memory Usage', linestyle='-', markersize=4)
plt.title('Total Memory Usage vs Timestamp')
plt.xlabel('Timestamp (seconds)')
plt.ylabel('Total Memory Usage (Mi)')
plt.grid(True)
plt.tight_layout()
plt.legend(loc='upper left')

# Save the Memory plot as a PNG file
plt.savefig(f'{output_dir}/memory-usage/ip-alloc-200-30.png')
plt.close()

print("Plots have been saved successfully.")
