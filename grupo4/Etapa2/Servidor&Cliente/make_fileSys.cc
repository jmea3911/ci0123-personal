#include "FileSystem.h"
#include "Bodega.h"
#include <iostream>

// Formatea bodega.dat
// Bodega1 (postres, caramelos) y Bodega2 (bebidas, reposteria).

struct ProductoInicial {
    std::string categoria, nombre;
    uint16_t cantidad;
    uint32_t precioCentavos;
};

int main() {
    std::cout << "Formateando bodega.dat..." << std::endl;
    Filesystem fs("bodega.dat");
    fs.crearArchivo("TicAmazon Isla 4");

    // Crea las dos bodegas iniciales.
    uint16_t b1 = fs.registrarBodega("Bodega1", "Bodega1");
    uint16_t b2 = fs.registrarBodega("Bodega2", "Bodega2");
    std::cout << "  Bodega1 (postres, caramelos)" << std::endl;
    std::cout << "  Bodega2 (bebidas, reposteria)" << std::endl;

    Bodega bodega1(fs, b1);
    Bodega bodega2(fs, b2);

    // Crea las categorias correspondientes a cada bodega.
    bodega1.crearCategoria("postres", "postres");
    bodega1.crearCategoria("caramelos", "caramelos");
    bodega2.crearCategoria("bebidas", "bebidas");
    bodega2.crearCategoria("reposteria", "reposteria");

    // Catalogo inicial de productos de Bodega1.
    std::vector<ProductoInicial> catalogoBodega1 = {
        {"postres",   "Flan-caramelo",    20,  320},
        {"postres",   "Tres-leches",      15,  380},
        {"postres",   "Tiramisu",         10,  450},
        {"caramelos", "Caramelo-menta",   300,  25},
        {"caramelos", "Caramelo-leche",   200,  30},
        {"caramelos", "Paleta-caramelo",  100,  50},
    };

    // Catalogo inicial de productos de Bodega2.
    std::vector<ProductoInicial> catalogoBodega2 = {
        {"bebidas",   "Cafe-frio",        40,  325},
        {"bebidas",   "Limonada",         60,  250},
        {"bebidas",   "Smoothie-fresa",   30,  400},
        {"bebidas",   "Cafe-negro",       100, 200},
        {"bebidas",   "Cappuccino",       60,  325},
        {"bebidas",   "Latte",            55,  350},
        {"reposteria","Croissant",        40,  220},
        {"reposteria","Brownie",          30,  250},
        {"reposteria","Pan-dulce",        50,  160},
    };

    // Inserta los productos del primer catalogo.
    for (auto& item : catalogoBodega1) {
        bodega1.agregarProducto(item.categoria, item.nombre, item.cantidad, item.precioCentavos);
        std::cout << "  OK   " << item.nombre << " -> " << item.categoria << " (Bodega1)" << std::endl;
    }

    // Inserta los productos del segundo catalogo.
    for (auto& item : catalogoBodega2) {
        bodega2.agregarProducto(item.categoria, item.nombre, item.cantidad, item.precioCentavos);
        std::cout << "  OK   " << item.nombre << " -> " << item.categoria << " (Bodega2)" << std::endl;
    }

    std::cout << "Listo. bodega.dat quedo formateada, con las dos bodegas separadas." << std::endl;
    return 0;
}