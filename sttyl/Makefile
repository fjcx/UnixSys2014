# CSCI E-28 HW3
# author: Frank O'Connor
# Make file for sttyl program

CC = gcc
CFLAGS = -Wall
PNAME = sttyl
OBJECTS = $(PNAME).o

$(PNAME): $(OBJECTS) 
	$(CC) -o $(PNAME) $(OBJECTS) 

$(PNAME).o: sttyl.c
	$(CC) -c $(CFLAGS) $(PNAME).c 

clean:
	rm -f *.o core $(PNAME)

