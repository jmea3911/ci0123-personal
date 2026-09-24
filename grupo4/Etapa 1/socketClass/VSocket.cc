/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  ****** VSocket base class implementation
  *
  * (Fedora version)
  *
 **/

#include <sys/socket.h>
#include <arpa/inet.h>		// ntohs, htons
#include <stdexcept>            // runtime_error
#include <cstring>		// memset
#include <netdb.h>		// getaddrinfo, freeaddrinfo
#include <unistd.h>		// close
/*
#include <cstddef>
#include <cstdio>

//#include <sys/types.h>
*/
#include "VSocket.h"


/**
  *  Class creator (constructor)
  *     use Unix socket system call
  *
  *  @param     char t: socket type to define
  *     's' for stream
  *     'd' for datagram
  *  @param     bool ipv6: if we need a IPv6 socket
  *
 **/
void VSocket::Init( char t, bool IPv6 ){

   int st = -1;
   int family = AF_INET;
   int socketType = SOCK_STREAM;


   if ( IPv6 ) {
      family = AF_INET6;
   }
   if ( 'd' == t ) {
      socketType = SOCK_DGRAM;
   }

   st = socket( family, socketType, 0 );
   if ( -1 == st ) {
      throw std::runtime_error( "VSocket::BuildSocket, (reason)" );
   }
   this->sockId = st;
   this->type = t;
   this->IPv6 = IPv6;
   this->port = 0;

}


/**
  * Class destructor
  *
 **/
VSocket::~VSocket() {

   this->Close();

}


/**
  * Close method
  *    use Unix close system call (once opened a socket is managed like a file in Unix)
  *
 **/
void VSocket::Close(){
   if ( -1 == this->sockId ) return;  
   int st = -1;

   st = close( this->sockId );
   this->sockId = -1;

   if ( -1 == st ) {
      throw std::runtime_error( "VSocket::Close()" );
   }
}

/**
  * TryToConnect method
  *   use "connect" Unix system call
  *
  * @param      char * host: host address in dot notation, example "10.84.166.62"
  * @param      int port: process address, example 80
  *
 **/
int VSocket::TryToConnect( const char * hostip, int port ) {

   int st = -1;
   struct sockaddr_in host4;
   struct sockaddr_in6 host6;
   struct sockaddr * ha;
   socklen_t len;



   if ( this->IPv6 ) {
      memset( &host6, 0, sizeof( host6 ) );
      host6.sin6_family = AF_INET6;
      st = inet_pton( AF_INET6, hostip, &host6.sin6_addr );
      host6.sin6_port = htons( port );
      ha = (struct sockaddr *) &host6;
      len = sizeof( host6 );
   } else {
      memset( &host4, 0, sizeof( host4 ) );
      host4.sin_family = AF_INET;
      st = inet_pton( AF_INET, hostip, &host4.sin_addr );
      host4.sin_port = htons( port );
      ha = (struct sockaddr *) &host4;
      len = sizeof( host4 );
   }

   if ( 1 != st ) {	// 0 means invalid address, -1 means address error
      throw std::runtime_error( "VSocket::TryToConnect( const char *, int ) [inet_pton]" );
   }

   st = connect( this->sockId, ha, len );
   if ( -1 == st ) {
      throw std::runtime_error( "VSocket::TryToConnect" );
   }
   this->port = port;
   return st;

}


/**
  * TryToConnect method
  *   use "connect" Unix system call
  *
  * @param      char * host: host address in dns notation, example "os.ecci.ucr.ac.cr"
  * @param      char * service: process address, example "http"
  *
 **/

int VSocket::TryToConnect( const char *host, const char *service ) {
   int st = -1;
   struct addrinfo hints, *result, *rp;

   memset( &hints, 0, sizeof( struct addrinfo ) );
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = 0;
   hints.ai_protocol = 0;

   st = getaddrinfo( host, service, &hints, &result );
   if ( 0 != st ) {
      throw std::runtime_error( "VSocket::TryToConnect [getaddrinfo]" );
   }

   st = -1;
   for ( rp = result; rp; rp = rp->ai_next ) {
      st = connect( this->sockId, rp->ai_addr, rp->ai_addrlen );
      if ( 0 == st )
         break;
   }

   freeaddrinfo( result );

   if ( 0 != st ) {
      throw std::runtime_error( "VSocket::TryToConnect [connect]" );
   }

   return st;

}


/**
  * Bind method
  *    use "bind" Unix system call (man 3 bind) (server mode)
  *
  * @param      int port: bind a unamed socket to a port defined in sockaddr structure
  *
  *  Links the calling process to a service at port
  *
 **/
int VSocket::Bind( int port ) {
   int st = -1;

   return st;

}


/**
  *  sendTo method
  *
  *  @param	const void * buffer: data to send
  *  @param	size_t size data size to send
  *  @param	void * addr address to send data
  *
  *  Send data to another network point (addr) without connection (Datagram)
  *
 **/
size_t VSocket::sendTo( const void * buffer, size_t size, void * addr ) {
   int st = -1;

   return st;

}


/**
  *  recvFrom method
  *
  *  @param	const void * buffer: data to send
  *  @param	size_t size data size to send
  *  @param	void * addr address to receive from data
  *
  *  @return	size_t bytes received
  *
  *  Receive data from another network point (addr) without connection (Datagram)
  *
 **/
size_t VSocket::recvFrom( void * buffer, size_t size, void * addr ) {
   int st = -1;

   return st;

}

