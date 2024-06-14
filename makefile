UFS: Nain.o Filesystem.o Components.o
	g++ Nain.o Filesystem.o Components.o -o UFS

Components.o: Components.cpp
	g++ -c Components.cpp

Nain.o: Main.cpp
	g++ -c Main.cpp -o Nain.o

Filesystem.o: Filesystem.cpp
	g++ -c Filesystem.cpp

clean:
	-rm -f *.o