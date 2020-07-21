#Makefile for simple_forwarding_simulation
#http://man7.org/linux/man-pages/man1/g++.1.html

OUTPUT_DIR	:= build
TARGETS		:= simple_forwarding_simulation

CC			:= g++
CFLAGS		:= -g3 -Wall -std=c++11 -lpthread

SRC_FILES	+= $(TARGETS).cpp
OBJS		:= $(patsubst %.cpp, %.o, $(notdir $(SRC_FILES)))

$(TARGETS) : $(OBJS)
	$(CC) $^ -o $(OUTPUT_DIR)/$@ $(CFLAGS)
	@mv *.o $(OUTPUT_DIR)
	@echo "Linking complete!"

$(OBJS) : $(SRC_FILES)
	@mkdir -p $(OUTPUT_DIR)
	$(CC) -c $^ $(CFLAGS)
	@echo "Compiling and assembly complete!"

clean:
	rm -rf $(OUTPUT_DIR)
