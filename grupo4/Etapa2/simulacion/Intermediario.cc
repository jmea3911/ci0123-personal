#include <sstream>
#include <iomanip>

#include "Intermediario.h"
#include "Bitacora.h"
#include "Validaciones.h"

Intermediario::Intermediario( Buzon & buzon ) : buzon_( buzon ), idBodega_( -1 ) {
}

// Recibe el HOLA de la bodega y aprende su id, nombre y categorías.

void Intermediario::registrarBodega() {
    struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } msg;
    buzon_.Recibir( &msg, sizeof( msg.texto ), CANAL_REGISTRO );
    std::string linea( msg.texto );

    std::istringstream iss( linea );
    std::string verbo, idBodegaTexto, categoriasTexto;
    iss >> verbo >> idBodegaTexto >> nombreBodega_ >> categoriasTexto;

    idBodega_ = std::stoi( idBodegaTexto );

    // Partir "cat1,cat2,cat3" por comas
    std::istringstream cats( categoriasTexto );
    std::string categoria;
    while ( std::getline( cats, categoria, ',' ) ) {
        categoriasAtendidas_.insert( categoria );
    }

    rotular( idBodega_, ID_INTERMEDIARIO, linea, "bodega registrada" );
}

// REQUEST_CATEGORIES (10): no lleva campos. Se le pide la lista a la
// bodega y se reenvia tal cual al cliente como CATEGORY_LIST (11).
void Intermediario::manejarRequestCategories() {
    std::string mensaje = armarMensaje( idTextoIntermediario(), idTextoBodega(), REQUEST_CATEGORIES );
    buzon_.Enviar( mensaje.c_str(), canal( ID_INTERMEDIARIO, idBodega_ ) );
    rotular( ID_INTERMEDIARIO, idBodega_, mensaje, "pidiendo categorias a " + nombreBodega_ );

    struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } resp;
    buzon_.Recibir( &resp, sizeof( resp.texto ), canal( idBodega_, ID_INTERMEDIARIO ) );
    std::string respuesta( resp.texto );
    MensajeProtocolo msgResp = parsearMensaje( respuesta );

    std::string salida = armarMensaje( idTextoIntermediario(), idTextoCliente(), CATEGORY_LIST, msgResp.campos );
    buzon_.Enviar( salida.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
    rotular( ID_INTERMEDIARIO, ID_CLIENTE, salida, "categorias reenviadas al cliente" );
}

// REQUEST_PRODUCTS (20): un campo, la categoria.
void Intermediario::manejarRequestProducts( const std::string & categoria ) {
    if ( !validarCategoria( categoria ) ) {
        std::string err = armarMensaje( idTextoIntermediario(), idTextoCliente(), ERR_FORMAT,
                                         { std::to_string( REQUEST_PRODUCTS ), "categoria", categoria } );
        buzon_.Enviar( err.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, err, "categoria no cumple regex" );
        return;
    }

    if ( categoriasAtendidas_.find( categoria ) == categoriasAtendidas_.end() ) {
        // Ya sabemos (desde el registro) que ninguna bodega atiende esta
        // categoria; no hace falta ni preguntarle.
        std::ostringstream texto;
        texto << "No hay productos disponibles en la categoria \"" << categoria << "\"";
        std::string vacio = armarMensaje( idTextoIntermediario(), idTextoCliente(), PRODUCT_LIST_EMPTY, { texto.str() } );
        buzon_.Enviar( vacio.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, vacio, "la bodega no atiende esa categoria" );
        return;
    }

    std::string fwd = armarMensaje( idTextoIntermediario(), idTextoBodega(), REQUEST_PRODUCTS, { categoria } );
    buzon_.Enviar( fwd.c_str(), canal( ID_INTERMEDIARIO, idBodega_ ) );
    rotular( ID_INTERMEDIARIO, idBodega_, fwd, "reenviando a " + nombreBodega_ );

    struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } resp;
    buzon_.Recibir( &resp, sizeof( resp.texto ), canal( idBodega_, ID_INTERMEDIARIO ) );
    std::string respuesta( resp.texto );
    MensajeProtocolo msgResp = parsearMensaje( respuesta );

    bool vacio = msgResp.campos.empty() || msgResp.campos[ 0 ] == "0";
    if ( vacio ) {
        std::ostringstream texto;
        texto << "No hay productos disponibles en la categoria \"" << categoria << "\"";
        std::string salidaVacia = armarMensaje( idTextoIntermediario(), idTextoCliente(), PRODUCT_LIST_EMPTY, { texto.str() } );
        buzon_.Enviar( salidaVacia.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, salidaVacia, "categoria vacia" );
    } else {
        std::string salida = armarMensaje( idTextoIntermediario(), idTextoCliente(), PRODUCT_LIST, msgResp.campos );
        buzon_.Enviar( salida.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, salida, "listado reenviado al cliente" );
    }
}

