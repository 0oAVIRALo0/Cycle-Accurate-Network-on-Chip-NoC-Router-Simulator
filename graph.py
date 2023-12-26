import matplotlib.pyplot as plot
from matplotlib.ticker import MaxNLocator

with open("data.txt", 'r') as file:
    for line in file:
        arr = line.split()
        if(arr[0] == "Connections"):
            Connections = arr[1:]
        elif(arr[0] == "FlitCount"):
            FlitCount = [int(i) for i in arr[1:]]
    plot.bar(Connections, FlitCount, 0.5)
    plot.xlabel("Connections")
    plot.ylabel("Number of flits that passed through")
    plot.title("Flit count for various connections")
    plot.gca().yaxis.set_major_locator(MaxNLocator(integer=True))
    plot.show()