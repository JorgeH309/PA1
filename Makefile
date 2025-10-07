# Compiler and flags
WARNING = -Wall -Wshadow --pedantic
ERROR = -Wvla
CXX = g++ -std=c++17 -g $(WARNING) $(ERROR)
VAL = valgrind --tool=memcheck --log-file=memcheck.txt --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose

# Source and object files
SRCS = main.cpp
OBJS = $(SRCS:%.cpp=%.o)

# Target executable
TARGET = pa1

# Default build
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET)



# Compile .cpp -> .o
.cpp.o:
	$(CXX) -c $< -o $@

# Convenience: run program with sample inputs
run0: $(TARGET)
	./$(TARGET) 3e-10 ./examples/inv.param ./examples/wire.param ./examples/5.txt out1.pre out2 out3 out4
	diff out2 ./examples/5.elmore

run1: $(TARGET)
	./$(TARGET) 10 ./examples/fake_inv.param ./examples/fake_wire.param ./examples/3.txt out1.pre out2 out3 out4
	diff out2 ./examples/3.elmore

# Memory check
testmemory: $(TARGET)
	$(VAL) ./$(TARGET) 10 ./examples/fake_inv.param ./examples/fake_wire.param ./examples/3.txt out1.pre out2 out3 out4

# Debug with gdb
gdb: $(TARGET)
	gdb --args ./$(TARGET) 1.5e-9 ./examples/inv.param ./examples/wire.param ./examples/5.txt out1.pre out2 out3 out4

# Clean generated files
clean:
	rm -f $(TARGET) *.o out* memcheck.txt *~
