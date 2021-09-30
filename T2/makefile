#
# Makefile ESQUELETO
#
# DEVE ter uma regra "all" para geração da biblioteca
# regra "clean" para remover todos os objetos gerados.
#
# NECESSARIO adaptar este esqueleto de makefile para suas necessidades.
#
#

CC=gcc
LIB_DIR=./lib
INC_DIR=./include
BIN_DIR=./bin
SRC_DIR=./src
EXEMPLO_DIR=./exemplo

all: t2fs.o libt2fs.a

geral:
					$(CC) -o main $(SRC_DIR)/t2fs.c $(LIB_DIR)/apidisk.o $(LIB_DIR)/bitmap2.o -Wall

t2fs.o:  	$(SRC_DIR)/t2fs.c $(INC_DIR)/t2fs.h $(INC_DIR)/apidisk.h $(INC_DIR)/bitmap2.h
					$(CC) -c $(SRC_DIR)/t2fs.c -Wall
					mv t2fs.o $(BIN_DIR)/t2fs.o

libt2fs.a: 	$(BIN_DIR)/t2fs.o $(LIB_DIR)/apidisk.o $(LIB_DIR)/bitmap2.o
						ar crs $(LIB_DIR)/libt2fs.a  $(BIN_DIR)/*.o $(LIB_DIR)/*.o

exemplo: 	$(BIN_DIR)/t2fs.o $(LIB_DIR)/apidisk.o $(LIB_DIR)/bitmap2.o
						ar crs $(EXEMPLO_DIR)/libt2fs.a  $(BIN_DIR)/*.o $(LIB_DIR)/*.o

clean:
			rm -rf $(LIB_DIR)/*.a $(BIN_DIR)/*.o $(SRC_DIR)/~ $(INC_DIR)/*~ *~
