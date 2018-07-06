PRO_DIR=../..
-include $(PRO_DIR)/makefile.dep

OBJ_DIR=./obj_dep
BIN_DIR=./bin
$(shell mkdir -p $(OBJ_DIR))
$(shell mkdir -p $(BIN_DIR))

BIN=$(BIN_DIR)/authsvr
OBJ=$(patsubst %.cpp, $(OBJ_DIR)/%.o, $(wildcard *.cpp))
OBJ+=$(PRO_DIR)/lib/libcommon.a	\
	$(PRO_DIR)/lib/libcore.a

$(BIN) : $(OBJ)
	g++ $(CXXFLAGS) $(INC) $^ -o $@ $(LIB)

clean:
	rm -rf $(BIN) $(OBJ_DIR)

-include $(PRO_DIR)/makefile.rule
