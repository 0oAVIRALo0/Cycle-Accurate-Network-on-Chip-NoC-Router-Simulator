import matplotlib.pyplot as plt

# Read data from file
with open('output.txt', 'r') as f:
    lines = f.readlines()

# Parse data
ID = []
Cycle = []
for line in lines:
    id, cycle = map(int, line.split())
    ID.append(id)
    Cycle.append(cycle)

# Create bar graph
plt.bar(ID, Cycle)

# Label axes
plt.xlabel('ID')
plt.ylabel('Cycle')

# Show the graph
plt.show()
