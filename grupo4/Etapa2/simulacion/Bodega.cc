#include <sstream>
#include <algorithm>
#include <iomanip>
#include "Bodega.h"
#include "Bitacora.h"

Bodega::Bodega( Buzon & buzon, int miId, const std::string & nombre,
                 std::vector<std::string> categorias, std::vector<Producto> productos )
    : buzon_( buzon ), miId_( miId ), nombre_( nombre ),
      categorias_( std::move( categorias ) ), productos_( std::move( productos ) ) {
}

// Se anuncia ante el intermediario con su ID y las categorías que atiende.

void Bodega::anunciarse() {
    std::ostringstream msg;
    msg << "HOLA " << miId_ << " " << nombre_ << " ";
    for ( size_t i = 0; i < categorias_.size(); ++i ) {
        msg << categorias_[ i ];
        if ( i + 1 < categorias_.size() ) msg << ",";
    }
    buzon_.Enviar( msg.str().c_str(), CANAL_REGISTRO );
    rotular( miId_, ID_INTERMEDIARIO, msg.str(), "la bodega se anuncia" );
}

// REQUEST_CATEGORIES (10): responde con CATEGORY_LIST (11), un campo con
// las categorías separadas por coma.
void Bodega::manejarRequestCategories() {
    std::ostringstream lista;
    for ( size_t i = 0; i < categorias_.size(); ++i ) {
        lista << categorias_[ i ];
        if ( i + 1 < categorias_.size() ) lista << ",";
    }
    std::string resp = armarMensaje( idTexto( miId_ ), idTextoIntermediario(), CATEGORY_LIST, { lista.str() } );
    buzon_.Enviar( resp.c_str(), canal( miId_, ID_INTERMEDIARIO ) );
    rotular( miId_, ID_INTERMEDIARIO, resp, "categorias enviadas" );
}

// REQUEST_PRODUCTS (20): responde con PRODUCT_LIST (22): <count>/<lista>,
// o con campo "0" si la categoria no la atiende esta bodega.

void Bodega::manejarRequestProducts( const std::string & categoria ) {
    bool laAtiendo = std::find( categorias_.begin(), categorias_.end(), categoria ) != categorias_.end();

    if ( !laAtiendo ) {
        std::string resp = armarMensaje( idTexto( miId_ ), idTextoIntermediario(), PRODUCT_LIST, { "0" } );
        buzon_.Enviar( resp.c_str(), canal( miId_, ID_INTERMEDIARIO ) );
        rotular( miId_, ID_INTERMEDIARIO, resp, "categoria que no me corresponde" );
        return;
    }

    std::ostringstream lista;
    int cantidad = 0;
    for ( const auto & p : productos_ ) {
        if ( p.categoria != categoria ) continue;
        lista << p.nombre << "," << p.precio << "," << p.stock << ";";
        ++cantidad;
    }
    std::string resp = armarMensaje( idTexto( miId_ ), idTextoIntermediario(), PRODUCT_LIST,
                                      { std::to_string( cantidad ), lista.str() } );
    buzon_.Enviar( resp.c_str(), canal( miId_, ID_INTERMEDIARIO ) );
    rotular( miId_, ID_INTERMEDIARIO, resp, "listado de la categoria " + categoria );
}

void Bodega::manejarReserveStock( const std::string & producto, const std::string & cantidadTexto ) {
    if ( producto == "silenciar" ) {
        rotular( ID_INTERMEDIARIO, miId_, "RESERVE_STOCK " + producto,
                 "(" + nombre_ + " no va a responder: simulando falla)" );
        return;
    }

    int cantidadPedida = std::stoi( cantidadTexto );
    auto it = std::find_if( productos_.begin(), productos_.end(),
                             [ & ]( const Producto & p ) { return p.nombre == producto; } );

    float precio = 0.0f;
    int cantidadReservada = 0;
    if ( it != productos_.end() ) {
        precio = it->precio;
        cantidadReservada = std::min( cantidadPedida, it->stock );
        it->stock -= cantidadReservada;
    }

    std::ostringstream precioTexto;
    precioTexto << std::fixed << std::setprecision( 2 ) << precio;

    std::string resp = armarMensaje( idTexto( miId_ ), idTextoIntermediario(), RESERVED,
                                      { producto, precioTexto.str(), std::to_string( cantidadReservada ) } );
    buzon_.Enviar( resp.c_str(), canal( miId_, ID_INTERMEDIARIO ) );
    rotular( miId_, ID_INTERMEDIARIO, resp, "reserva de stock" );
}

void Bodega::ejecutar() {
    anunciarse();

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
            rotular( ID_INTERMEDIARIO, miId_, linea, "orden de cierre: " + nombre_ + " termina su hilo" );
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
            // RESERVE_STOCK (50) se agrega cuando Intermediario mande
            // ADD_TO_CART/RESERVE_STOCK.
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