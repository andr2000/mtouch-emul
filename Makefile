all:
	${CC} -o mtouch-emul mtouch-emul.c
clean:
	rm -rf mtouch-emul *.o
