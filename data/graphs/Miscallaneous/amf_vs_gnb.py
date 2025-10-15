import matplotlib.pyplot as plt

# Read the y-values from the amf.txt file
with open('amf.txt', 'r') as file:
    y_values = [int(line.strip()) for line in file]

# Generate x-values based on the number of y-values
x_values = list(range(1, len(y_values) + 1))

# Read the time values from the time.txt file (these will be shown on x-axis)
with open('time.txt', 'r') as file:
    time_values = [int(line.strip()) for line in file]

# Plot the y-values against time values
plt.plot(time_values, y_values, marker='o', linestyle='-')

# Annotate the points where y changes from 1 to 2 and from 2 to 3
for i in range(1, len(y_values)):
    if y_values[i] != y_values[i - 1]:
        plt.annotate(f'gnb_count = {x_values[i]}', 
                     xy=(time_values[i], y_values[i]), 
                     xytext=(time_values[i] + 1, y_values[i] - 0.2),
                     arrowprops=dict(facecolor='black', shrink=0.05))

# Customize the y-axis to only show 1, 2, and 3
plt.yticks([1, 2, 3])

# Set the labels and title
plt.xlabel('Time')
plt.ylabel('AMFs')
plt.title('AMF vs gNB')

# Show the plot
plt.show()

