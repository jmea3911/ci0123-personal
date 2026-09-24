/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *  2026-ii
  *  Grupos: 2 y 5
  *
  *  SSL Socket class implementation
  *
  * (Fedora version)
  *
 **/
 
// SSL includes
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <stdexcept>
#include <sys/socket.h>
#include <sys/time.h>

#include "SSLSocket.h"
#include "Socket.h"

/**
  *  Class constructor
  *     use base class
  *
  *  @param     char t: socket type to define
  *     's' for stream
  *     'd' for datagram
  *  @param     bool ipv6: if we need a IPv6 socket
  *
 **/
SSLSocket::SSLSocket( bool IPv6 ) {

   this->Init( 's', IPv6 );

   this->Context = nullptr;
   this->BIO = nullptr;

   this->InitSSL();					// Initializes to client context

}


/**
  *  Class constructor
  *     use base class
  *
  *  @param     char t: socket type to define
  *     's' for stream
  *     'd' for datagram
  *  @param     bool IPv6: if we need a IPv6 socket
  *
 **/
SSLSocket::SSLSocket( char * certFileName, char * keyFileName, bool IPv6 ) {
   
   this->Init('s', IPv6);
   this->Context = nullptr;
   this->BIO = nullptr;
   this->InitSSL(true);
   this->LoadCertificates(certFileName, keyFileName, false);	// false = no se exige certificado del cliente (sin autenticacion mutua)
}


/**
  *  Class constructor
  *
  *  @param     int id: socket descriptor
  *
 **/
SSLSocket::SSLSocket( int id ) {
   this->sockId = id;
   this->Context = nullptr;
   this->BIO = nullptr;
   //this->Init('s', false);
   this->InitSSL(false);
   SSL *ssl = (SSL *) this->BIO;
   SSL_set_fd(ssl, this->sockId);

}


/**
  * Class destructor
  *
 **/
SSLSocket::~SSLSocket() {

// SSL destroy
   if ( nullptr != this->Context ) {
      SSL_CTX_free( reinterpret_cast<SSL_CTX *>( this->Context ) );
   }
   if ( nullptr != this->BIO ) {
      SSL_free( reinterpret_cast<SSL *>( this->BIO ) );
   }

   this->Close();

}


/**
  *  InitSSL
  *     use SSL_new with a defined context
  *
  *  Create a SSL object
  *
 **/
void SSLSocket::InitSSL( bool serverContext ) {
   this->InitContext( serverContext );
   SSL * ssl = SSL_new( reinterpret_cast<SSL_CTX *>( this->Context ) );
   if(!ssl) {
      throw std::runtime_error( "SSLSocket::InitSSL( SSL_new failed )" );
   }
   this->BIO = reinterpret_cast<void *>( ssl );
}


/**
  *  InitContext
  *     use SSL_library_init, OpenSSL_add_all_algorithms, SSL_load_error_strings, TLS_server_method, SSL_CTX_new
  *
  *  Creates a new SSL server context to start encrypted comunications, this context is stored in class instance
  *
 **/
void SSLSocket::InitContext( bool serverContext ) {
   const SSL_METHOD * method = serverContext ? TLS_server_method() : TLS_client_method();
   if ( nullptr == method ) {
      throw std::runtime_error( "SSLSocket::InitContext( bool ):method is null");
   }
  
   SSL_CTX * context = SSL_CTX_new( method );
   if (!context) {
      throw std::runtime_error( "SSLSocket::InitContext( bool ) SSL_CTX_new failed" );
   }

   this->Context = reinterpret_cast<void *>( context );

}


/**
 *  Load certificates
 *    verify and load certificates
 *
 *  @param	const char * certFileName, file containing certificate
 *  @param	const char * keyFileName, file containing keys
 *
 **/
 void SSLSocket::LoadCertificates( const char * certFileName, const char * keyFileName, bool requirePeerCert ) {
   SSL_CTX *ctx = reinterpret_cast<SSL_CTX *>(this->Context);

   // cargar el certificado y la llave propios -- esto SIEMPRE hace falta,
   // sin importar si se le va a pedir certificado al otro extremo o no
   if (SSL_CTX_use_certificate_file(ctx, certFileName, SSL_FILETYPE_PEM) <= 0) {
      ERR_print_errors_fp(stderr);
      throw std::runtime_error("SSLSocket::LoadCertificates(): Error - certificate file");
   }

   if (SSL_CTX_use_PrivateKey_file(ctx, keyFileName, SSL_FILETYPE_PEM) <= 0) {
      ERR_print_errors_fp(stderr);
      throw std::runtime_error("SSLSocket::LoadCertificates(): Error - key file");
   }

   if (!SSL_CTX_check_private_key(ctx)) {
      throw std::runtime_error("SSLSocket::LoadCertificates(): Private key different to the public certificate");
   }

   if ( !requirePeerCert ) {
      // sin autenticacion mutua no se le pide certificado al otro
      return;
   }


   SSL_CTX_set_verify( ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr );

   if ( SSL_CTX_load_verify_locations( ctx, certFileName, nullptr ) <= 0 ) {
      ERR_print_errors_fp(stderr);
      throw std::runtime_error("SSLSocket::LoadCertificates(): Error - couldn't load trusted CA (verify locations)");
   }

   //cargar certificados de CA confiables para validar el certificado del otro extremo
   if ( nullptr != this->BIO ) {
      SSL * ssl = reinterpret_cast<SSL *>( this->BIO );

      if ( SSL_use_certificate_file( ssl, certFileName, SSL_FILETYPE_PEM ) <= 0 ) {
         ERR_print_errors_fp(stderr);
         throw std::runtime_error("SSLSocket::LoadCertificates(): Error - certificate file (SSL)");
      }
      if ( SSL_use_PrivateKey_file( ssl, keyFileName, SSL_FILETYPE_PEM ) <= 0 ) {
         ERR_print_errors_fp(stderr);
         throw std::runtime_error("SSLSocket::LoadCertificates(): Error - key file (SSL)");
      }
      if ( !SSL_check_private_key( ssl ) ) {
         throw std::runtime_error("SSLSocket::LoadCertificates(): Private key different to the public certificate (SSL)");
      }
   }
}
 

