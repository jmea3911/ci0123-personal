#include "Almacenamiento.h"
#include <cstring>
#include <iostream>

// Catalogo inicial: 5 categorias x 10 productos, tomado del diseno
// real del equipo (Diseno_Filesystem_Cafeteria_Grupo4).

namespace {

struct CategoriaDef {
    const char *nombre;
    const char *id;
    uint8_t bloque_indice;
};

// Orden fijo de categorias y sus bloques de indice
const CategoriaDef kCategorias[5] = {
    {"Bebidas frias", "bebfr", 3},
    {"Bebidas calientes", "bebca", 6},
    {"Postres", "postr", 9},
    {"Caramelos", "caram", 12},
    {"Reposteria", "repos", 15},
};

struct ProductoDef {
    const char *categoria_id;
    const char *descripcion;
    double precio;
    int stock;
};

const ProductoDef kCatalogo[] = {
    // Bebidas frias
    {"bebfr", "Cafe frio", 3.25, 40},
    {"bebfr", "Frappe de cafe", 3.75, 35},
    {"bebfr", "Limonada", 2.50, 60},
    {"bebfr", "Te helado", 2.75, 55},
    {"bebfr", "Smoothie de fresa", 4.00, 30},
    {"bebfr", "Jugo de naranja", 2.60, 50},
    {"bebfr", "Agua embotellada", 1.50, 90},
    {"bebfr", "Refresco de cola", 2.00, 70},
    {"bebfr", "Horchata fria", 3.00, 25},
    {"bebfr", "Chocolate frio", 3.50, 20},
    // Bebidas calientes
    {"bebca", "Cafe negro", 2.00, 100},
    {"bebca", "Cafe con leche", 2.50, 90},
    {"bebca", "Cappuccino", 3.25, 60},
    {"bebca", "Latte", 3.50, 55},
    {"bebca", "Espresso", 2.25, 70},
    {"bebca", "Te caliente", 2.00, 65},
    {"bebca", "Chocolate caliente", 3.00, 45},
    {"bebca", "Capuchino de vainilla", 3.75, 30},
    {"bebca", "Mocha", 3.80, 28},
    {"bebca", "Te de manzanilla", 2.10, 40},
    // Postres
    {"postr", "Flan de caramelo", 3.20, 20},
    {"postr", "Tres leches", 3.80, 15},
    {"postr", "Gelatina", 1.80, 30},
    {"postr", "Arroz con leche", 2.90, 25},
    {"postr", "Budin de pan", 3.00, 18},
    {"postr", "Mousse de chocolate", 3.50, 12},
    {"postr", "Cheesecake", 4.25, 15},
    {"postr", "Tiramisu", 4.50, 10},
    {"postr", "Brownie con helado", 4.00, 20},
    {"postr", "Copa de frutas", 2.75, 22},
    // Caramelos
    {"caram", "Caramelo de menta", 0.25, 300},
    {"caram", "Caramelo de cafe", 0.25, 250},
    {"caram", "Caramelo de leche", 0.30, 200},
    {"caram", "Caramelo de frutas", 0.25, 280},
    {"caram", "Chicle de menta", 0.20, 150},
    {"caram", "Paleta de caramelo", 0.50, 100},
    {"caram", "Caramelo de chocolate", 0.35, 180},
    {"caram", "Caramelo acido", 0.30, 160},
    {"caram", "Toffee", 0.40, 90},
    {"caram", "Caramelo de miel", 0.35, 120},
    // Reposteria
    {"repos", "Croissant", 2.20, 40},
    {"repos", "Queque seco", 2.00, 22},
    {"repos", "Brownie", 2.50, 30},
    {"repos", "Galleta de avena", 1.50, 60},
    {"repos", "Cachito de jamon", 2.80, 35},
    {"repos", "Empanada de pina", 1.90, 45},
    {"repos", "Pan dulce", 1.60, 50},
    {"repos", "Muffin de arandanos", 2.75, 28},
    {"repos", "Dona glaseada", 2.10, 33},
    {"repos", "Rollo de canela", 3.00, 20},
};

const int kNumProductos = sizeof(kCatalogo) / sizeof(kCatalogo[0]);

void CopiarString(char *destino, size_t tam_destino, const std::string &origen) {
    std::memset(destino, 0, tam_destino);
    std::strncpy(destino, origen.c_str(), tam_destino - 1);
}

}  

