// Simulación de la tercera etapa de TicAmazon (Equipo 4 - cafeteria).
// Hilos: Cliente, Intermediario y dos servidores de productos (Bodega1 y Bodega2),
// cada uno con su disco del almacenamiento (fs1.dat.ci0123 y fs2.dat.ci0123).
// Bodega1 arranca con el sistema; Bodega2 se incorpora con el sistema ya en marcha.

#include <thread>
#include <chrono>
#include <vector>
#include <memory>
#include <string>
#include <cstdio>
#include <stdexcept>

#include "Buzon.h"
#include "Protocolo.h"
#include "Bodega.h"
#include "Intermediario.h"
#include "Cliente.h"

int main( int argc, char ** argv ) {
    // Cada servidor de productos trabaja sobre su propio disco del almacenamiento
    // (se crean con "make discos" en la carpeta almacenamiento/). Se pueden pasar otras rutas:
    //     ./simulacion [disco_SERV_01 [disco_SERV_02]]
    const std::string disco1 = argc > 1 ? argv[ 1 ] : "../almacenamiento/fs1.dat.ci0123";
    const std::string disco2 = argc > 2 ? argv[ 2 ] : "../almacenamiento/fs2.dat.ci0123";

    Buzon buzon; // un solo buzon compartido por todos los hilos

    // Las categorias y productos de cada servidor salen de su disco, no del codigo:
    //   SERV_01 (fs1): bodegas 1 y 2 -> postres, caramelos, bebidas, reposteria
    //   SERV_02 (fs2): bodegas 3 y 4 -> helados, snacks, desayunos, sandwiches
    std::unique_ptr<Bodega> bodega1, bodega2;
    try {
        bodega1 = std::make_unique<Bodega>( buzon, ID_PRIMER_SERVIDOR, "Bodega1", "172.16.123.68", 9050, disco1 );
        bodega2 = std::make_unique<Bodega>( buzon, ID_PRIMER_SERVIDOR + 1, "Bodega2", "172.16.123.69", 9050, disco2 );
    } catch ( const std::exception & e ) {
        fprintf( stderr, "No se pudo abrir el almacenamiento: %s\n"
                         "Cree los discos con:  make discos   (o  cd ../almacenamiento && make discos)\n", e.what() );
        return 1;
    }
    Intermediario intermediario( buzon );
    Cliente cliente( buzon );

    // Bodega2 no arranca con el resto: el cliente lo pide en el paso del guion donde se
    // incorpora un servidor nuevo, y entonces se registra ante el intermediario.
    std::thread hBodega2;
    cliente.incorporarServidor = [ & ]() {
        hBodega2 = std::thread( &Bodega::ejecutar, bodega2.get() );
    };

    std::thread hBodega1( &Bodega::ejecutar, bodega1.get() );
    std::thread hIntermediario( &Intermediario::ejecutar, &intermediario );

    // Pausa para que Bodega1 se registre antes de que el cliente empiece
    std::this_thread::sleep_for( std::chrono::milliseconds( 300 ) );
    std::thread hCliente( &Cliente::ejecutar, &cliente );

    // El cliente manda EXIT a intermediario; este lo reenvía a los servidores registrados antes de terminar él mismo.
    // Por eso join() en todos los hilos no se queda bloqueado para siempre.
    hCliente.join();
    hIntermediario.join();
    hBodega1.join();
    if ( hBodega2.joinable() ) hBodega2.join();

    return 0;
}
