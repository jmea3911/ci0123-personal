#include "FileSystem.h"
#include "Bodega.h"
#include <iostream>

// Formatea bodega.dat
// Bodega1 (9 categorias) y Bodega2 (9 categorias): 113 productos en total.

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
    std::cout << "  Bodega1 y Bodega2 registradas" << std::endl;

    Bodega bodega1(fs, b1);
    Bodega bodega2(fs, b2);

    // Crea las categorias correspondientes a cada bodega.
    bodega1.crearCategoria("postres", "postres");
    bodega1.crearCategoria("caramelos", "caramelos");
    bodega2.crearCategoria("bebidas", "bebidas");
    bodega2.crearCategoria("reposteria", "reposteria");

    // Categorias adicionales. Cada una tiene 7 productos para que la lista
    // (tipo 22) siga cabiendo en el limite de 256 bytes del protocolo.
    // Bodega1 queda con 9 categorias, mas de las 7 casillas de un bloque de
    // directorio, asi que esto tambien ejercita el crecimiento por punteros.
    bodega1.crearCategoria("snacks", "snacks");
    bodega1.crearCategoria("panaderia", "panaderia");
    bodega1.crearCategoria("helados", "helados");
    bodega1.crearCategoria("frutas", "frutas");
    bodega1.crearCategoria("lacteos", "lacteos");
    bodega1.crearCategoria("cereales", "cereales");
    bodega1.crearCategoria("galletas", "galletas");
    bodega2.crearCategoria("sandwiches", "sandwiches");
    bodega2.crearCategoria("ensaladas", "ensaladas");
    bodega2.crearCategoria("sopas", "sopas");
    bodega2.crearCategoria("jugos", "jugos");
    bodega2.crearCategoria("infusiones", "infusiones");
    bodega2.crearCategoria("desayunos", "desayunos");
    bodega2.crearCategoria("almuerzos", "almuerzos");

    // Catalogo inicial de productos de Bodega1.
    std::vector<ProductoInicial> catalogoBodega1 = {
        {"postres",   "Flan-caramelo",    20,  320},
        {"postres",   "Tres-leches",      15,  380},
        {"postres",   "Tiramisu",         10,  450},
        {"caramelos", "Caramelo-menta",   300,  25},
        {"caramelos", "Caramelo-leche",   200,  30},
        {"caramelos", "Paleta-caramelo",  100,  50},
        {"snacks",     "Papitas-clasicas", 50, 150},
        {"snacks",     "Papitas-limon", 45, 150},
        {"snacks",     "Nachos-queso", 35, 250},
        {"snacks",     "Palomitas", 60, 200},
        {"snacks",     "Mani-salado", 40, 180},
        {"snacks",     "Platanitos", 55, 175},
        {"snacks",     "Tortrix", 70, 140},
        {"panaderia",  "Pan-blanco", 80, 200},
        {"panaderia",  "Pan-integral", 60, 250},
        {"panaderia",  "Pan-de-ajo", 45, 300},
        {"panaderia",  "Baguette", 30, 350},
        {"panaderia",  "Pan-de-molde", 50, 400},
        {"panaderia",  "Rosquillas", 65, 180},
        {"panaderia",  "Pan-de-queso", 40, 260},
        {"helados",    "Helado-vainilla", 40, 300},
        {"helados",    "Helado-chocolate", 40, 300},
        {"helados",    "Helado-fresa", 35, 300},
        {"helados",    "Paleta-limon", 60, 150},
        {"helados",    "Paleta-coco", 55, 150},
        {"helados",    "Sundae", 25, 450},
        {"helados",    "Copa-tres-sabores", 20, 550},
        {"frutas",     "Manzana", 120, 100},
        {"frutas",     "Banano", 200, 60},
        {"frutas",     "Fresas", 50, 350},
        {"frutas",     "Uvas", 45, 400},
        {"frutas",     "Mango", 70, 250},
        {"frutas",     "Pina-picada", 40, 300},
        {"frutas",     "Sandia-picada", 35, 280},
        {"lacteos",    "Yogurt-natural", 60, 200},
        {"lacteos",    "Yogurt-fresa", 55, 220},
        {"lacteos",    "Queso-fresco", 30, 450},
        {"lacteos",    "Leche-entera", 80, 250},
        {"lacteos",    "Leche-deslactosada", 50, 300},
        {"lacteos",    "Natilla", 45, 180},
        {"lacteos",    "Mantequilla", 25, 350},
        {"cereales",   "Granola", 40, 400},
        {"cereales",   "Avena-instantanea", 60, 250},
        {"cereales",   "Barra-cereal", 150, 120},
        {"cereales",   "Corn-flakes", 35, 450},
        {"cereales",   "Cereal-chocolate", 30, 500},
        {"cereales",   "Muesli", 25, 550},
        {"cereales",   "Arroz-inflado", 45, 300},
        {"galletas",   "Galleta-avena", 200, 90},
        {"galletas",   "Galleta-chocolate", 180, 100},
        {"galletas",   "Galleta-vainilla", 220, 80},
        {"galletas",   "Galleta-integral", 150, 110},
        {"galletas",   "Galleta-coco", 160, 100},
        {"galletas",   "Galleta-jengibre", 140, 95},
        {"galletas",   "Galleta-mantequilla", 170, 105},
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
        {"sandwiches", "Sandwich-jamon", 45, 300},
        {"sandwiches", "Sandwich-pollo", 35, 350},
        {"sandwiches", "Sandwich-atun", 30, 325},
        {"sandwiches", "Sandwich-queso", 40, 400},
        {"sandwiches", "Sandwich-vegetal", 25, 375},
        {"sandwiches", "Club-sandwich", 20, 500},
        {"sandwiches", "Panini-caprese", 20, 475},
        {"ensaladas",  "Ensalada-cesar", 25, 550},
        {"ensaladas",  "Ensalada-griega", 20, 525},
        {"ensaladas",  "Ensalada-frutas", 30, 400},
        {"ensaladas",  "Ensalada-pollo", 20, 600},
        {"ensaladas",  "Ensalada-atun", 18, 575},
        {"ensaladas",  "Ensalada-mixta", 35, 450},
        {"ensaladas",  "Ensalada-caprese", 22, 500},
        {"sopas",      "Sopa-de-pollo", 30, 450},
        {"sopas",      "Sopa-de-tomate", 25, 400},
        {"sopas",      "Sopa-negra", 28, 425},
        {"sopas",      "Crema-de-hongos", 20, 475},
        {"sopas",      "Sopa-de-mariscos", 12, 750},
        {"sopas",      "Crema-de-zapallo", 22, 420},
        {"sopas",      "Sopa-azteca", 18, 460},
        {"jugos",      "Jugo-naranja", 60, 300},
        {"jugos",      "Jugo-zanahoria", 35, 325},
        {"jugos",      "Jugo-pina", 40, 300},
        {"jugos",      "Jugo-verde", 30, 375},
        {"jugos",      "Jugo-mora", 45, 325},
        {"jugos",      "Jugo-maracuya", 38, 325},
        {"jugos",      "Jugo-papaya", 42, 300},
        {"infusiones", "Te-verde", 80, 200},
        {"infusiones", "Te-negro", 80, 200},
        {"infusiones", "Te-manzanilla", 70, 180},
        {"infusiones", "Te-frio-limon", 50, 250},
        {"infusiones", "Te-chai", 40, 300},
        {"infusiones", "Chocolate-caliente", 55, 325},
        {"infusiones", "Agua-dulce", 90, 150},
        {"desayunos",  "Gallo-pinto", 40, 500},
        {"desayunos",  "Huevos-revueltos", 35, 450},
        {"desayunos",  "Pancakes", 30, 550},
        {"desayunos",  "Tostadas-aguacate", 25, 525},
        {"desayunos",  "Omelette-queso", 22, 575},
        {"desayunos",  "Tamal-asado", 45, 350},
        {"desayunos",  "Avena-con-frutas", 38, 400},
        {"almuerzos",  "Casado-pollo", 20, 850},
        {"almuerzos",  "Casado-carne", 18, 950},
        {"almuerzos",  "Casado-pescado", 15, 1000},
        {"almuerzos",  "Arroz-con-pollo", 22, 800},
        {"almuerzos",  "Chifrijo", 16, 900},
        {"almuerzos",  "Olla-de-carne", 12, 1100},
        {"almuerzos",  "Pasta-bolonesa", 20, 850},
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