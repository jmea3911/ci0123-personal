#include "ServidorProductos.h"
#include "protocolo.h"

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <cstring>
#include <cstdio>
#include <cctype>

static std::string aMinusculas(const std::string& s) {
    std::string r = s;
    for (char& c : r) c = (char)tolower((unsigned char)c);
    return r;
}

// Inicializa el servidor cargando las bodegas y categorias disponibles.
ServidorProductos::ServidorProductos(const std::string& archivo) : fs_(archivo) {
    for (auto& dirBodega : fs_.listarBodegas()) {
        std::string nombreBodega(dirBodega.nombre, strnlen(dirBodega.nombre, sizeof(dirBodega.nombre)));
        bodegas_.emplace_back(fs_, dirBodega.bloqueDirCategorias);
        size_t idx = bodegas_.size() - 1;

        // Relaciona cada categoria con la bodega que la maneja.
        for (auto& dirCat : bodegas_[idx].listarCategorias()) {
            std::string nombreCategoria(dirCat.nombre, strnlen(dirCat.nombre, sizeof(dirCat.nombre)));
            categoriaAIndiceBodega_[aMinusculas(nombreCategoria)] = idx;
            categorias_.push_back(nombreCategoria);
        }
    }
}

// Verifica si el servidor maneja una categoria determinada.
bool ServidorProductos::tieneCategoria(const std::string& categoria) const {
    return categoriaAIndiceBodega_.find(aMinusculas(categoria)) != categoriaAIndiceBodega_.end();
}

// Convierte un precio almacenado en centavos a un texto con dos decimales.
static std::string formatearPrecio(uint32_t centavos) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2f", centavos / 100.0);
    return buf;
}

//arma una pagina HTML con el catalogo
std::string ServidorProductos::generarPaginaHTML() {
    std::lock_guard<std::mutex> lock(mtx_);

    std::ostringstream html;

    // Construye la estructura inicial de la pagina HTML y sus estilos.
    html << "<!DOCTYPE html>\n<html lang=\"es\">\n<head>\n"
         << "<meta charset=\"utf-8\">\n"
         << "<title>TicAmazon - Servidor de productos</title>\n"
         << "<style>\n"
         << "body{font-family:sans-serif;max-width:700px;margin:2rem auto;padding:0 1rem;color:#222}\n"
         << "h1{color:#b34700}\n"
         << "h2{border-bottom:2px solid #eee;padding-bottom:.3rem;margin-top:2rem}\n"
         << "table{width:100%;border-collapse:collapse;margin-top:.5rem}\n"
         << "th,td{text-align:left;padding:.4rem .6rem;border-bottom:1px solid #eee}\n"
         << "th{background:#f7f7f7}\n"
         << ".vacio{color:#888;font-style:italic}\n"
         << "footer{margin-top:2rem;color:#888;font-size:.85rem}\n"
         << "</style>\n</head>\n<body>\n"
         << "<h1>Servidor de productos &mdash; " << MI_ID << "</h1>\n"
         << "<p>Categorias que maneja este servidor: ";

    // Agrega a la pagina las categorias que maneja este servidor.
    for (size_t i = 0; i < categorias_.size(); ++i) {
        html << categorias_[i];
        if (i + 1 < categorias_.size()) html << ", ";
    }
    html << "</p>\n";

    // Genera una seccion HTML para cada categoria.
    for (auto& categoria : categorias_) {
        html << "<h2>" << categoria << "</h2>\n";
        size_t idx = categoriaAIndiceBodega_[aMinusculas(categoria)];
        auto productos = bodegas_[idx].listarProductos(categoria);

        // Indica cuando una categoria no tiene productos.
        if (productos.empty()) {
            html << "<p class=\"vacio\">(sin productos)</p>\n";
            continue;
        }

        html << "<table>\n<tr><th>Producto</th><th>Precio</th><th>Stock</th></tr>\n";

        // Agrega cada producto a la tabla con su precio y stock.
        for (auto& p : productos) {
            std::string nombre(p.nombre, strnlen(p.nombre, sizeof(p.nombre)));
            char precioBuf[16];
            snprintf(precioBuf, sizeof(precioBuf), "%.2f", p.precioCentavos / 100.0);
            html << "<tr><td>" << nombre << "</td><td>$" << precioBuf
                 << "</td><td>" << p.cantidad << "</td></tr>\n";
        }
        html << "</table>\n";
    }

    // Agrega el pie de pagina y devuelve todo el HTML generado.
    html << "<footer>Generado por " << MI_ID
         << " &mdash; Isla 4 :)</footer>\n</body>\n</html>\n";
    return html.str();
}

