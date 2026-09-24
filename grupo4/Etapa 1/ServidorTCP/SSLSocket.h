/**
  *  Universidad de Costa Rica
  *  ECCI
  *  CI0123 Proyecto integrador de redes y sistemas operativos
  *
  ****** SSLSocket class interface
  *
  *  Adds SSL/TLS support on top of the existing VSocket / Socket hierarchy.
  *  Built for Trabajo en clase # 09.
  *
 **/

#ifndef SSLSocket_h
#define SSLSocket_h

#include <openssl/ssl.h>
#include "Socket.h"

class SSLSocket : public Socket {

   public:
      // Client-side constructor: plain TCP socket + SSL client context
      SSLSocket( bool IPv6 = false );

      // Server-side constructor: plain TCP socket + SSL server context
      // already loaded with the given certificate/key pair
      SSLSocket( const char * certFile, const char * keyFile, bool IPv6 = false );

      // Used internally by AcceptConnection() to wrap an already-accepted
      // TCP descriptor. No SSL context is attached yet: the caller must
      // call Copy() (to share the listening socket's SSL_CTX) and
      // then Accept() (to run the handshake) before Read/Write/Close.
      SSLSocket( int id );

      ~SSLSocket();

      void Init();                 // one-time OpenSSL library initialization
      void InitContext();          // build a client-role SSL_CTX
      void InitServerContext();    // build a server-role SSL_CTX
      void InitServer( const char * certFile, const char * keyFile );
                                    // InitServerContext() + load cert/key pair

      void Copy( SSLSocket * original );
                                    // share original's SSL_CTX (no ownership),
                                    // build a new SSL object bound to this->sockId
                                    // (named "Copy" to match the reference
                                    // SSLServer.cc, which calls client->Copy(server))

      int Connect( const char *, int );          // TCP connect + SSL_connect handshake
      int Connect( const char *, const char * ); // idem, DNS/service version

      SSLSocket * Accept();         // run the server-side SSL_accept() handshake
      VSocket * AcceptConnection(); // TCP accept() wrapped in a new SSLSocket

      size_t Read( void *, size_t );
      size_t Write( const void *, size_t );
      size_t Write( const char * );

      void ShowCerts();             // print peer certificate subject/issuer
      const char * GetCipher();     // negotiated cipher suite name

      void Close();                 // SSL_shutdown/SSL_free, then TCP close

   protected:
      SSL_CTX * ctx;          // SSL context (may be shared, see ownsContext)
      SSL      * ssl;         // per-connection SSL state
      bool       isServer;
      bool       ownsContext; // only the socket that created ctx frees it

      static bool sslLibReady;

};

#endif // SSLSocket_h