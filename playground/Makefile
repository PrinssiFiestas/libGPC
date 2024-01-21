all: ../build/libgpc.a
	gcc haversine.c ../build/libgpc.a -Wall -I../include/ -o haversine.exe

../build/libgpc.a:
	make -C ..

clean:
	rm haversine.exe