// Ciclo de vida

Almacenamiento::~Almacenamiento() { Cerrar(); }

void Almacenamiento::Cerrar() {
    if (disco_.is_open()) disco_.close();
}

RegistroProducto Almacenamiento::HacerRegistro(const std::string &descripcion, double precio_dolares, int cantidad) {
    RegistroProducto r{};
    CopiarString(r.descripcion, sizeof(r.descripcion), descripcion);
    r.cantidad = static_cast<uint16_t>(cantidad);
    r.precio = static_cast<uint32_t>(precio_dolares * 100 + 0.5);  // a centavos
    r.estado = 1;
    r.reservado = 0;
    return r;
}

bool Almacenamiento::Crear(const std::string &ruta) {
    // 1. Crear el archivo con NUM_BLOQUES bloques vacios
    disco_.open(ruta, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!disco_.is_open()) return false;

    char bloque_vacio[TAMANO_BLOQUE] = {0};
    for (int i = 0; i < NUM_BLOQUES; i++) {
        disco_.write(bloque_vacio, TAMANO_BLOQUE);
    }
    disco_.close();

    disco_.open(ruta, std::ios::in | std::ios::out | std::ios::binary);
    if (!disco_.is_open()) return false;

    // 2. Bitmap: marcar como ocupados los bloques 0-2 (sistema) y los
    //    bloques de indice + datos de las 5 categorias (3 al 17).
    BloqueBitmap bm{};
    for (int b = 0; b <= 17; b++) MarcarBit(&bm, b, true);
    EscribirBitmap(bm);

    // 3. Superbloque.
    Superbloque sb{};
    std::memcpy(sb.magic, "CAFE", 4);
    CopiarString(sb.nombre_negocio, sizeof(sb.nombre_negocio), "Cafeteria");
    sb.num_categorias = 5;
    sb.bloques_libres = NUM_BLOQUES - 18;  // 238 libres al inicio
    EscribirBloque(BLOQUE_SUPERBLOQUE, &sb);

    // 4. Directorio de categorias.
    BloqueDirectorio dir{};
    for (int i = 0; i < 5; i++) {
        EntradaCategoria &e = dir.entradas[i];
        CopiarString(e.nombre, sizeof(e.nombre), kCategorias[i].nombre);
        CopiarString(e.id, sizeof(e.id), kCategorias[i].id);
        e.estado = 1;
        e.bloque_indice = kCategorias[i].bloque_indice;
        e.n_productos = 10;
        e.reservado = 0;
    }
    EscribirBloque(BLOQUE_DIRECTORIO, &dir);

    // 5. Por cada categoria: bloque de indice (2 bloques de datos fijos
    //    al inicio) + los bloques de datos con los 10 productos.
    for (const CategoriaDef &cat : kCategorias) {
        uint8_t bloque_datos_1 = cat.bloque_indice + 1;
        uint8_t bloque_datos_2 = cat.bloque_indice + 2;

        BloqueIndice idx{};
        idx.num_bloques_datos = 2;
        idx.bloques_datos[0] = bloque_datos_1;
        idx.bloques_datos[1] = bloque_datos_2;
        EscribirBloque(cat.bloque_indice, &idx);

        // Juntar los productos de esta categoria del catalogo.
        std::vector<RegistroProducto> productos;
        for (const ProductoDef &p : kCatalogo) {
            if (std::string(p.categoria_id) == cat.id) {
                productos.push_back(HacerRegistro(p.descripcion, p.precio, p.stock));
            }
        }

        // Repartir en los dos bloques de datos (8 registros c/u).
        uint8_t bloques[2] = {bloque_datos_1, bloque_datos_2};
        for (size_t i = 0; i < productos.size(); i++) {
            int bloque_idx = i / PRODUCTOS_POR_BLOQUE_DATOS;
            int slot = i % PRODUCTOS_POR_BLOQUE_DATOS;

            RegistroProducto buffer_bloque[PRODUCTOS_POR_BLOQUE_DATOS];
            if (slot == 0) {
                std::memset(buffer_bloque, 0, sizeof(buffer_bloque));
            } else {
                LeerBloque(bloques[bloque_idx], buffer_bloque);
            }
            buffer_bloque[slot] = productos[i];
            EscribirBloque(bloques[bloque_idx], buffer_bloque);
        }
    }

    return true;
}

