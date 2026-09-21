/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *
  ****** SSLSocket example, server code (fork-based variant)
  *
  *  Same protocol/behavior as SSLServer.cc, but services each connection
  *  in a forked child process instead of a std::thread -- covers the
  *  "debe funcionar con procesos" requirement of Trabajo en clase # 09.
  *
  *  Compile with: -lssl -lcrypto
  *
 **/

#include <cstdlib>	// atoi, exit
#include <cstdio>	// printf
#include <cstring>	// strlen, strcmp
#include <unistd.h>	// fork
#include <signal.h>	// signal, SIGCHLD
#include "SSLSocket.h"

#define PORT	4321


void Service( SSLSocket * client ) {

   char buf[ 1024 ] = { 0 };
   int bytes;
   const char* ServerResponse="\n<Body>\n\
\t<Server>os.ecci.ucr.ac.cr</Server>\n\
\t<dir>ci0123</dir>\n\
\t<Name>Proyecto Integrador Redes y sistemas Operativos</Name>\n\
\t<NickName>PIRO</NickName>\n\
\t<Description>Consolidar e integrar los conocimientos de redes y sistemas operativos</Description>\n\
\t<Author>profesores PIRO</Author>\n\
</Body>\n";
   const char *validMessage = "\n<Body>\n\
\t<UserName>piro</UserName>\n\
\t<Password>ci0123</Password>\n\
</Body>\n";

   client->Accept();       // SSL handshake (server side)
   client->ShowCerts();

   bytes = client->Read( buf, sizeof( buf ) );
   buf[ bytes ] = '\0';
   printf( "[pid %d] Client msg: \"%s\"\n", getpid(), buf );

   if ( ! strcmp( validMessage, buf ) ) {
      client->Write( ServerResponse, strlen( ServerResponse ) );
   } else {
      client->Write( "Invalid Message", strlen( "Invalid Message" ) );
   }

   client->Close();

}


int main( int cuantos, char ** argumentos ) {

   SSLSocket * server, * client;
   int port = PORT;
   pid_t pid;

   if ( cuantos > 1 ) {
      port = atoi( argumentos[ 1 ] );
   }

   signal( SIGCHLD, SIG_IGN );   // auto-reap finished children, avoid zombies

   server = new SSLSocket( (const char *) "ci0123.pem", (const char *) "key0123.pem", false );
   server->Bind( port );
   server->MarkPassive( 10 );

   for ( ; ; ) {

      // Plain TCP accept only -- no SSL context attached yet.
      // The handshake happens after fork(), inside whichever process
      // (here, the child) will actually own this connection.
      client = (SSLSocket *) server->AcceptConnection();

      pid = fork();

      if ( -1 == pid ) {

         printf( "fork() failed\n" );
         client->Close();
         delete client;

      } else if ( 0 == pid ) {

         // Child: services only this one connection, then exits
         server->VSocket::Close();   // release the listening socket in the
                                      // child; does NOT touch the shared
                                      // SSL_CTX (that call is the base
                                      // VSocket::Close, bypassing
                                      // SSLSocket::Close's SSL_CTX_free)

         client->Copy( server );     // share the SSL_CTX, build this
                                      // connection's own SSL object
         Service( client );

         delete client;
         exit( 0 );

      } else {

         // Parent: doesn't service this connection, keeps accepting more
         client->Close();
         delete client;

      }

   }

}
