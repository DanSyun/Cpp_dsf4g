PRO_DIR=..
-include $(PRO_DIR)/makefile.dep

OBJ_DIR=./obj_dep
LIB_DIR=$(PRO_DIR)/lib
$(shell mkdir -p $(OBJ_DIR))
$(shell mkdir -p $(LIB_DIR))

BIN=$(LIB_DIR)/libproto.a
#OBJ=$(patsubst %.proto, $(OBJ_DIR)/%.pb.o, $(wildcard *.proto))
#按顺序编译
OBJ=	$(OBJ_DIR)/pb_common.o		\
	$(OBJ_DIR)/pb_svr_config.o	\
	$(OBJ_DIR)/pb_config.o		\
	$(OBJ_DIR)/pb_cs_table.o	\
	$(OBJ_DIR)/pb_cs_msg.o		\
	$(OBJ_DIR)/pb_ss_msg.o		\
	$(OBJ_DIR)/pb_inter_msg.o	\
	$(OBJ_DIR)/pb_db_data.o		\
	$(OBJ_DIR)/pb_object.o		\

$(BIN) : $(OBJ)
	ar rcs $@ $^

$(OBJ) : $(OBJ_DIR)/%.o : %.proto
	$(PROTOC) --cpp_out=./ $<
	g++ -c $(INC) $(CXXFLAGS) $(patsubst %.proto, %.pb.cc, $<) -o $@

clean:
	rm -rf $(BIN) $(OBJ_DIR) *.pb.cc *.pb.h

