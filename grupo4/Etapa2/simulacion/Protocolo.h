#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>


// IDs INTERNOS (para el enrutamiento por Buzon / colas de mensajes)

const int ID_CLIENTE       = 1;
const int ID_INTERMEDIARIO = 2;
const int ID_BODEGA        = 3;

// El mtype identifica el CANAL, quién habla con quién, no solo
// el destinatario. Se calcula como origen*10 + destino. Así:
//   Cliente -> Intermediario  = 12
//   Intermediario -> Cliente  = 21
//   Intermediario -> Bodega   = 23
//   Bodega -> Intermediario   = 32

inline long canal( int origen, int destino ) {
    return origen * 10 + destino;
}

// Canal que usa la bodega para anunciarse al arrancar.
const long CANAL_REGISTRO = 99;

// Tamaño máximo de mensaje (coincide con el límite de 256 bytes del protocolo).
const int TAM_MAX_MENSAJE = 256;

// IDs DE TEXTO DEL PROTOCOLO TICAMAZON 

const std::string NUM_ISLA = "04";

inline std::string idTextoCliente()       { return "CLI_" + NUM_ISLA; }
inline std::string idTextoIntermediario() { return "INT_" + NUM_ISLA; }
inline std::string idTextoBodega()        { return "SERV"; }

// Traduce un ID interno (ID_CLIENTE, ID_INTERMEDIARIO, ID_BODEGA) al
// identificador de texto que exige el protocolo (CLI_04, INT_04, SERV).
// Útil para armar mensajes y para los rótulos en Bitacora.
inline std::string idTexto( int idInterno ) {
    switch ( idInterno ) {
        case ID_CLIENTE:       return idTextoCliente();
        case ID_INTERMEDIARIO: return idTextoIntermediario();
        case ID_BODEGA:        return idTextoBodega();
        default:                return "?";
    }
}

// tabla del protocolo)
enum TipoMensaje {
    REQUEST_CATEGORIES = 10,
    CATEGORY_LIST      = 11,
    REQUEST_PRODUCTS   = 20,
    PRODUCT_LIST       = 22,

    PRODUCT_LIST_EMPTY = 23,
    ADD_TO_CART        = 30,

    CART_UPDATED       = 32,
    REQUEST_FACTURA    = 40,
    FACTURA            = 41,
  
    CART_EMPTY         = 42,
    RESERVE_STOCK      = 50,
    RESERVED           = 51,
    ERR_FORMAT         = 90,
    ERR_SIZE           = 91,
    ERR_COMM           = 92
};

// ARMAR / PARSEAR mensajes con el formato ORIGEN|DESTINO/TIPO/campo1/...

// Construye la línea de protocolo a partir de sus partes.
// Ej: armarMensaje( idTextoCliente(), idTextoIntermediario(), REQUEST_PRODUCTS, { "postres" } )
//     -> "CLI_04|INT_04/20/postres"
inline std::string armarMensaje( const std::string & origen, const std::string & destino,
                                  int tipoMensaje, const std::vector<std::string> & campos = {} ) {
    std::ostringstream oss;
    oss << origen << "|" << destino << "/"
        << std::setw( 2 ) << std::setfill( '0' ) << tipoMensaje;
    for ( const auto & campo : campos ) {
        oss << "/" << campo;
    }
    return oss.str();
}

// Mensaje ya separado en sus componentes, para que cada clase no tenga que hacer su propio parseo 
struct MensajeProtocolo {
    std::string origen;
    std::string destino;
    int tipoMensaje = -1;
    std::vector<std::string> campos;
};

// Descompone una línea "ORIGEN|DESTINO/TIPO/campo1/campo2/..." en sus partes.
// No valida regex/tamaños (eso lo sigue haciendo Validaciones.cc); si la
// línea viene mal formada, tipoMensaje queda en -1 para que quien llama lo
// detecte.
inline MensajeProtocolo parsearMensaje( const std::string & linea ) {
    MensajeProtocolo m;

    auto posPipe = linea.find( '|' );
    if ( posPipe == std::string::npos ) return m; // mal formado

    m.origen = linea.substr( 0, posPipe );
    std::string resto = linea.substr( posPipe + 1 ); // DESTINO/TIPO/campo1/...

    std::vector<std::string> partes;
    std::istringstream iss( resto );
    std::string parte;
    while ( std::getline( iss, parte, '/' ) ) {
        partes.push_back( parte );
    }
    if ( partes.size() < 2 ) return m; // falta destino o tipo

    m.destino = partes[ 0 ];
    try {
        m.tipoMensaje = std::stoi( partes[ 1 ] );
    } catch ( const std::exception & ) {
        m.tipoMensaje = -1;
        return m;
    }
    for ( size_t i = 2; i < partes.size(); ++i ) {
        m.campos.push_back( partes[ i ] );
    }
    return m;
}

// Un producto de bodega.

struct Producto {
    std::string nombre;
    float precio;
    int stock;
    std::string descripcion;
    std::string categoria;
};

#endif