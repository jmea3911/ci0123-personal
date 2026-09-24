#include "FileSystem.h"
#include "Bodega.h"
#include <iostream>
#include <iomanip>
#include <cstring>

// Genera un reporte con las bodegas, categorias y productos de un archivo.
void reportarArchivo(const std::string& archivo) {
    std::cout << "=== " << archivo << " ===" << std::endl;
    Filesystem fs(archivo);

    // Verifica que el archivo exista y tenga una estructura valida.
    if (!fs.archivoValido()) {
        std::cout << "  (archivo invalido o inexistente)\n";
        return;
    }

    // Obtiene las bodegas almacenadas en el archivo.
    auto bodegas = fs.listarBodegas();
    if (bodegas.empty()) {
        std::cout << "  (sin bodegas)\n";
        return;
    }

    int totalProductos = 0, totalCategorias = 0;

    // Recorre cada bodega y sus respectivas categorias y productos.
    for (auto& dirBodega : bodegas) {
        std::string nombreBodega(dirBodega.nombre, strnlen(dirBodega.nombre, sizeof(dirBodega.nombre)));
        std::cout << "  [" << nombreBodega << "]\n";
        Bodega bodega(fs, dirBodega.bloqueDirCategorias);

        for (auto& dirCat : bodega.listarCategorias()) {
            std::string nombreCategoria(dirCat.nombre, strnlen(dirCat.nombre, sizeof(dirCat.nombre)));
            auto productos = bodega.listarProductos(nombreCategoria);

            totalProductos += (int)productos.size();
            totalCategorias++;

            std::cout << "    " << nombreCategoria << " (" << productos.size() << "):\n";

            // Muestra la informacion de cada producto de la categoria.
            for (auto& p : productos) {
                std::string nombre(p.nombre, strnlen(p.nombre, sizeof(p.nombre)));
                std::cout << "      - " << std::left << std::setw(30) << nombre
                           << " cantidad=" << p.cantidad
                           << " precio=$" << (p.precioCentavos / 100.0) << std::endl;
            }
        }
    }

    // Muestra el total de elementos encontrados en el archivo.
    std::cout << "  total: " << bodegas.size() << " bodega(s), " << totalCategorias
               << " categoria(s), " << totalProductos << " producto(s)" << std::endl << std::endl;
}

int main(int argc, char** argv) {
    std::vector<std::string> archivos;

    // Permite recibir uno o varios archivos por linea de comandos.
    if (argc > 1) {
        for (int i = 1; i < argc; ++i) archivos.push_back(argv[i]);
    } else {
        // Si no se proporciona un archivo, utiliza bodega.dat.
        archivos = {"bodega.dat"};
    }

    // Genera un reporte para cada archivo recibido.
    for (auto& archivo : archivos) reportarArchivo(archivo);
    return 0;
}