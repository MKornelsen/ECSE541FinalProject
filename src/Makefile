C_FILES = eie_main.cpp 
INCLUDE_PATHS = -I. -I../include -I$(SYSTEMC_HOME)/include
LINKER_PATHS = -L. -L$(SYSTEMC_HOME)/lib-linux64
LINKER_ARGUMENTS = -lsystemc -lm

EXEC_NAME = Proj_exec

all: 
	g++ $(INCLUDE_PATHS) $(LINKER_PATHS) -o $(EXEC_NAME) $(C_FILES) $(LINKER_ARGUMENTS) 

clean: 
	rm -rf $(EXEC_NAME)
