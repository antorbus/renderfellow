TARGETMAIN = main
TARGETLIBS = utils
CC = gcc
CFLAGS = -g -Wall -Wextra 
CINCLUDEFLAG = -I/opt/homebrew/include/SDL2 -D_THREAD_SAFE -L/opt/homebrew/lib -lSDL2 -I/opt/homebrew/include 
ARG = 

	

all:
	@echo "Compiling"
	$(CC) -o $(TARGETMAIN) $(TARGETMAIN).c $(TARGETLIBS).c $(CFLAGS)  $(CINCLUDEFLAG) 

run:
	@echo "Compiling and running"
	$(CC) -o $(TARGETMAIN) $(TARGETMAIN).c $(TARGETLIBS).c $(CFLAGS) $(CINCLUDEFLAG) 
	./$(TARGETMAIN) $(ARG)
