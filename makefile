
MAKE_DIR =	proto		\
		core		\
		src/common	\
		src/tcpsvr	\
		src/proxysvr	\
		src/authsvr	\
		src/hallsvr	\
		src/gamesvr	\
		src/dbproxy	\
		src/client	\

default: debug
debug:
	for dir in $(MAKE_DIR);	\
	do			\
		make -j2 -C $$dir;	\
	done

clean:
	for dir in $(MAKE_DIR);		\
	do				\
		make clean -C $$dir;	\
	done

proto:
	make -j2 -C proto

core:
	make -j2 -C core

.PHONY: debug clean proto core
