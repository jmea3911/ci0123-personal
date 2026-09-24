#pragma once
#include <string>
#include <vector>
#include <map>
#include <mutex>

#include "FileSystem.h"
#include "Bodega.h"

// ServidorProductos entiende el protocolo (10/11/20/22/50/51) y lo traduce
// a llamadas sobre el Filesystem y las Bodega 
class ServidorProductos {
public:
    explicit ServidorProductos(const std::string& archivo);

    std::string procesarMensaje(const std::string& lineaRecibida);
    std::string generarPaginaHTML();

    bool tieneCategoria(const std::string& categoria) const;
    const std::vector<std::string>& categorias() const { return categorias_; }

private:
    Filesystem fs_;           
    std::vector<Bodega> bodegas_;
    std::map<std::string, size_t> categoriaAIndiceBodega_;
    std::vector<std::string> categorias_;
    std::mutex mtx_;
    static constexpr const char* MI_ID = "SERV";
};
