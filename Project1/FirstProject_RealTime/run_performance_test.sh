#!/bin/bash

echo "=========================================="
echo "Matrix Operations Performance Test"
echo "=========================================="
echo ""
echo "This script will test the performance of:"
echo "1. Single-threaded execution"
echo "2. OpenMP parallel execution"
echo "3. Multiprocessing execution"
echo ""
echo "Creating test matrices..."
echo ""

# Create larger test matrices for better performance comparison
cat > /home/ali/university/RealTime/Project1/FirstProject_RealTime/matrices/Large_A.txt << 'EOF'
Large_A
50 50
EOF

# Generate 50x50 matrix with random-like values
for i in {1..50}; do
    for j in {1..50}; do
        echo -n "$((i * 50 + j)) " >> /home/ali/university/RealTime/Project1/FirstProject_RealTime/matrices/Large_A.txt
    done
    echo "" >> /home/ali/university/RealTime/Project1/FirstProject_RealTime/matrices/Large_A.txt
done

cat > /home/ali/university/RealTime/Project1/FirstProject_RealTime/matrices/Large_B.txt << 'EOF'
Large_B
50 50
EOF

for i in {1..50}; do
    for j in {1..50}; do
        echo -n "$((2500 - (i * 50 + j))) " >> /home/ali/university/RealTime/Project1/FirstProject_RealTime/matrices/Large_B.txt
    done
    echo "" >> /home/ali/university/RealTime/Project1/FirstProject_RealTime/matrices/Large_B.txt
done

echo "✓ Created Large_A (50x50) and Large_B (50x50)"
echo ""
echo "Now running the program..."
echo "You can test options 10, 11, or 12 with these large matrices to see performance differences."
echo ""
echo "Try:"
echo "  1. Load matrices from folder (option 6)"
echo "  2. Add Large_A + Large_B (option 10)"
echo "  3. Multiply Large_A × Large_B (option 12)"
echo ""
echo "Press any key to start the program..."
read -n 1

./menu_demo_v2
