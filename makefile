all : clean compile run

compile : 
	@gcc -o hw2 HW2.c
run : 
	./hw2 10
clean :
	@rm -f *.o
	@rm -f hw2
	
	
