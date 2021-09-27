LIBS=`pkg-config openssl,libmariadb,json-c --libs --cflags`
all:
	gcc main.c db.c -pthread $(LIBS) -o server
clean:
	rm *.pem
	rm server