/**
 *  Connect
 *     use SSL_connect to establish a secure conection
 *
 *  Create a SSL connection
 *
 *  @param	char * hostName, host name
 *  @param	int port, service number
 *
 **/
int SSLSocket::Connect( const char * hostName, int port ) {
   int st;

   st = this->TryToConnect( hostName, port );		// Establish a non ssl connection first
   if (st < 0) return st;

   SSL *ssl = reinterpret_cast<SSL *>( this->BIO );
   SSL_set_fd(ssl, this->sockId);//socket descriptor is attached to SSL

   st = SSL_connect(ssl);//establish a SSL connection
   if (st != 1) {
      int err = SSL_get_error(ssl, st);
      fprintf(stderr, "SSL_connect failed with error code %d\n", err);
      return -1;
   }
   return 0;
}


/**
 *  Connect
 *     use SSL_connect to establish a secure conection
 *
 *  Create a SSL connection
 *
 *  @param	char * hostName, host name
 *  @param	char * service, service name
 *
 **/
int SSLSocket::Connect( const char * host, const char * service ) {
   int st;

   st = this->TryToConnect( host, service );
   SSL *ssl = reinterpret_cast<SSL *>( this->BIO );
   SSL_set_fd(ssl, this->sockId);//socket descriptor is attached to SSL
   st = SSL_connect(ssl);//establish a SSL connection
   if (st != 1) {
      int err = SSL_get_error(ssl, st);
      fprintf(stderr, "SSL_connect failed with error code %d\n", err);
      return -1;
   }
   return 0;

}


/**
  *  Read
  *     use SSL_read to read data from an encrypted channel
  *
  *  @param	void * buffer to store data read
  *  @param	size_t size, buffer's capacity
  *
  *  @return	size_t byte quantity read
  *
  *  Reads data from secure channel
  *
 **/
size_t SSLSocket::Read( void * buffer, size_t size ) {
   SSL *ssl = reinterpret_cast<SSL *>(this->BIO);
   int st = SSL_read(ssl, buffer, static_cast<int>(size));

   if (st <= 0) {
      int err = SSL_get_error(ssl, st);

      // el otro lado ya mando todo lo que tenia y cerro la conexion con o sin un "close_notify" 
      if ( SSL_ERROR_ZERO_RETURN == err ) {
         return 0; // cierre con close_notify
      }
      if ( SSL_ERROR_SYSCALL == err && 0 == st ) {
         return 0; // TLS 1.2 cierre sin alerta formal
      }

      unsigned long ultimoError = ERR_peek_last_error();
      if ( SSL_ERROR_SSL == err && SSL_R_UNEXPECTED_EOF_WHILE_READING == ERR_GET_REASON( ultimoError ) ) {
         return 0; // TLS 1.3 
      }
      // cualquier otro codigo si es un error real
      fprintf(stderr, "SSL_read error code %d\n", err);
      throw std::runtime_error("SSLSocket::Read(): SSL_read failed");
   }

   return static_cast<size_t>(st);

}


/**
  *  Write
  *     use SSL_write to write data to an encrypted channel
  *
  *  @param	void * buffer to store data read
  *  @param	size_t size, buffer's capacity
  *
  *  @return	size_t byte quantity written
  *
  *  Writes data to a secure channel
  *
 **/
size_t SSLSocket::Write( const char * string ) {
   return this->Write(static_cast<const void *>(string), strlen(string));
}




/**
  *  Write
  *     use SSL_write to write data to an encrypted channel
  *
  *  @param	const void * buffer to store data to write
  *  @param	size_t size, buffer's capacity
  *
  *  @return	size_t byte quantity written
  *
  *  Reads data from secure channel
  *
 **/
