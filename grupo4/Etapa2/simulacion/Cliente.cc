#include <thread>
#include <chrono>
#include <vector>
#include <sstream>

#include "Cliente.h"
#include "Bitacora.h"
#include "Protocolo.h"

Cliente::Cliente( Buzon & buzon ) : buzon_( buzon ) {
}

// pedir() se encarga de mandar mensaje, esperar la respuesta y rotular ambos.
void Cliente::pedir( const std::string & mensaje ) {
    buzon_.Enviar( mensaje.c_str(), canal( ID_CLIENTE, ID_INTERMEDIARIO ) );
    rotular( ID_CLIENTE, ID_INTERMEDIARIO, mensaje, "solicitud enviada" );

    struct { long mtype; char texto[ TAM_MAX_MENSAJE ]; } resp;
    buzon_.Recibir( &resp, sizeof( resp.texto ), canal( ID_INTERMEDIARIO, ID_CLIENTE ) );
    rotular( ID_INTERMEDIARIO, ID_CLIENTE, std::string( resp.texto ), "respuesta recibida por el cliente" );

    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) ); // pausa para que el log sea legible
}

void Cliente::ejecutar() {
    // Pausa para evitar que el primer HOLA de la bodega se mezcle con el del cliente
    std::this_thread::sleep_for( std::chrono::milliseconds( 150 ) );

    const std::string cli      = idTextoCliente();
    const std::string intermed = idTextoIntermediario();

    std::vector<std::string> peticiones = {
        // caso normal: pedir la lista de categorias disponibles
        armarMensaje( cli, intermed, REQUEST_CATEGORIES ),
        // ERR_FORMAT: categoria muy corta (regex exige 3-20 letras)
        armarMensaje( cli, intermed, REQUEST_PRODUCTS, { "co" } ),
        // formato valido, pero ninguna bodega atiende esta categoria
        armarMensaje( cli, intermed, REQUEST_PRODUCTS, { "Licores" } ),
        // caso normal: categoria que si atiende la bodega
        armarMensaje( cli, intermed, REQUEST_PRODUCTS, { "Caramelos" } ),
        //caso en el que el carrito esta vacio
        armarMensaje( cli, intermed, REQUEST_FACTURA ),                    
        //ERR_FORMAT
        armarMensaje( cli, intermed, ADD_TO_CART, { "torta#3", "2" } ),    
        //caso correcto
        armarMensaje( cli, intermed, ADD_TO_CART, { "menta", "2" } ),
        //caso correcto
        armarMensaje( cli, intermed, ADD_TO_CART, { "fresa", "500" } ),
        //ERR_COMM
        armarMensaje( cli, intermed, ADD_TO_CART, { "silenciar", "1" } ),  
        //Factura final
        armarMensaje( cli, intermed, REQUEST_FACTURA )                    
        

    };

    for ( const auto & peticion : peticiones ) {
        pedir( peticion );
    }

    // Fin del guion de pruebas
    buzon_.Enviar( "EXIT", canal( ID_CLIENTE, ID_INTERMEDIARIO ) );
    rotular( ID_CLIENTE, ID_INTERMEDIARIO, "EXIT", "cliente termino su guion, avisa cierre" );
}