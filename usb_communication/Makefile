#Makefile for usb_communication
#http://man7.org/linux/man-pages/man1/g++.1.html

SRC_DIR		:= src
INC_DIR		:= include
LIB_DIR		:= lib
OUTPUT_DIR	:= build

CC			:= g++
OPT			:= -g3 -O3
CFLAGS		+= $(OPT)
CFLAGS		+= -Wall -std=c++14 -lpthread -fpermissive
CFLAGS		+= -ftree-vectorize

SRC_FILES	+= $(wildcard ./$(SRC_DIR)/*.c)
SRC_FILES	+= $(TARGETS).c
LIB_FILES	+= -lkodo_rlnc
OBJS		:= $(patsubst %.c, %.o, $(notdir $(SRC_FILES)))

$(TARGETS) : $(OBJS)
	$(CC) $^ -o $(OUTPUT_DIR)/$@ $(CFLAGS) -I $(INC_DIR) -L $(LIB_DIR) $(LIB_FILES)
	@mv *.o $(OUTPUT_DIR)
	@echo "Linking complete!"

$(OBJS) : $(SRC_FILES)
	@mkdir -p $(OUTPUT_DIR)
	$(CC) -c $^ $(CFLAGS) -I $(INC_DIR)
	@echo "Compiling and assembly complete!"

clean:
	rm -rf $(OUTPUT_DIR)
	rm -f *.o
