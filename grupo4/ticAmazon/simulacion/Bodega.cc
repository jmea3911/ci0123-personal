#include <sstream>
#include <algorithm>
#include <iomanip>
#include <cstdio>
#include <stdexcept>
#include "Bodega.h"
#include "Bitacora.h"

// Abre el disco del almacenamiento. Si no existe, FileSystem lanza std::runtime_error.
Bodega::Bodega( Buzon & buzon, int miId, const std::string & nombre,
                 const std::string & ip, int puerto, const std::string & rutaDisco )
    : buzon_( buzon ), miId_( miId ), nombre_( nombre ), ip_( ip ), puerto_( puerto ),
      rutaDisco_( rutaDisco ) {
    fs_ = std::make_unique<FileSystem>( rutaDisco_ );
    inventario_ = std::make_unique<Inventario>( *fs_ );
    idDisco_ = fs_->idDisco();
}

// El almacenamiento guarda el precio en centavos; el protocolo lo manda como <enteros>.<2 decimales>.
static std::string precioTexto( uint32_t centavos ) {
    char buf[ 16 ];
    snprintf( buf, sizeof( buf ), "%u.%02u", centavos / 100, centavos % 100 );
    return buf;
}

// Se registra ante el intermediario con REGISTER_SERVER (70): publica su nombre,
// ip, puerto y las categorías que atiende. Espera el REGISTER_ACK (71).
// Devuelve false si el intermediario rechazó el registro.

bool Bodega::registrarse() {
    // Las categorias que se publican son las que hay en el disco en este momento
    std::vector<std::string> categorias;
    try {
        categorias = inventario_->categorias();
    } catch ( const std::exception & e ) {
        rotular( miId_, ID_INTERMEDIARIO, "-", nombre_ + ": no se pudo leer el almacenamiento " + rutaDisco_ + " (" + e.what() + "), no se registra" );
        return false;
    }
    if ( categorias.empty() ) {
        rotular( miId_, ID_INTERMEDIARIO, "-", nombre_ + ": el almacenamiento " + rutaDisco_ + " no tiene productos, no se registra" );
        return false;
    }

    std::string registro = armarMensaje( idTexto( miId_ ), idTextoIntermediario(), REGISTER_SERVER,
                                          { nombre_, ip_, std::to_string( puerto_ ), unir( categorias, ',' ) } );
    rotular( miId_, ID_INTERMEDIARIO, registro, nombre_ + " se registra y publica las categorias leidas de su almacenamiento (" + idDisco_ + ")" );
    buzon_.Enviar( registro.c_str(), CANAL_REGISTRO );

    struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } resp;
    buzon_.Recibir( &resp, sizeof( resp.texto ), canal( ID_INTERMEDIARIO, miId_ ) );
    std::string respuesta( resp.texto );

    if ( parsearMensaje( respuesta ).tipoMensaje != REGISTER_ACK ) {
        rotular( ID_INTERMEDIARIO, miId_, respuesta, nombre_ + ": registro rechazado, termina su hilo" );
        return false;
    }
    return true;
}

// REQUEST_CATEGORIES (10): responde con CATEGORY_LIST (11), un campo con
// las categorías separadas por coma. Se leen del almacenamiento en cada consulta.
void Bodega::manejarRequestCategories() {
    std::vector<std::string> categorias;
    try {
        categorias = inventario_->categorias();
    } catch ( const std::exception & e ) {
        // sin respuesta: el intermediario lo toma como falla de comunicacion (ERR_COMM)
        rotular( ID_INTERMEDIARIO, miId_, "REQUEST_CATEGORIES", nombre_ + ": error leyendo el almacenamiento (" + e.what() + "), no responde" );
        return;
    }
    std::string resp = armarMensaje( idTexto( miId_ ), idTextoIntermediario(), CATEGORY_LIST, { unir( categorias, ',' ) } );
    buzon_.Enviar( resp.c_str(), canal( miId_, ID_INTERMEDIARIO ) );
    rotular( miId_, ID_INTERMEDIARIO, resp, "categorias leidas del almacenamiento (" + idDisco_ + ")" );
}

// REQUEST_PRODUCTS (20): responde con PRODUCT_LIST (22): <count>/<lista>,
// o con campo "0" si la categoria no existe en el almacenamiento de este servidor.
// El almacenamiento compara la categoria sin distinguir mayusculas.

