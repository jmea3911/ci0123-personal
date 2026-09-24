// test_almacenamiento.cc
//
// Programa de prueba para las primitivas de Almacenamiento:
//   1. Crea cafeteria.img desde cero con el catalogo real.
//   2. Lista los productos de una categoria.
//   3. Busca un producto especifico.
//   4. Agrega productos hasta forzar el crecimiento de una categoria
//      (probar que AsignarBloque + BloqueIndice expandible funcionan).
//   5. Cierra y vuelve a abrir el archivo para confirmar persistencia.

#include <cstring>
#include <iomanip>
#include <iostream>

#include "Almacenamiento.h"

void ImprimirProducto(const RegistroProducto &r) {
    std::string desc(r.descripcion, strnlen(r.descripcion, sizeof(r.descripcion)));
    std::cout << "  - " << std::left << std::setw(24) << desc << " $" << std::fixed
              << std::setprecision(2) << (r.precio / 100.0) << "  stock=" << r.cantidad << "\n";
}

int main() {
    Almacenamiento disco;

    std::cout << "== 1. Creando cafeteria.img ==\n";
    if (!disco.Crear("cafeteria.img")) {
        std::cerr << "No se pudo crear cafeteria.img\n";
        return 1;
    }
    std::cout << "OK\n\n";

    std::cout << "== 2. Productos en categoria 'postr' (Postres) ==\n";
    for (const RegistroProducto &r : disco.ListarProductos("postr")) {
        ImprimirProducto(r);
    }
    std::cout << "\n";

    std::cout << "== 3. Buscando 'Tiramisu' en 'postr' ==\n";
    RegistroProducto encontrado{};
    if (disco.BuscarProducto("postr", "Tiramisu", &encontrado)) {
        ImprimirProducto(encontrado);
    } else {
        std::cout << "  No encontrado\n";
    }
    std::cout << "\n";

    std::cout << "== 4. Forzando crecimiento de 'caram' (Caramelos) ==\n";
    std::cout << "  Productos antes: " << disco.ListarProductos("caram").size() << "\n";

    // 'caram' ya tiene 10/16 slots ocupados (2 bloques de datos).
    // Agregamos 10 productos mas para forzar que pida un bloque nuevo.
    for (int i = 1; i <= 10; i++) {
        RegistroProducto nuevo{};
        std::string nombre = "Caramelo extra " + std::to_string(i);
        std::strncpy(nuevo.descripcion, nombre.c_str(), sizeof(nuevo.descripcion) - 1);
        nuevo.cantidad = 50;
        nuevo.precio = 25;  // $0.25
        nuevo.estado = 1;

        bool ok = disco.AgregarProducto("caram", nuevo);
        std::cout << "  Agregar '" << nombre << "': " << (ok ? "OK" : "FALLO") << "\n";
    }

    std::cout << "  Productos despues: " << disco.ListarProductos("caram").size() << "\n";
    std::cout << "  (deberian ser 20: 10 originales + 10 nuevos, repartidos en 3 bloques de datos)\n\n";

    disco.Cerrar();

    std::cout << "== 5. Reabriendo cafeteria.img para confirmar persistencia ==\n";
    Almacenamiento disco2;
    if (!disco2.Abrir("cafeteria.img")) {
        std::cerr << "No se pudo reabrir cafeteria.img\n";
        return 1;
    }
    auto productos_caram = disco2.ListarProductos("caram");
    std::cout << "  'caram' tiene " << productos_caram.size() << " productos tras reabrir\n";

    RegistroProducto extra{};
    if (disco2.BuscarProducto("caram", "Caramelo extra 7", &extra)) {
        std::cout << "  Encontrado tras reabrir: ";
        ImprimirProducto(extra);
    } else {
        std::cout << "  ERROR: no se encontro 'Caramelo extra 7' tras reabrir\n";
    }

    return 0;
}
