#include "FileSystem.h"
#include "Bodega.h"
#include <iostream>
#include <iomanip>
#include <cstring>

// Muestra las formas de uso disponibles del programa.
void uso(const char* prog) {
    std::cerr << "Uso:\n"
              << "  " << prog << " <archivo.dat> bodegas\n"
              << "  " << prog << " <archivo.dat> crear-bodega <nombre>\n"
              << "  " << prog << " <archivo.dat> <bodega> categorias\n"
              << "  " << prog << " <archivo.dat> <bodega> agregar-categoria <nombre>\n"
              << "  " << prog << " <archivo.dat> <bodega> listar [categoria]\n"
              << "  " << prog << " <archivo.dat> <bodega> insertar <categoria> <nombre> <cantidad> <precio_centavos>\n";
}

// Imprime los datos principales de un producto.
void imprimirProducto(const RegistroProducto& p) {
    std::string nombre(p.nombre, strnlen(p.nombre, sizeof(p.nombre)));
    std::cout << "  " << std::left << std::setw(30) << nombre
               << " cantidad=" << p.cantidad
               << " precio=$" << (p.precioCentavos / 100.0) << std::endl;
}

int main(int argc, char** argv) {
    if (argc < 3) { uso(argv[0]); return 1; }

    std::string archivo = argv[1];
    std::string segundo = argv[2];
    Filesystem fs(archivo);
    if (!fs.archivoValido()) {
        std::cerr << "\"" << archivo << "\" no existe o no es un archivo valido "
                     "(¿lo formateaste con make_fileSys / make fileSys primero?)\n";
        return 1;
    }

    // Lista todas las bodegas existentes.
    if (segundo == "bodegas") {
        for (auto& b : fs.listarBodegas())
            std::cout << std::string(b.nombre, strnlen(b.nombre, sizeof(b.nombre))) << std::endl;
        return 0;
    }

    // Crea una nueva bodega.
    if (segundo == "crear-bodega") {
        if (argc < 4) { uso(argv[0]); return 1; }
        std::string nombre = argv[3];
        uint16_t r = fs.registrarBodega(nombre, nombre);
        std::cout << (r != BLOQUE_NULO ? "OK: bodega creada " : "FALLO ") << nombre << std::endl;
        return r != BLOQUE_NULO ? 0 : 1;
    }

    // el resto necesita el nombre de la bodega como tercer argumento
    if (argc < 4) { uso(argv[0]); return 1; }
    std::string nombreBodega = segundo;
    std::string accion = argv[3];

    DirBodega dirBodega;
    if (!fs.buscarBodega(nombreBodega, dirBodega)) {
        std::cerr << "Bodega \"" << nombreBodega << "\" no existe en " << archivo << "\n";
        return 1;
    }
    Bodega bodega(fs, dirBodega.bloqueDirCategorias);

    // Lista las categorias de la bodega.
    if (accion == "categorias") {
        for (auto& c : bodega.listarCategorias())
            std::cout << std::string(c.nombre, strnlen(c.nombre, sizeof(c.nombre))) << std::endl;
        return 0;
    }

    // Agrega una nueva categoria a la bodega.
    if (accion == "agregar-categoria") {
        if (argc < 5) { uso(argv[0]); return 1; }
        std::string nombre = argv[4];
        bodega.crearCategoria(nombre, nombre);
        std::cout << "OK: categoria agregada " << nombre << std::endl;
        return 0;
    }

    // Lista los productos de una categoria o de todas las categorias.
    if (accion == "listar") {
        if (argc >= 5) {
            std::string categoria = argv[4];
            auto productos = bodega.listarProductos(categoria);
            std::cout << categoria << " (" << productos.size() << " producto(s)):\n";
            for (auto& p : productos) imprimirProducto(p);
        } else {
            for (auto& c : bodega.listarCategorias()) {
                std::string categoria(c.nombre, strnlen(c.nombre, sizeof(c.nombre)));
                auto productos = bodega.listarProductos(categoria);
                std::cout << categoria << " (" << productos.size() << " producto(s)):\n";
                for (auto& p : productos) imprimirProducto(p);
            }
        }
        return 0;
    }

    // Inserta un nuevo producto en una categoria.
    if (accion == "insertar") {
        if (argc < 8) { uso(argv[0]); return 1; }
        std::string categoria = argv[4];
        std::string nombre = argv[5];
        int cantidad = std::stoi(argv[6]);
        uint32_t precioCentavos = (uint32_t)std::stoul(argv[7]);
        bool ok = bodega.agregarProducto(categoria, nombre, cantidad, precioCentavos);
        std::cout << (ok ? "OK: " : "FALLO: ") << nombre << " -> " << categoria << std::endl;
        return ok ? 0 : 1;
    }

    // Si la accion no coincide con ninguna opcion, muestra el uso.
    uso(argv[0]);
    return 1;
}