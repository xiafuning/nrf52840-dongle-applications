#Makefile for similar triangles verification
#http://man7.org/linux/man-pages/man1/g++.1.html

IDIR_DEP	:= 
LIB_DIR		:=
TARGETS		:= usb_comm_out
OUTPUT_DIR	:= build

CC 			:= g++
CFLAGS 		:= -g -Wall -std=c++11 -lpthread -Werror
SRC 		:= usb_communication

$(OUTPUT_DIR)/$(TARGETS) : $(OUTPUT_DIR)/$(TARGETS).o
	$(CC) $^ -o $@ $(CFLAGS)
	@echo "Linking complete!"

$(OUTPUT_DIR)/$(TARGETS).o : $(OUTPUT_DIR)/$(TARGETS).s
	$(CC) -c $^ -o $@ $(CFLAGS)
	@echo "Assembly complete!"

$(OUTPUT_DIR)/$(TARGETS).s : $(OUTPUT_DIR)/$(TARGETS).i
	$(CC) -S $^ -o $@ $(CFLAGS)
	@echo "Compilation complete!"

$(OUTPUT_DIR)/$(TARGETS).i : $(SRC).c
	@mkdir -p $(OUTPUT_DIR)
	$(CC) -E $^ -o $@ $(CFLAGS)
	@echo "Pre-processing complete!"

clean:
	rm -rf $(OUTPUT_DIR)
