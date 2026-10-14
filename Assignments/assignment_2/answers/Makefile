CC = gcc
CFLAGS = -g -Wall
LDLIBS = -lpthread -lm

OBJ	= life.o real_rand.o plot.o timer.o

life:	        $(OBJ)
		$(CC) $(LDFLAGS) -o $@ $(OBJ)  $(LDLIBS)

clean:	
	$(RM) *.o life *~;
	$(RM) core;
	$(RM) PI*;
