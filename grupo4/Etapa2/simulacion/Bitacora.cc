#include <cstdio>
#include <mutex>

#include "Bitacora.h"
#include "Protocolo.h"

static std::mutex mtxPantalla;


void rotular( int origen, int destino, const std::string & linea, const std::string & explicacion ) {
    std::lock_guard<std::mutex> guard( mtxPantalla );
    printf( "[%-6s -> %-6s] %-45s %s\n",
            idTexto( origen ).c_str(), idTexto( destino ).c_str(), linea.c_str(), explicacion.c_str() );
}