bool Almacenamiento::Abrir(const std::string &ruta) {
    disco_.open(ruta, std::ios::in | std::ios::out | std::ios::binary);
    if (!disco_.is_open()) return false;

    Superbloque sb{};
    if (!LeerBloque(BLOQUE_SUPERBLOQUE, &sb)) return false;
    if (std::memcmp(sb.magic, "CAFE", 4) != 0) {
        std::cerr << "Archivo no tiene el formato 'CAFE' esperado\n";
        return false;
    }
    return true;
}


// Las 4 primitivas de bloque


bool Almacenamiento::LeerBloque(int num_bloque, void *buffer) {
    if (num_bloque < 0 || num_bloque >= NUM_BLOQUES) return false;
    disco_.seekg(static_cast<std::streamoff>(num_bloque) * TAMANO_BLOQUE);
    disco_.read(reinterpret_cast<char *>(buffer), TAMANO_BLOQUE);
    return static_cast<bool>(disco_);
}

bool Almacenamiento::EscribirBloque(int num_bloque, const void *buffer) {
    if (num_bloque < 0 || num_bloque >= NUM_BLOQUES) return false;
    disco_.seekp(static_cast<std::streamoff>(num_bloque) * TAMANO_BLOQUE);
    disco_.write(reinterpret_cast<const char *>(buffer), TAMANO_BLOQUE);
    disco_.flush();
    return static_cast<bool>(disco_);
}

int Almacenamiento::AsignarBloque() {
    BloqueBitmap bm{};
    if (!LeerBitmap(&bm)) return -1;

    for (int b = 0; b < NUM_BLOQUES; b++) {
        if (!BitOcupado(bm, b)) {
            MarcarBit(&bm, b, true);
            EscribirBitmap(bm);

            Superbloque sb{};
            LeerBloque(BLOQUE_SUPERBLOQUE, &sb);
            if (sb.bloques_libres > 0) sb.bloques_libres--;
            EscribirBloque(BLOQUE_SUPERBLOQUE, &sb);

            return b;
        }
    }
    return -1;  // disco lleno
}

void Almacenamiento::LiberarBloque(int num_bloque) {
    BloqueBitmap bm{};
    if (!LeerBitmap(&bm)) return;
    MarcarBit(&bm, num_bloque, false);
    EscribirBitmap(bm);

    Superbloque sb{};
    LeerBloque(BLOQUE_SUPERBLOQUE, &sb);
    sb.bloques_libres++;
    EscribirBloque(BLOQUE_SUPERBLOQUE, &sb);
}


// Bitmap: helpers internos


bool Almacenamiento::LeerBitmap(BloqueBitmap *bm) { return LeerBloque(BLOQUE_BITMAP, bm); }

bool Almacenamiento::EscribirBitmap(const BloqueBitmap &bm) { return EscribirBloque(BLOQUE_BITMAP, &bm); }

bool Almacenamiento::BitOcupado(const BloqueBitmap &bm, int num_bloque) {
    int byte_idx = num_bloque / 8;
    int bit_idx = num_bloque % 8;
    return (bm.bits[byte_idx] >> bit_idx) & 1;
}

void Almacenamiento::MarcarBit(BloqueBitmap *bm, int num_bloque, bool ocupado) {
    int byte_idx = num_bloque / 8;
    int bit_idx = num_bloque % 8;
    if (ocupado) {
        bm->bits[byte_idx] |= (1 << bit_idx);
    } else {
        bm->bits[byte_idx] &= ~(1 << bit_idx);
    }
}


// Operaciones de alto nivel


