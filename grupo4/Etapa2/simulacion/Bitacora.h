#ifndef BITACORA_H
#define BITACORA_H

#include <string>

// Imprime una línea con el formato [origen -> destino] mensaje y explicación.
// origen/destino son los IDs internos (ID_CLIENTE, ID_INTERMEDIARIO, ID_BODEGA);
// rotular() los traduce a los IDs de texto del protocolo (CLI_04, INT_04, SERV)
// antes de imprimirlos. Internamente usa un mutex para que los rótulos de los
// distintos hilos no se mezclen.
void rotular( int origen, int destino, const std::string & linea, const std::string & explicacion );

#endif