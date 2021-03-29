TOP=	.
TITLE=auxtool
SRC_DIR= $(TOP)/auxtool
BIN_DIR= $(TOP)/bin
CC=cc
INCLUDES=
LIBS=-lm
CFLAG=
MAKEFILE= Makefile
AR=ar r
CFLAGS=$(INCLUDES) $(LIBS) $(CFLAG)

$(TITLE): $(BIN_DIR)/main.o
	$(CC) $(CFLAGS) $(BIN_DIR)/main.o -o $(BIN_DIR)/$(TITLE)

$(BIN_DIR)/main.o: $(BIN_DIR)
	$(CC) -c $(SRC_DIR)/main.c -o $(BIN_DIR)/main.o

$(BIN_DIR):
	mkdir $(BIN_DIR)

all: $(TITLE)

clean:
	rm -f $(BIN_DIR)

clean_temp:
	rm -f $(BIN_DIR)/*.o