bool Almacenamiento::BuscarCategoria(const std::string &id_categoria, EntradaCategoria *out, int *indice_dir) {
    BloqueDirectorio dir{};
    if (!LeerBloque(BLOQUE_DIRECTORIO, &dir)) return false;

    for (int i = 0; i < MAX_CATEGORIAS; i++) {
        EntradaCategoria &e = dir.entradas[i];
        if (e.estado == 1 && std::string(e.id, strnlen(e.id, sizeof(e.id))) == id_categoria) {
            *out = e;
            if (indice_dir) *indice_dir = i;
            return true;
        }
    }
    return false;
}

std::vector<RegistroProducto> Almacenamiento::ListarProductos(const std::string &id_categoria) {
    std::vector<RegistroProducto> resultado;

    EntradaCategoria cat{};
    if (!BuscarCategoria(id_categoria, &cat)) return resultado;

    BloqueIndice idx{};
    if (!LeerBloque(cat.bloque_indice, &idx)) return resultado;

    for (int i = 0; i < idx.num_bloques_datos; i++) {
        RegistroProducto buffer_bloque[PRODUCTOS_POR_BLOQUE_DATOS];
        if (!LeerBloque(idx.bloques_datos[i], buffer_bloque)) continue;

        for (const RegistroProducto &r : buffer_bloque) {
            if (r.estado == 1) resultado.push_back(r);
        }
    }
    return resultado;
}

bool Almacenamiento::BuscarProducto(const std::string &id_categoria, const std::string &descripcion, RegistroProducto *out) {
    for (const RegistroProducto &r : ListarProductos(id_categoria)) {
        if (std::string(r.descripcion, strnlen(r.descripcion, sizeof(r.descripcion))) == descripcion) {
            *out = r;
            return true;
        }
    }
    return false;
}

bool Almacenamiento::AgregarProducto(const std::string &id_categoria, const RegistroProducto &prod) {
    int indice_dir = -1;
    EntradaCategoria cat{};
    if (!BuscarCategoria(id_categoria, &cat, &indice_dir)) return false;

    BloqueIndice idx{};
    if (!LeerBloque(cat.bloque_indice, &idx)) return false;

    // 1. Buscar un slot libre en los bloques de datos ya asignados.
    for (int i = 0; i < idx.num_bloques_datos; i++) {
        RegistroProducto buffer_bloque[PRODUCTOS_POR_BLOQUE_DATOS];
        if (!LeerBloque(idx.bloques_datos[i], buffer_bloque)) continue;

        for (int s = 0; s < PRODUCTOS_POR_BLOQUE_DATOS; s++) {
            if (buffer_bloque[s].estado == 0) {
                buffer_bloque[s] = prod;
                EscribirBloque(idx.bloques_datos[i], buffer_bloque);

                cat.n_productos++;
                BloqueDirectorio dir{};
                LeerBloque(BLOQUE_DIRECTORIO, &dir);
                dir.entradas[indice_dir] = cat;
                EscribirBloque(BLOQUE_DIRECTORIO, &dir);
                return true;
            }
        }
    }

    // 2. No hay espacio: pedir un bloque nuevo y agregarlo al indice
    //    (esto es lo que permite que la categoria "crezca").
    int nuevo_bloque = AsignarBloque();
    if (nuevo_bloque < 0) return false;  // disco lleno

    if (idx.num_bloques_datos >= static_cast<int>(sizeof(idx.bloques_datos))) {
        LiberarBloque(nuevo_bloque);
        return false;  // indice lleno (no deberia pasar con 255 entradas)
    }

    RegistroProducto buffer_nuevo[PRODUCTOS_POR_BLOQUE_DATOS] = {};
    buffer_nuevo[0] = prod;
    EscribirBloque(nuevo_bloque, buffer_nuevo);

    idx.bloques_datos[idx.num_bloques_datos] = static_cast<uint8_t>(nuevo_bloque);
    idx.num_bloques_datos++;
    EscribirBloque(cat.bloque_indice, &idx);

    cat.n_productos++;
    BloqueDirectorio dir{};
    LeerBloque(BLOQUE_DIRECTORIO, &dir);
    dir.entradas[indice_dir] = cat;
    EscribirBloque(BLOQUE_DIRECTORIO, &dir);

    return true;
}
