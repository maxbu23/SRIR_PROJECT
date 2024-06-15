INC = $(wildcard *.h)
SRC = $(wildcard *.cpp)
OBJ = $(SRC:.cpp=.o)
EXE = run

CXX = /opt/nfs/mpich-4.2.0/bin/mpicxx
MPIEXEC = /opt/nfs/mpich-4.2.0/bin/mpiexec
LDFLAGS =  -o $(EXE)
MPIFLAGS = -f nodes -n 4
CPPFLAGS = -Wall -ggdb -std=c++17

$(EXE): $(OBJ)
	$(CXX) $(LDFLAGS) $(OBJ)
$(OBJ): $(SRC)

out:
	$(MPIEXEC) $(MPIFLAGS) ./$(EXE)

.PHONY: clean run

clean:
	rm $(OBJ) $(EXE)