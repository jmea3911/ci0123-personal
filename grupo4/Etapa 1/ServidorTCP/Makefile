# Makefile
# Builds everything in ServidorTCP in one shot:
#   - ForkMirrorServer.out / ThreadMirrorServer.out / MirrorClient.out  (Tarea 7)
#   - sslserver / sslclient                                            (Trabajo en clase 9)
#
# Works on Linux as-is. On macOS, auto-detects Homebrew's openssl@3 paths.
#

CC  := gcc
CXX := g++
CFLAGS   := -g -c
CXXFLAGS := -g -c -std=c++11
LDFLAGS  :=
SSL_LIBS := -lssl -lcrypto
THREAD_FLAGS := -pthread

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    OPENSSL_PREFIX := $(shell brew --prefix openssl@3)
    CXXFLAGS += -I$(OPENSSL_PREFIX)/include
    LDFLAGS  += -L$(OPENSSL_PREFIX)/lib
endif

DEPS := VSocket.h Socket.h SSLSocket.h

TARGETS := ForkMirrorServer.out ThreadMirrorServer.out MirrorClient.out sslserver sslclient sslforkserver

all: $(TARGETS)

%.o: %.cc $(DEPS)
	$(CXX) $(CXXFLAGS) $< -o $@

ForkMirrorServer.out: ForkMirrorServer.o VSocket.o Socket.o
	$(CXX) $^ $(LDFLAGS) -o $@

ThreadMirrorServer.out: ThreadMirrorServer.o VSocket.o Socket.o
	$(CXX) $^ $(LDFLAGS) $(THREAD_FLAGS) -o $@

MirrorClient.out: MirrorClient.o VSocket.o Socket.o
	$(CXX) $^ $(LDFLAGS) -o $@

sslserver: SSLServer.o VSocket.o Socket.o SSLSocket.o
	$(CXX) $^ $(LDFLAGS) $(THREAD_FLAGS) -o $@ $(SSL_LIBS)

sslclient: SSLClient.o VSocket.o Socket.o SSLSocket.o
	$(CXX) $^ $(LDFLAGS) $(THREAD_FLAGS) -o $@ $(SSL_LIBS)

sslforkserver: SSLForkServer.o VSocket.o Socket.o SSLSocket.o
	$(CXX) $^ $(LDFLAGS) -o $@ $(SSL_LIBS)

clean:
	rm -f *.o ForkMirrorServer.out ThreadMirrorServer.out MirrorClient.out sslserver sslclient sslforkserver