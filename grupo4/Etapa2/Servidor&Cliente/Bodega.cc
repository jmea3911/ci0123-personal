#include "Bodega.h"
#include <cstring>
#include <cctype>
#include <iostream>
 
static std::string aMinusculas(const std::string& s) {
    std::string r = s;
    for (char& c : r) c = (char)tolower((unsigned char)c);
    return r;
}


Bodega::Bodega(Filesystem& fs, uint16_t bloqueDirCategorias)
    : fs_(fs), bloqueDirCategorias_(bloqueDirCategorias) {}

// Busca una categoria recorriendo los bloques del directorio.
bool Bodega::ubicarCasillaCategoria(const std::string& nombreBuscado, int& bloqueOut, int& posOut) {
    std::string buscadoMin = aMinusculas(nombreBuscado); // sin distinguir mayusculas/minusculas
    int bloqueActual = bloqueDirCategorias_;
    while (bloqueActual != BLOQUE_NULO) {
        BloqueDirCategorias dir;
        fs_.leerBloque(bloqueActual, &dir);

        for (int i = 0; i < CASILLAS_POR_BLOQUE_DIR; i++) {
            if (dir.casillas[i].estado == 1 &&
                buscadoMin == aMinusculas(std::string(dir.casillas[i].nombre, strnlen(dir.casillas[i].nombre, sizeof(dir.casillas[i].nombre))))) {
                bloqueOut = bloqueActual;
                posOut = i;
                return true;
            }
        }
        bloqueActual = dir.encabezado.siguienteBloque;
    }
    return false;
}

// Crea una nueva categoria y reserva el bloque donde se almacenara su indice.
void Bodega::crearCategoria(const std::string& nombre, const std::string& id) {
    std::lock_guard<std::mutex> lock(fs_.mutex());

    int bloqueActual = bloqueDirCategorias_;
    BloqueDirCategorias dir{};

    while (true) {
        fs_.leerBloque(bloqueActual, &dir);

        for (int i = 0; i < CASILLAS_POR_BLOQUE_DIR; i++) {
            if (dir.casillas[i].estado == 0) {
                int bloqueIndice = fs_.pedirBloque();
                if (bloqueIndice == -1) {
                    std::cerr << "no hay bloques libres para crear la categoria\n";
                    return;
                }

                // Guarda los datos de la nueva categoria en el directorio.
                DirCategoria nueva{};
                std::strncpy(nueva.nombre, nombre.c_str(), sizeof(nueva.nombre) - 1);
                std::strncpy(nueva.id, id.c_str(), sizeof(nueva.id) - 1);
                nueva.estado = 1;
                nueva.bloqueIndice = bloqueIndice;
                dir.casillas[i] = nueva;
                dir.encabezado.casillasUsadas++;
                fs_.escribirBloque(bloqueActual, &dir);

                // Inicializa el bloque indice de la categoria.
                BloqueIndice indiceVacio{};
                indiceVacio.cantidadBloques = 0;
                indiceVacio.cantidadProductos = 0;
                fs_.escribirBloque(bloqueIndice, &indiceVacio);
                return;
            }
        }

        // Si el bloque esta lleno, continua con el siguiente bloque del directorio.
        if (dir.encabezado.siguienteBloque != BLOQUE_NULO) {
            bloqueActual = dir.encabezado.siguienteBloque;
            continue;
        }

        // Si no existe otro bloque, reserva uno nuevo para ampliar el directorio.
        int nuevoBloque = fs_.pedirBloque();
        if (nuevoBloque == -1) {
            std::cerr << "no hay bloques libres para crecer el directorio de categorias\n";
            return;
        }
        dir.encabezado.siguienteBloque = nuevoBloque;
        fs_.escribirBloque(bloqueActual, &dir);

        BloqueDirCategorias nuevoDir{};
        nuevoDir.encabezado.siguienteBloque = BLOQUE_NULO;
        nuevoDir.encabezado.casillasUsadas = 0;
        fs_.escribirBloque(nuevoBloque, &nuevoDir);

        bloqueActual = nuevoBloque;
    }
}

// Busca una categoria por su identificador y devuelve sus datos.
bool Bodega::buscarCategoria(const std::string& id, DirCategoria& resultado) {
    std::lock_guard<std::mutex> lock(fs_.mutex());

    int bloque, pos;
    if (!ubicarCasillaCategoria(id, bloque, pos)) return false;

    BloqueDirCategorias dir;
    fs_.leerBloque(bloque, &dir);
    resultado = dir.casillas[pos];
    return true;
}

