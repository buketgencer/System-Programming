all : clear compile run clean

clear:
	@clear 

clean:
	@rm -f *.o
	@rm -f *.out

compile:
	@gcc -g -c stuManSis.c
	@gcc -g -o stuManSis.out stuManSis.o

run:
	@./stuManSis.out

#valgrind:
#	@valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./stuManSis.out
#all : clear compile run valgrind clean  
#if you want to use valgrind, you can add valgrind in the all command
