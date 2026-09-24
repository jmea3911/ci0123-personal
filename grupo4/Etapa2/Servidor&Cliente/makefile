CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread
SSLFLAGS = -lssl -lcrypto

TARGETS = make_fileSys servidor cliente fileSys_herr read_fileSys

all: $(TARGETS)

make_fileSys: make_fileSys.cc Bitmap.cc FileSystem.cc Bodega.cc Formatos.h Bitmap.h FileSystem.h Bodega.h
	$(CXX) $(CXXFLAGS) -o $@ make_fileSys.cc Bitmap.cc FileSystem.cc Bodega.cc

servidor: VSocket.cc Socket.cc SSLSocket.cc ServidorProductos.cc Server.cc ServidorProductos.h Bitmap.cc FileSystem.cc Bodega.cc protocolo.cc protocolo.h
	$(CXX) $(CXXFLAGS) -o $@ VSocket.cc Socket.cc SSLSocket.cc ServidorProductos.cc Server.cc Bitmap.cc FileSystem.cc Bodega.cc protocolo.cc $(SSLFLAGS)

cliente: VSocket.cc Socket.cc SSLSocket.cc Client.cc protocolo.cc protocolo.h
	$(CXX) $(CXXFLAGS) -o $@ VSocket.cc Socket.cc SSLSocket.cc Client.cc protocolo.cc $(SSLFLAGS)

fileSys_herr: fileSys_herr.cc Bitmap.cc FileSystem.cc Bodega.cc
	$(CXX) $(CXXFLAGS) -o $@ fileSys_herr.cc Bitmap.cc FileSystem.cc Bodega.cc

read_fileSys: read_fileSys.cc Bitmap.cc FileSystem.cc Bodega.cc
	$(CXX) $(CXXFLAGS) -o $@ read_fileSys.cc Bitmap.cc FileSystem.cc Bodega.cc

#formatea y llena bodega.dat (un solo archivo, con Bodega1 y Bodega2 separadas)
fileSys: make_fileSys
	./make_fileSys

clean:
	rm -f $(TARGETS) *.o

distclean: clean
	rm -f bodega.dat

.PHONY: all fileSys clean distclean