//procesa una linea de protocolo ya recibida completa 
std::string ServidorProductos::procesarMensaje(const std::string& lineaRecibida) {
    std::lock_guard<std::mutex> lock(mtx_);

    // Convierte la linea recibida en un mensaje del protocolo.
    MensajeV2 msg = parsearMensaje(lineaRecibida);
    if (msg.tipo.empty()) return "";

    //10 - REQUEST_CATEGORIES: todas las categorias, de todas las bodegas
    if (msg.tipo == "10") {
        std::ostringstream lista;
        for (size_t i = 0; i < categorias_.size(); ++i) {
            lista << categorias_[i];
            if (i + 1 < categorias_.size()) lista << ",";
        }

        // Responde con la lista de categorias manejadas por el servidor.
        return construirMensaje(MI_ID, msg.origen, "11", {lista.str()});
    }

    //20 - REQUEST_PRODUCTS
    if (msg.tipo == "20") {
        std::string categoria = msg.campos.empty() ? "" : msg.campos[0];
        auto it = categoriaAIndiceBodega_.find(aMinusculas(categoria));
        if (it == categoriaAIndiceBodega_.end()) return ""; //ninguna bodega maneja esa categoria

        // Obtiene los productos de la categoria solicitada.
        auto productos = bodegas_[it->second].listarProductos(categoria);
        std::ostringstream lista;
        for (size_t i = 0; i < productos.size(); ++i) {
            auto& p = productos[i];
            std::string nombre(p.nombre, strnlen(p.nombre, sizeof(p.nombre)));
            lista << nombre << "," << formatearPrecio(p.precioCentavos) << "," << p.cantidad;
            if (i + 1 < productos.size()) lista << ";";
        }

        // Envia la cantidad de productos y sus datos al cliente.
        return construirMensaje(MI_ID, msg.origen, "22",
                                 {std::to_string(productos.size()), lista.str()});
    }

    //50 - RESERVE_STOCK -- responde siempre con 51 y la cantidad realmente
    //reservada (puede ser 0 si no existe o no habia stock)
    if (msg.tipo == "50") {
        if (msg.campos.size() < 2) return "";
        std::string nombre = msg.campos[0];
        int cantidad;
        try { cantidad = std::stoi(msg.campos[1]); } catch (...) { return ""; }

        //ubicar el producto (y su precio) entre todas las categorias de todas las bodegas
        double precio = 0;
        bool existe = false;
        std::string categoriaDueña;

        // Busca el producto en las categorias manejadas por el servidor.
        for (auto& categoria : categorias_) {
            size_t idx = categoriaAIndiceBodega_[aMinusculas(categoria)];
            for (auto& p : bodegas_[idx].listarProductos(categoria)) {
                std::string desc(p.nombre, strnlen(p.nombre, sizeof(p.nombre)));
                if (aMinusculas(desc) == aMinusculas(nombre)) { precio = p.precioCentavos / 100.0; existe = true; categoriaDueña = categoria; break; }
            }
            if (existe) break;
        }

        // Intenta disminuir la cantidad disponible del producto.
        int cantidadReservada = 0;
        if (existe) {
            size_t idx = categoriaAIndiceBodega_[aMinusculas(categoriaDueña)];
            if (bodegas_[idx].ajustarCantidad(categoriaDueña, nombre, -cantidad)) {
                cantidadReservada = cantidad;
            }
        }

        // Construye la respuesta indicando el producto, precio y cantidad reservada.
        char precioBuf[16];
        snprintf(precioBuf, sizeof(precioBuf), "%.2f", precio);
        return construirMensaje(MI_ID, msg.origen, "51",
                                 {nombre, precioBuf, std::to_string(cantidadReservada)});
    }

    // Devuelve una respuesta vacia para tipos de mensaje no reconocidos.
    return "";
}