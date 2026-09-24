#ifndef INTERMEDIARIO_H
#define INTERMEDIARIO_H

#include <set>
#include <string>
#include <vector>
#include "Buzon.h"
#include "Protocolo.h"


struct ItemCarrito {
    std::string nombre;
    double precio;
    int cantidad;
};

// Representa el "servidor intermediario". No tiene datos propios de productos.
// Comunica la bodega con el cliente
class Intermediario {
public:
    explicit Intermediario( Buzon & buzon );

    void ejecutar();

private:
    Buzon & buzon_;

    int idBodega_;                               // id de la bodega, extraido del HOLA
    std::string nombreBodega_;                   // "Bodega", para los rótulos y ERR_COMM
    std::set<std::string> categoriasAtendidas_;  // categorías que sabemos que existen
    std::vector<ItemCarrito> carrito_;           // lo que el cliente ha agregado y ya se reservo

    void registrarBodega();
    void manejarRequestCategories();
    void manejarRequestProducts( const std::string & categoria );
    void manejarAddToCart( const std::string & producto, const std::string & cantidadTexto );
    void manejarRequestFactura();
};

#endif