size_t SSLSocket::Write( const void * buffer, size_t size ) {
   SSL *ssl = reinterpret_cast<SSL *>(this->BIO);
   int st = SSL_write(ssl, buffer, static_cast<int>(size));

   if (st <= 0) {
      int err = SSL_get_error(ssl, st);
      fprintf(stderr, "SSL_write error code %d\n", err);
      throw std::runtime_error("SSLSocket::Write(): SSL_write failed");
   }

   return static_cast<size_t>(st);

}


/**
 *   Show SSL certificates
 *
 **/
void SSLSocket::ShowCerts() {
   X509 *cert;
   char *line;

   cert = SSL_get_peer_certificate( (SSL *) this->BIO );		 // Get certificates (if available)
   if ( nullptr != cert ) {
      printf("Server certificates:\n");
      line = X509_NAME_oneline( X509_get_subject_name( cert ), 0, 0 );
      printf( "Subject: %s\n", line );
      free( line );
      line = X509_NAME_oneline( X509_get_issuer_name( cert ), 0, 0 );
      printf( "Issuer: %s\n", line );
      free( line );
      X509_free( cert );
   } else {
      printf( "No certificates.\n" );
   }

}


/**
 *   Return the name of the currently used cipher
 *
 **/
const char * SSLSocket::GetCipher() {

   return SSL_get_cipher( reinterpret_cast<SSL *>( this->BIO ) );

}


/**
  *  UseCertificate
  *
  *  Permite que una conexión (típicamente el cliente) cargue y presente su propio
  *  certificado ante el otro extremo -- necesario para la autenticación de dos vías
  *  que pide el servidor con SSL_VERIFY_FAIL_IF_NO_PEER_CERT. Debe llamarse ANTES
  *  de Connect().
  *
  *  @param	const char * certFileName, archivo con el certificado propio
  *  @param	const char * keyFileName, archivo con la llave privada propia
  *
 **/
void SSLSocket::UseCertificate( const char * certFileName, const char * keyFileName ) {
   this->LoadCertificates( certFileName, keyFileName );	// requirePeerCert = false: el cliente no exige nada, solo se identifica
}


/**
  *   SSLSocket::AcceptConnection
  *
  *  waits for a TLS/SSL client to initiate the TLS/SSL handshake (server side)
  *
  *  @returns	a new SSLSocket instance representing the accepted, already
  *  		SSL-handshaked connection (or nullptr if the handshake fails)
  *
 **/
VSocket * SSLSocket::AcceptConnection() {

   int newId = this->WaitForConnection(); //aceptar conexión plana
   if ( -1 == newId ) {
      throw std::runtime_error( "SSLSocket::AcceptConnection(): WaitForConnection failed" );
   }

   return this->CompletarConexion( newId );

}


/**
  *   SSLSocket::CompletarConexion
  *
  *  Termina de preparar una conexion cuyo descriptor ya fue aceptado
  *
  *  @param	int fd, descriptor ya aceptado (de WaitForConnection)
  *
  *  @returns	una nueva instancia de SSLSocket ya con el handshake
  *  		hecho (o nullptr si el handshake fallo)
  *
 **/
VSocket * SSLSocket::CompletarConexion( int newId ) {

   // si un cliente abre la conexion TCP pero nunca completa el saludo TLS
   struct timeval limiteTiempo;
   limiteTiempo.tv_sec = 10;
   limiteTiempo.tv_usec = 0;
   setsockopt( newId, SOL_SOCKET, SO_RCVTIMEO, &limiteTiempo, sizeof(limiteTiempo) );
   setsockopt( newId, SOL_SOCKET, SO_SNDTIMEO, &limiteTiempo, sizeof(limiteTiempo) );

   SSLSocket * peer = new SSLSocket( newId );//crea nuevo socket

   if ( nullptr != peer->BIO ) {
      SSL_free( reinterpret_cast<SSL *>( peer->BIO ) );
      peer->BIO = nullptr;
   }
   if ( nullptr != peer->Context ) {
      SSL_CTX_free( reinterpret_cast<SSL_CTX *>( peer->Context ) );
      peer->Context = nullptr;
   }

   // SSL_CTX_up_ref incrementa el contador de referencias del contexto
   SSL_CTX_up_ref( reinterpret_cast<SSL_CTX *>( this->Context ) );
   peer->Context = this->Context;

   SSL * ssl = SSL_new( reinterpret_cast<SSL_CTX *>( peer->Context ) );
   if ( nullptr == ssl ) {
      throw std::runtime_error( "SSLSocket::CompletarConexion(): SSL_new failed" );
   }
   SSL_set_fd( ssl, peer->sockId );
   peer->BIO = reinterpret_cast<void *>( ssl );

   int st = SSL_accept( ssl ); // espera a que el cliente inicie el handshake TLS/SSL
   if ( st != 1 ) {
      int err = SSL_get_error( ssl, st );
      fprintf( stderr, "SSLSocket::CompletarConexion(): SSL_accept failed with error code %d\n", err );
      ERR_print_errors_fp( stderr );
      delete peer;
      return nullptr;
   }

   return peer;

}