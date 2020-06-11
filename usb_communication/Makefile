#Makefile for usb_communication
#http://man7.org/linux/man-pages/man1/g++.1.html

SRC_DIR		:= src
INC_DIR		:= include
LIB_DIR		:=
OUTPUT_DIR	:= build

CC			:= g++
CFLAGS		:= -g3 -Wall -std=c++11 -lpthread -Werror

SRC_FILES	+= $(wildcard ./$(SRC_DIR)/*.c)
SRC_FILES	+= $(TARGETS).c
OBJS		:= $(patsubst %.c, %.o, $(notdir $(SRC_FILES)))

$(TARGETS) : $(OBJS)
	$(CC) $^ -o $(OUTPUT_DIR)/$@ $(CFLAGS) -I $(INC_DIR)
	@mv *.o $(OUTPUT_DIR)
	@echo "Linking complete!"

$(OBJS) : $(SRC_FILES)
	@mkdir -p $(OUTPUT_DIR)
	$(CC) -c $^ $(CFLAGS) -I $(INC_DIR)
	@echo "Compiling and assembly complete!"

clean:
	rm -rf $(OUTPUT_DIR)