void Intermediario::manejarAddToCart( const std::string & producto, const std::string & cantidadTexto ) {
    if ( !validarProducto( producto ) ) {
        std::string err = armarMensaje( idTextoIntermediario(), idTextoCliente(), ERR_FORMAT,
                                         { std::to_string( ADD_TO_CART ), "producto", producto } );
        buzon_.Enviar( err.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, err, "producto no cumple regex" );
        return;
    }
    if ( !validarCount( cantidadTexto ) ) {
        std::string err = armarMensaje( idTextoIntermediario(), idTextoCliente(), ERR_FORMAT,
                                         { std::to_string( ADD_TO_CART ), "cantidad", cantidadTexto } );
        buzon_.Enviar( err.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, err, "cantidad no cumple regex" );
        return;
    }

    std::string fwd = armarMensaje( idTextoIntermediario(), idTextoBodega(), RESERVE_STOCK, { producto, cantidadTexto } );
    buzon_.Enviar( fwd.c_str(), canal( ID_INTERMEDIARIO, idBodega_ ) );
    rotular( ID_INTERMEDIARIO, idBodega_, fwd, "reservando en " + nombreBodega_ );

    struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } resp;
    int intentos = 2;
    int st = buzon_.RecibirConEspera( &resp, sizeof( resp.texto ),
                                       canal( idBodega_, ID_INTERMEDIARIO ), intentos, 300 );

    if ( st == -1 ) {
        std::string err = armarMensaje( idTextoIntermediario(), idTextoCliente(), ERR_COMM,
                                         { idTextoBodega(), std::to_string( RESERVE_STOCK ), std::to_string( intentos ) } );
        buzon_.Enviar( err.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, err, nombreBodega_ + " no respondio" );
        return;
    }

    std::string respuesta( resp.texto );
    MensajeProtocolo msgResp = parsearMensaje( respuesta );

    if ( msgResp.tipoMensaje == RESERVED && msgResp.campos.size() >= 3 ) {
        const std::string & nombreProd = msgResp.campos[ 0 ];
        double precio = std::stod( msgResp.campos[ 1 ] );
        int cantidadReservada = std::stoi( msgResp.campos[ 2 ] );

        if ( cantidadReservada > 0 ) {
            carrito_.push_back( { nombreProd, precio, cantidadReservada } );
        }

        std::string salida = armarMensaje( idTextoIntermediario(), idTextoCliente(), CART_UPDATED, msgResp.campos );
        buzon_.Enviar( salida.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, salida, "carrito actualizado" );
    }
}

void Intermediario::manejarRequestFactura() {
    if ( carrito_.empty() ) {
        std::string vacio = armarMensaje( idTextoIntermediario(), idTextoCliente(), CART_EMPTY, { "El carrito esta vacio" } );
        buzon_.Enviar( vacio.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
        rotular( ID_INTERMEDIARIO, ID_CLIENTE, vacio, "carrito vacio" );
        return;
    }

    std::ostringstream detalle;
    double total = 0.0;
    for ( const auto & item : carrito_ ) {
        double subtotal = item.precio * item.cantidad;
        total += subtotal;
        detalle << item.nombre << "," << item.cantidad << ","
                 << std::fixed << std::setprecision( 2 ) << subtotal << ";";
    }
    std::ostringstream totalTexto;
    totalTexto << std::fixed << std::setprecision( 2 ) << total;

    std::string factura = armarMensaje( idTextoIntermediario(), idTextoCliente(), FACTURA,
                                          { detalle.str(), totalTexto.str() } );
    buzon_.Enviar( factura.c_str(), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
    rotular( ID_INTERMEDIARIO, ID_CLIENTE, factura, "factura emitida" );

    carrito_.clear();
}


void Intermediario::ejecutar() {
    registrarBodega();

    while ( true ) {
        struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } msg;

        try {
            buzon_.Recibir( &msg, sizeof( msg.texto ), canal( ID_CLIENTE, ID_INTERMEDIARIO ) );
        } catch ( const std::runtime_error & ) {
            return; // el buzon fue destruido: salida de respaldo
        }

        std::string linea( msg.texto );
        rotular( ID_CLIENTE, ID_INTERMEDIARIO, linea, "peticion recibida" );

        if ( linea == "EXIT" ) {
            rotular( ID_CLIENTE, ID_INTERMEDIARIO, linea, "orden de cierre recibida del cliente" );
            buzon_.Enviar( "EXIT", canal( ID_INTERMEDIARIO, idBodega_ ) );
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
            case ADD_TO_CART:
                if ( mensaje.campos.size() >= 2 ) {
                    manejarAddToCart( mensaje.campos[ 0 ], mensaje.campos[ 1 ] );
                }
                break;
            case REQUEST_FACTURA:
                manejarRequestFactura();
                break;
            default:
                
                rotular( ID_CLIENTE, ID_INTERMEDIARIO, linea, "tipo de mensaje no reconocido (todavia)" );
                break;
        }
    }
}