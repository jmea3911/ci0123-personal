// Almacenamiento.h
//
//
// Distribucion del disco (256 bloques de 256 bytes = 64 KB):
//   bloque 0   -> superbloque
//   bloque 1   -> mapa de bits
//   bloque 2   -> directorio de categorias (8 entradas x 32 bytes)
//   bloque 3+  -> indice y datos de cada categoria
//
// Diferencia respecto al diseno original en papel: el bloque de
// indice de cada categoria no fija "siempre 2 bloques de datos",
// sino que guarda una LISTA de bloques de datos. Esto permite que
// una categoria crezca (pida bloques nuevos al bitmap) sin tener
// que reacomodar el directorio ni las demas categorias.

#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

// Constantes del disco

constexpr int TAMANO_BLOQUE = 256;
constexpr int NUM_BLOQUES = 256;

constexpr int BLOQUE_SUPERBLOQUE = 0;
constexpr int BLOQUE_BITMAP = 1;
constexpr int BLOQUE_DIRECTORIO = 2;

constexpr int MAX_CATEGORIAS = 8;                       // caben en 1 bloque de 256B / 32B
constexpr int PRODUCTOS_POR_BLOQUE_DATOS = TAMANO_BLOQUE / 32;  // 8 registros de 32 bytes

// Estructuras en disco (cada una ocupa exactamente un bloque, salvo
// EntradaCategoria y RegistroProducto que son registros de 32 bytes
// dentro de un bloque)

#pragma pack(push, 1)

struct Superbloque {
    char magic[4];            // "CAFE"
    char nombre_negocio[20];  // "Cafeteria"
    uint8_t num_categorias;
    uint8_t bloques_libres;
    char reservado[230];
};
static_assert(sizeof(Superbloque) == TAMANO_BLOQUE, "Superbloque debe ocupar un bloque completo");

struct EntradaCategoria {
    char nombre[20];         // offset 0
    char id[8];               // offset 20
    uint8_t estado;           // offset 28 (0 libre, 1 ocupada)
    uint8_t bloque_indice;    // offset 29
    uint8_t n_productos;      // offset 30
    uint8_t reservado;        // offset 31
};
static_assert(sizeof(EntradaCategoria) == 32, "Entrada de categoria debe ser de 32 bytes");

struct BloqueDirectorio {
    EntradaCategoria entradas[MAX_CATEGORIAS];
};
static_assert(sizeof(BloqueDirectorio) == TAMANO_BLOQUE, "Directorio debe ocupar un bloque completo");

struct BloqueBitmap {
    uint8_t bits[32];       // 256 bits = 256 bloques
    uint8_t reservado[224];
};
static_assert(sizeof(BloqueBitmap) == TAMANO_BLOQUE, "Bitmap debe ocupar un bloque completo");

// Bloque de indice de una categoria: lista expandible de bloques de datos.
struct BloqueIndice {
    uint8_t num_bloques_datos;
    uint8_t bloques_datos[255];  // numeros de bloque; se usan los primeros num_bloques_datos
};
static_assert(sizeof(BloqueIndice) == TAMANO_BLOQUE, "Bloque de indice debe ocupar un bloque completo");

struct RegistroProducto {
    char descripcion[24];  // offset 0
    uint16_t cantidad;      // offset 24
    uint32_t precio;        // offset 26 (centavos de dolar)
    uint8_t estado;         // offset 30 (0 libre, 1 ocupada)
    uint8_t reservado;      // offset 31
};
static_assert(sizeof(RegistroProducto) == 32, "Registro de producto debe ser de 32 bytes");

#pragma pack(pop)

// Clase Almacenamiento

class Almacenamiento {
   public:
    Almacenamiento() = default;
    ~Almacenamiento();

    // Crea cafeteria.img desde cero: superbloque, bitmap, directorio
    // con las 5 categorias fijas del diseno, y el catalogo inicial de
    // 50 productos (10 por categoria).
    bool Crear(const std::string &ruta);

    // Abre un cafeteria.img ya existente.
    bool Abrir(const std::string &ruta);

    void Cerrar();

    // Las 4 primitivas de bloque 
    bool LeerBloque(int num_bloque, void *buffer);
    bool EscribirBloque(int num_bloque, const void *buffer);
    int AsignarBloque();               // busca el primer bit libre, lo marca, retorna el # de bloque (-1 si no hay espacio)
    void LiberarBloque(int num_bloque);

    bool BuscarCategoria(const std::string &id_categoria, EntradaCategoria *out, int *indice_dir = nullptr);
    std::vector<RegistroProducto> ListarProductos(const std::string &id_categoria);
    bool BuscarProducto(const std::string &id_categoria, const std::string &descripcion, RegistroProducto *out);

    // Agrega un producto a una categoria. Si los bloques de datos
    // actuales estan llenos, pide un bloque nuevo (AsignarBloque) y
    // lo agrega a la lista del BloqueIndice: asi la categoria crece.
    bool AgregarProducto(const std::string &id_categoria, const RegistroProducto &prod);

   private:
    std::fstream disco_;

    bool LeerBitmap(BloqueBitmap *bm);
    bool EscribirBitmap(const BloqueBitmap &bm);
    static bool BitOcupado(const BloqueBitmap &bm, int num_bloque);
    static void MarcarBit(BloqueBitmap *bm, int num_bloque, bool ocupado);

    static RegistroProducto HacerRegistro(const std::string &descripcion, double precio_dolares, int cantidad);
};
