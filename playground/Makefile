all: ../build/libgpc.a
	gcc -o haversine.exe ../build/libgpc.a haversine.c -Wall

../build/libgpc.a:
	make -C ..

clean:
	rm haversine.exe
