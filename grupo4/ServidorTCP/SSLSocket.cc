/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *
  ****** SSLSocket class implementation
  *
  *  Built for Trabajo en clase # 09.
  *  Compile with: -lssl -lcrypto
  *
 **/

#include <cstring>     // strlen
#include <cstdio>      // printf
#include <cstdlib>     // free
#include <stdexcept>   // runtime_error

#include <openssl/err.h>

#include "SSLSocket.h"

bool SSLSocket::sslLibReady = false;


/**
  *  Client-side constructor
  *     Builds a plain TCP socket (base classes) and a client SSL_CTX
 **/
SSLSocket::SSLSocket( bool IPv6 ) : Socket( 's', IPv6 ) {

   this->ctx = nullptr;
   this->ssl = nullptr;
   this->isServer = false;
   this->ownsContext = false;

   this->Init();
   this->InitContext();
   this->ownsContext = true;

}


/**
  *  Server-side constructor
  *     Builds a plain TCP socket (base classes) and a server SSL_CTX
  *     already loaded with the given certificate/key pair
  *
  *  @param   const char * certFile: path to the PEM certificate (e.g. "ci0123.pem")
  *  @param   const char * keyFile:  path to the PEM private key  (e.g. "key0123.pem")
 **/
SSLSocket::SSLSocket( const char * certFile, const char * keyFile, bool IPv6 ) : Socket( 's', IPv6 ) {

   this->ctx = nullptr;
   this->ssl = nullptr;
   this->isServer = true;
   this->ownsContext = false;

   this->Init();
   this->InitServer( certFile, keyFile );
   this->ownsContext = true;

}


/**
  *  Constructor from an already-open descriptor
  *     Used by AcceptConnection() right after "accept". No SSL context
  *     is attached yet: the caller must call Copy() (to share the
  *     server's SSL_CTX) and then Accept() (to run the SSL handshake)
  *     before Read/Write/Close are usable.
 **/
SSLSocket::SSLSocket( int id ) : Socket( id ) {

   this->ctx = nullptr;
   this->ssl = nullptr;
   this->isServer = true;
   this->ownsContext = false;

}


SSLSocket::~SSLSocket() {

   this->Close();

}


/**
  *  Init method
  *     One-time OpenSSL library setup. Safe to call from every
  *     constructor: only runs once per process.
 **/
void SSLSocket::Init() {

   if ( sslLibReady ) return;

   SSL_library_init();
   SSL_load_error_strings();
   OpenSSL_add_all_algorithms();

   sslLibReady = true;

}


/**
  *  InitContext method
  *     Builds a client-role SSL_CTX
 **/
void SSLSocket::InitContext() {

   this->ctx = SSL_CTX_new( TLS_client_method() );
   if ( nullptr == this->ctx ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::InitContext" );
   }

}


/**
  *  InitServerContext method
  *     Builds a server-role SSL_CTX
 **/
void SSLSocket::InitServerContext() {

   this->ctx = SSL_CTX_new( TLS_server_method() );
   if ( nullptr == this->ctx ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::InitServerContext" );
   }

}


/**
  *  InitServer method
  *     InitServerContext() plus loading the certificate/private key pair
  *
  *  @param   const char * certFile: path to the PEM certificate
  *  @param   const char * keyFile:  path to the PEM private key
 **/
void SSLSocket::InitServer( const char * certFile, const char * keyFile ) {

   this->InitServerContext();

   if ( SSL_CTX_use_certificate_file( this->ctx, certFile, SSL_FILETYPE_PEM ) <= 0 ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::InitServer [certificate]" );
   }

   if ( SSL_CTX_use_PrivateKey_file( this->ctx, keyFile, SSL_FILETYPE_PEM ) <= 0 ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::InitServer [private key]" );
   }

   if ( ! SSL_CTX_check_private_key( this->ctx ) ) {
      throw std::runtime_error( "SSLSocket::InitServer [key/cert mismatch]" );
   }

}


/**
  *  Copy method
  *     Shares "original"'s SSL_CTX (does not take ownership) and builds
  *     a fresh SSL object bound to this socket's descriptor. Used right
  *     after AcceptConnection() returns a bare (ctx-less) SSLSocket --
  *     matches the reference SSLServer.cc, which calls client->Copy(server).
  *
  *  @param   SSLSocket * original: socket whose SSL_CTX (cert/key already
  *           loaded -- typically the listening server socket) is reused
 **/