// Agrega un producto buscando primero un espacio libre en los bloques existentes.
bool Bodega::agregarProducto(const std::string& idCategoria, const std::string& nombreProducto, int cantidad, uint32_t precioCentavos) {
    std::lock_guard<std::mutex> lock(fs_.mutex());

    int bloqueCasilla, posCasilla;
    if (!ubicarCasillaCategoria(idCategoria, bloqueCasilla, posCasilla)) {
        std::cerr << "categoria no encontrada: " << idCategoria << "\n";
        return false;
    }

    BloqueDirCategorias dirCat;
    fs_.leerBloque(bloqueCasilla, &dirCat);
    DirCategoria categoria = dirCat.casillas[posCasilla];

    BloqueIndice indice;
    fs_.leerBloque(categoria.bloqueIndice, &indice);

    int posicionLibre = -1;
    int bloqueDestino = -1;

    // buscar espacio en los bloques de datos que ya tiene la categoria
    for (int b = 0; b < indice.cantidadBloques && bloqueDestino == -1; b++) {
        RegistroProducto bloqueDatos[PRODUCTOS_POR_BLOQUE];
        fs_.leerBloque(indice.bloquesDatos[b], bloqueDatos);
        for (int p = 0; p < PRODUCTOS_POR_BLOQUE; p++) {
            if (bloqueDatos[p].estado == 0) {
                bloqueDestino = indice.bloquesDatos[b];
                posicionLibre = p;
                break;
            }
        }
    }

    // si no hay espacio, pedir un bloque nuevo y agregarlo al indice
    if (bloqueDestino == -1) {
        int nuevoBloque = fs_.pedirBloque();
        if (nuevoBloque == -1) {
            std::cerr << "no hay bloques libres para mas productos\n";
            return false;
        }
        indice.bloquesDatos[indice.cantidadBloques] = nuevoBloque;
        indice.cantidadBloques++;
        bloqueDestino = nuevoBloque;
        posicionLibre = 0;

        RegistroProducto bloqueNuevo[PRODUCTOS_POR_BLOQUE] = {};
        fs_.escribirBloque(bloqueDestino, bloqueNuevo);
    }

    // Escribe el nuevo producto en la posicion disponible.
    RegistroProducto bloqueDatos[PRODUCTOS_POR_BLOQUE];
    fs_.leerBloque(bloqueDestino, bloqueDatos);

    RegistroProducto nuevo{};
    std::strncpy(nuevo.nombre, nombreProducto.c_str(), sizeof(nuevo.nombre) - 1);
    nuevo.cantidad = cantidad;
    nuevo.precioCentavos = precioCentavos;
    nuevo.estado = 1;
    bloqueDatos[posicionLibre] = nuevo;
    fs_.escribirBloque(bloqueDestino, bloqueDatos);

    // Actualiza la cantidad de productos almacenados en el indice.
    indice.cantidadProductos++;
    fs_.escribirBloque(categoria.bloqueIndice, &indice);
    return true;
}

// Devuelve todas las categorias activas de la bodega.
std::vector<DirCategoria> Bodega::listarCategorias() {
    std::lock_guard<std::mutex> lock(fs_.mutex());

    std::vector<DirCategoria> categorias;
    int bloqueActual = bloqueDirCategorias_;
    while (bloqueActual != BLOQUE_NULO) {
        BloqueDirCategorias dir;
        fs_.leerBloque(bloqueActual, &dir);
        for (int i = 0; i < CASILLAS_POR_BLOQUE_DIR; i++) {
            if (dir.casillas[i].estado == 1) categorias.push_back(dir.casillas[i]);
        }
        bloqueActual = dir.encabezado.siguienteBloque;
    }
    return categorias;
}

// Devuelve todos los productos activos de una categoria.
std::vector<RegistroProducto> Bodega::listarProductos(const std::string& idCategoria) {
    std::lock_guard<std::mutex> lock(fs_.mutex());

    std::vector<RegistroProducto> productos;
    int bloqueCasilla, posCasilla;
    if (!ubicarCasillaCategoria(idCategoria, bloqueCasilla, posCasilla)) return productos;

    BloqueDirCategorias dirCat;
    fs_.leerBloque(bloqueCasilla, &dirCat);
    DirCategoria categoria = dirCat.casillas[posCasilla];

    BloqueIndice indice;
    fs_.leerBloque(categoria.bloqueIndice, &indice);

    for (int b = 0; b < indice.cantidadBloques; b++) {
        RegistroProducto bloqueDatos[PRODUCTOS_POR_BLOQUE];
        fs_.leerBloque(indice.bloquesDatos[b], bloqueDatos);
        for (int p = 0; p < PRODUCTOS_POR_BLOQUE; p++) {
            if (bloqueDatos[p].estado == 1) productos.push_back(bloqueDatos[p]);
        }
    }
    return productos;
}

// Ajusta la cantidad disponible de un producto mediante un incremento o decremento.
bool Bodega::ajustarCantidad(const std::string& idCategoria, const std::string& nombreProducto, int delta) {
    std::lock_guard<std::mutex> lock(fs_.mutex());

    int bloqueCasilla, posCasilla;
    if (!ubicarCasillaCategoria(idCategoria, bloqueCasilla, posCasilla)) return false;

    BloqueDirCategorias dirCat;
    fs_.leerBloque(bloqueCasilla, &dirCat);
    DirCategoria categoria = dirCat.casillas[posCasilla];

    BloqueIndice indice;
    fs_.leerBloque(categoria.bloqueIndice, &indice);

    std::string buscadoMin = aMinusculas(nombreProducto); // sin distinguir mayusculas/minusculas
    for (int b = 0; b < indice.cantidadBloques; b++) {
        RegistroProducto bloqueDatos[PRODUCTOS_POR_BLOQUE];
        fs_.leerBloque(indice.bloquesDatos[b], bloqueDatos);
        for (int p = 0; p < PRODUCTOS_POR_BLOQUE; p++) {
            if (bloqueDatos[p].estado != 1) continue;
            std::string nombre(bloqueDatos[p].nombre, strnlen(bloqueDatos[p].nombre, sizeof(bloqueDatos[p].nombre)));
            if (aMinusculas(nombre) != buscadoMin) continue;

            int nueva = (int)bloqueDatos[p].cantidad + delta;
            if (nueva < 0) return false; // no hay suficiente para restar esto
            bloqueDatos[p].cantidad = (uint16_t)nueva;
            fs_.escribirBloque(indice.bloquesDatos[b], bloqueDatos);
            return true;
        }
    }
    return false;
}