void Bodega::manejarRequestProducts( const std::string & categoria ) {
    std::vector<Producto> productos;
    try {
        productos = inventario_->productos( categoria );
    } catch ( const std::exception & e ) {
        rotular( ID_INTERMEDIARIO, miId_, "REQUEST_PRODUCTS " + categoria, nombre_ + ": error leyendo el almacenamiento (" + e.what() + "), no responde" );
        return;
    }

    if ( productos.empty() ) {
        std::string resp = armarMensaje( idTexto( miId_ ), idTextoIntermediario(), PRODUCT_LIST, { "0" } );
        buzon_.Enviar( resp.c_str(), canal( miId_, ID_INTERMEDIARIO ) );
        rotular( miId_, ID_INTERMEDIARIO, resp, "categoria que no esta en mi almacenamiento (" + idDisco_ + ")" );
        return;
    }

    // Cada entrada es producto,precio,stock y van separadas por ';'
    std::vector<std::string> entradas;
    for ( const auto & p : productos ) {
        entradas.push_back( p.nombre + "," + precioTexto( p.precioCentavos ) + "," + std::to_string( p.cantidad ) );
    }
    std::string resp = armarMensaje( idTexto( miId_ ), idTextoIntermediario(), PRODUCT_LIST,
                                      { std::to_string( entradas.size() ), unir( entradas, ';' ) } );
    buzon_.Enviar( resp.c_str(), canal( miId_, ID_INTERMEDIARIO ) );
    rotular( miId_, ID_INTERMEDIARIO, resp, "listado de la categoria " + categoria + " leido del almacenamiento (" + idDisco_ + ")" );
}

void Bodega::manejarReserveStock( const std::string & producto, const std::string & cantidadTexto ) {
    if ( producto == "silenciar" ) {
        rotular( ID_INTERMEDIARIO, miId_, "RESERVE_STOCK " + producto,
                 "(" + nombre_ + " no va a responder: simulando falla)" );
        return;
    }

    int cantidadPedida = std::stoi( cantidadTexto );

    // La reserva se descuenta del stock guardado en el almacenamiento (se escribe en el disco)
    Producto encontrado;
    bool hayProducto = false;
    int cantidadReservada = 0;
    try {
        cantidadReservada = inventario_->reservar( producto, cantidadPedida, &encontrado );
        hayProducto = !encontrado.nombre.empty();
    } catch ( const std::exception & e ) {
        rotular( ID_INTERMEDIARIO, miId_, "RESERVE_STOCK " + producto, nombre_ + ": error escribiendo en el almacenamiento (" + e.what() + "), no responde" );
        return;
    }

    std::string precioFormateado = hayProducto ? precioTexto( encontrado.precioCentavos ) : "0.00";

    std::string resp = armarMensaje( idTexto( miId_ ), idTextoIntermediario(), RESERVED,
                                      { producto, precioFormateado, std::to_string( cantidadReservada ) } );
    buzon_.Enviar( resp.c_str(), canal( miId_, ID_INTERMEDIARIO ) );
    rotular( miId_, ID_INTERMEDIARIO, resp, "reserva de stock descontada en el almacenamiento (" + idDisco_ + ")" );
}

void Bodega::ejecutar() {
    try {
        if ( !registrarse() ) return;
    } catch ( const std::runtime_error & ) {
        return; // el buzon fue destruido: salida de respaldo
    }

    while ( true ) {
        struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } msg;

        try {
            buzon_.Recibir( &msg, sizeof( msg.texto ), canal( ID_INTERMEDIARIO, miId_ ) );
        } catch ( const std::runtime_error & ) {
            return; // el buzon fue destruido: salida de respaldo
        }

        std::string linea( msg.texto );

        // EXIT es señal de control interna, no forma parte del protocolo.
        if ( linea == "EXIT" ) {
            return;
        }

        MensajeProtocolo mensaje = parsearMensaje( linea );

        switch ( mensaje.tipoMensaje ) {
            case REQUEST_CATEGORIES:
                manejarRequestCategories();
                break;
            case REQUEST_PRODUCTS:
                if ( !mensaje.campos.empty() ) {
                    manejarRequestProducts( mensaje.campos[ 0 ] );
                }
                break;
            case RESERVE_STOCK:
                if ( mensaje.campos.size() >= 2 ) {
                    manejarReserveStock( mensaje.campos[ 0 ], mensaje.campos[ 1 ] );
                }
                break;
            default:

                rotular( ID_INTERMEDIARIO, miId_, linea, "tipo de mensaje no reconocido (todavia)" );
                break;
        }
    }
}