void SSLSocket::Copy( SSLSocket * original ) {

   this->ctx = original->ctx;
   this->ownsContext = false;      // borrowed: do not free on Close()
   this->isServer = original->isServer;

   this->ssl = SSL_new( this->ctx );
   if ( nullptr == this->ssl ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::CopyContext" );
   }

   if ( 0 == SSL_set_fd( this->ssl, this->sockId ) ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::CopyContext [SSL_set_fd]" );
   }

}


/**
  *  Connect method (dot-notation address)
  *     TCP connect (base class TryToConnect) followed by the SSL/TLS
  *     client handshake
 **/
int SSLSocket::Connect( const char * hostip, int port ) {

   int st = this->TryToConnect( hostip, port );

   this->ssl = SSL_new( this->ctx );
   if ( nullptr == this->ssl ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::Connect" );
   }

   SSL_set_fd( this->ssl, this->sockId );

   if ( SSL_connect( this->ssl ) <= 0 ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::Connect [SSL_connect]" );
   }

   return st;

}


/**
  *  Connect method (DNS/service name)
 **/
int SSLSocket::Connect( const char * host, const char * service ) {

   int st = this->TryToConnect( host, service );

   this->ssl = SSL_new( this->ctx );
   if ( nullptr == this->ssl ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::Connect" );
   }

   SSL_set_fd( this->ssl, this->sockId );

   if ( SSL_connect( this->ssl ) <= 0 ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::Connect [SSL_connect]" );
   }

   return st;

}


/**
  *  Accept method
  *     Runs the server-side SSL/TLS handshake (SSL_accept) on a socket
  *     that already has ssl/ctx set up via CopyContext()
 **/
SSLSocket * SSLSocket::Accept() {

   int st = SSL_accept( this->ssl );
   if ( st <= 0 ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::Accept [SSL_accept]" );
   }

   return this;

}


/**
  *  AcceptConnection method
  *     TCP-level accept (base class WaitForConnection), wraps the new
  *     descriptor in a bare SSLSocket. The SSL handshake itself happens
  *     later, in the worker (thread or, after fork, child process), via
  *     Copy() + Accept() -- this keeps the (possibly slow)
  *     handshake off the single accept() loop.
 **/
VSocket * SSLSocket::AcceptConnection() {

   int id = this->WaitForConnection();

   return new SSLSocket( id );

}


/**
  *  Read method
  *     use SSL_read instead of the plain "read" system call
 **/
size_t SSLSocket::Read( void * buffer, size_t size ) {

   int st = SSL_read( this->ssl, buffer, size );
   if ( st < 0 ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::Read" );
   }

   return st;

}


/**
  *  Write method
  *     use SSL_write instead of the plain "write" system call
 **/
size_t SSLSocket::Write( const void * buffer, size_t size ) {

   int st = SSL_write( this->ssl, buffer, size );
   if ( st < 0 ) {
      ERR_print_errors_fp( stderr );
      throw std::runtime_error( "SSLSocket::Write( void *, size_t )" );
   }

   return st;

}


size_t SSLSocket::Write( const char * text ) {

   return this->Write( text, strlen( text ) );

}


/**
  *  ShowCerts method
  *     print the peer's certificate subject/issuer, if any
  *     (uses SSL_get_peer_certificate, per the assignment)
 **/
void SSLSocket::ShowCerts() {

   X509 * cert = SSL_get_peer_certificate( this->ssl );

   if ( nullptr == cert ) {
      printf( "No peer certificate presented\n" );
      return;
   }

   char * line;

   line = X509_NAME_oneline( X509_get_subject_name( cert ), 0, 0 );
   printf( "Certificate subject: %s\n", line );
   free( line );

   line = X509_NAME_oneline( X509_get_issuer_name( cert ), 0, 0 );
   printf( "Certificate issuer:  %s\n", line );
   free( line );

   X509_free( cert );

}


/**
  *  GetCipher method
  *     negotiated cipher suite name for this connection
  *     (uses SSL_get_cipher, per the assignment)
 **/
const char * SSLSocket::GetCipher() {

   return SSL_get_cipher( this->ssl );

}


/**
  *  Close method
  *     graceful SSL shutdown, release the SSL objects (and the SSL_CTX,
  *     only if this instance created it), then close the underlying TCP
  *     descriptor via the base class
 **/
void SSLSocket::Close() {

   if ( this->ssl ) {
      SSL_shutdown( this->ssl );
      SSL_free( this->ssl );        // also detaches from sockId
      this->ssl = nullptr;
   }

   if ( this->ctx && this->ownsContext ) {
      SSL_CTX_free( this->ctx );
      this->ctx = nullptr;
   }

   VSocket::Close();                 // closes this->sockId (already guards -1)

}