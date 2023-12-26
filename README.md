# Cycle-Accurate-Network-on-Chip-NoC-Router-Simulator

Designed and implemented a Cycle-Accurate Simulator to model a Network-on-Chip (NoC) infrastructure, incorporating two routing algorithms (XY and YX). Leveraged the simulator to analyze the impact of Process Variations on data packet movement within a network of routers.

### **Prerequisites**

- Before running the simulation, make sure you have the following:
    
    → C++ compiler installed on your system.
    

**How to Use**

**Compilation:**

- Compile the program using a C++ compiler. For example, using g++:
    
    ```jsx
    → g++ -o Noc Noc.cpp
    ```
    

**Execution:**

- Run the compiled program with the routing type as a command-line argument. For example, for XY routing:
    
    ```jsx
    → ./Noc XY
    // OR
    → ./Noc YX
    ```
