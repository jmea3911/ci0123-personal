#ifndef BODEGA_H
#define BODEGA_H

#include <string>
#include <vector>
#include <cstdint>
#include "Formatos.h"
#include "FileSystem.h"

// representa una bodega dentro de cafeteria.dat. no tiene archivo propio:
// todas las bodegas comparten el mismo Filesystem y el mismo bitmap.
class Bodega {
public:
    // bloqueDirCategorias viene de Filesystem::registrarBodega o buscarBodega
    Bodega(Filesystem& fs, uint16_t bloqueDirCategorias);

    void crearCategoria(const std::string& nombre, const std::string& id);
    bool buscarCategoria(const std::string& id, DirCategoria& resultado);
    std::vector<DirCategoria> listarCategorias();

    bool agregarProducto(const std::string& idCategoria, const std::string& nombreProducto, int cantidad, uint32_t precioCentavos);
    std::vector<RegistroProducto> listarProductos(const std::string& idCategoria);

    //resta cantidad a un producto ya existente
    //se usa para reservar existencias
    bool ajustarCantidad(const std::string& idCategoria, const std::string& nombreProducto, int delta);

private:
    Filesystem& fs_;
    uint16_t bloqueDirCategorias_;

    bool ubicarCasillaCategoria(const std::string& nombreBuscado, int& bloqueOut, int& posOut);
};

#endif