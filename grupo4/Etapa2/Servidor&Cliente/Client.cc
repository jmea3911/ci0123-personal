#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <cstring>

#include "VSocket.h"
#include "Socket.h"
#include "SSLSocket.h"
#include "protocolo.h"

//a quien le habla este cliente
static const std::string INTERMEDIO_HOST = "127.0.0.1";
static const int INTERMEDIO_PORT = 8080;
static const std::string MI_ID = "CLI_04";      // ID de este cliente
static const std::string DESTINO_INT = "INT_04"; // el intermediario de la isla

//carrito
struct ItemLocal { std::string nombre; int cantidad; };
static std::vector<ItemLocal> carritoLocal;

//funciones auxiliares

bool esNum(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) if (!isdigit((unsigned char)c)) return false;
    return true;
}

std::string aMinusculas(const std::string& s) {
    std::string r = s;
    for (char& c : r) c = (char)tolower((unsigned char)c);
    return r;
}

//lee toda la respuesta HTTP hasta que el servidor cierre la conexion
std::string leerRespuestaCompleta(VSocket* socket) {
    char buffer[4096];
    std::string acumulado;
    try {
        size_t leidos;
        while ((leidos = socket->Read(buffer, sizeof(buffer) - 1)) > 0) {
            acumulado.append(buffer, leidos);
        }
    } catch (const std::exception&) {
        //lo acumulado hasta aqui es lo que hay
    }
    return acumulado;
}

struct RespuestaIntermediario {
    bool exito = false;
    int codigoHTTP = -1;
    std::string mensajeProtocolo; // cuerpo ya extraido (ORIGEN|DESTINO/TIPO/...)
};

/**
 * enviarAlIntermediario - abre una conexion TCP, manda una peticion HTTP
 * POST con el mensaje del protocolo en el cuerpo, y devuelve la respuesta
 * ya separada en codigo HTTP + cuerpo.
 */
RespuestaIntermediario enviarAlIntermediario(const std::string& mensajeProtocolo) {
    RespuestaIntermediario resultado;
    VSocket* client = nullptr;
    try {
        client = new SSLSocket(false);

        if (client->Connect(INTERMEDIO_HOST.c_str(), INTERMEDIO_PORT) != 0) {
            throw std::runtime_error("Connect fallo");
        }
        std::string peticion = construirPeticionHTTP("/mensaje", INTERMEDIO_HOST, mensajeProtocolo);
        client->Write(peticion.c_str());

        std::string respuestaCompleta = leerRespuestaCompleta(client);
        delete client;
        client = nullptr;

        resultado.exito = true;
        resultado.codigoHTTP = extraerCodigoRespuesta(respuestaCompleta);
        resultado.mensajeProtocolo = extraerCuerpoRespuesta(respuestaCompleta);
    } catch (const std::exception& e) {
        std::cerr << "  (no se pudo hablar con el intermediario: " << e.what() << ")\n";
        if (client) delete client;
        resultado.exito = false;
    }
    return resultado;
}

//convierte "nombre,precio,stock;nombre,precio,stock;..." en filas para imprimir
void imprimirListaProductos(const std::string& lista) {
    auto items = splitV2(lista, ';');
    for (auto& item : items) {
        if (item.empty()) continue;
        auto campos = splitV2(item, ',');
        std::cout << "  - " << (campos.size() > 0 ? campos[0] : "?")
                   << " | precio: " << (campos.size() > 1 ? campos[1] : "?")
                   << " | stock: " << (campos.size() > 2 ? campos[2] : "?") << "\n";
    }
}

//pruebas

void pruebaCategorias() {
    std::string peticion = construirMensaje(MI_ID, DESTINO_INT, "10", {});
    std::cout << "\n[Cliente -> Intermediario] " << peticion << "\n";

    auto r = enviarAlIntermediario(peticion);
    if (!r.exito) {
        std::cout << "[Intermediario -> Cliente] ERR_COMM (no se pudo conectar)\n";
        return;
    }
    std::cout << "  (HTTP " << r.codigoHTTP << ")\n";
    std::cout << "[Intermediario -> Cliente] " << r.mensajeProtocolo << "\n";

    MensajeV2 resp = parsearMensaje(r.mensajeProtocolo);
    if (resp.tipo == "11" && !resp.campos.empty()) {
        std::cout << "  Categorias disponibles: " << resp.campos[0] << "\n";
    }
}

void pruebaListCat(const std::string& categoria) {
    std::string peticion = construirMensaje(MI_ID, DESTINO_INT, "20", {categoria});
    std::cout << "\n[Cliente -> Intermediario] " << peticion << "\n";

    auto r = enviarAlIntermediario(peticion);
    if (!r.exito) {
        std::cout << "[Intermediario -> Cliente] ERR_COMM (no se pudo conectar)\n";
        return;
    }
    std::cout << "  (HTTP " << r.codigoHTTP << ")\n";
    std::cout << "[Intermediario -> Cliente] " << r.mensajeProtocolo << "\n";

    MensajeV2 resp = parsearMensaje(r.mensajeProtocolo);
    if (resp.tipo == "22" && resp.campos.size() >= 2) {
        std::cout << "  " << resp.campos[0] << " producto(s):\n";
        imprimirListaProductos(resp.campos[1]);
    } else if (resp.tipo == "23" && !resp.campos.empty()) {
        std::cout << "  " << resp.campos[0] << "\n";
    }
}

void pruebaAgregarCarrito(const std::string& args) {
    size_t espacio = args.find_last_of(' ');
    if (espacio == std::string::npos || espacio == 0) {
        std::cout << "[Cliente] Uso: AGREGAR <nombre_producto> <cantidad>\n";
        return;
    }
    std::string nombre = args.substr(0, espacio);
    std::string cantidadStr = args.substr(espacio + 1);
    if (!esNum(cantidadStr)) {
        std::cout << "[Cliente] Cantidad invalida: '" << cantidadStr << "'\n";
        return;
    }

    std::string peticion = construirMensaje(MI_ID, DESTINO_INT, "30", {nombre, cantidadStr});
    std::cout << "\n[Cliente -> Intermediario] " << peticion << "\n";

    auto r = enviarAlIntermediario(peticion);
    if (!r.exito) {
        std::cout << "[Intermediario -> Cliente] ERR_COMM (no se pudo conectar)\n";
        return;
    }
    std::cout << "[Intermediario -> Cliente] " << r.mensajeProtocolo << "\n";

    MensajeV2 resp = parsearMensaje(r.mensajeProtocolo);
    if (resp.tipo == "32" && resp.campos.size() >= 3) {
        int cantidadPedida = std::stoi(cantidadStr);
        int cantidadConfirmada = std::stoi(resp.campos[2]);
        if (cantidadConfirmada <= 0) {
            std::cout << "  No se pudo agregar (sin stock o producto no encontrado).\n";
        } else if (cantidadConfirmada < cantidadPedida) {
            carritoLocal.push_back({nombre, cantidadConfirmada});
            std::cout << "  Agregado al carrito, pero solo " << cantidadConfirmada
                       << " de " << cantidadPedida << " pedidas (stock insuficiente).\n";
        } else {
            carritoLocal.push_back({nombre, cantidadConfirmada});
            std::cout << "  Agregado al carrito.\n";
        }
    }
}

void pruebaFactura() {
    int count = (int)carritoLocal.size();
    std::string peticion;
    if (count == 0) {
        peticion = construirMensaje(MI_ID, DESTINO_INT, "40", {"0"});
    } else {
        std::ostringstream detalle;
        for (size_t i = 0; i < carritoLocal.size(); ++i) {
            detalle << carritoLocal[i].nombre << "," << carritoLocal[i].cantidad;
            if (i + 1 < carritoLocal.size()) detalle << ";";
        }
        peticion = construirMensaje(MI_ID, DESTINO_INT, "40", {std::to_string(count), detalle.str()});
    }
    std::cout << "\n[Cliente -> Intermediario] " << peticion << "\n";

    auto r = enviarAlIntermediario(peticion);
    if (!r.exito) {
        std::cout << "[Intermediario -> Cliente] ERR_COMM (no se pudo conectar)\n";
        return;
    }
    std::cout << "[Intermediario -> Cliente] " << r.mensajeProtocolo << "\n";

    MensajeV2 resp = parsearMensaje(r.mensajeProtocolo);
    if (resp.tipo == "41" && resp.campos.size() >= 3) {
        std::cout << "\n--- Factura TicAmazon ---\n";
        std::cout << "Total: " << resp.campos[0] << "\n";
        auto items = splitV2(resp.campos[2], ';');
        for (auto& item : items) {
            auto campos = splitV2(item, ',');
            if (campos.size() < 3) continue;
            std::cout << "  " << campos[0] << " x" << campos[1] << " = " << campos[2] << "\n";
        }
        carritoLocal.clear(); // el pedido ya quedo cerrado en el intermediario
    } else if (resp.tipo == "42") {
        std::cout << "  " << (resp.campos.empty() ? "" : resp.campos[0]) << "\n";
    }
}

void pruebaErrComm() {
    std::cout << "\n[Cliente -> Intermediario] (forzando error de conexion, puerto de prueba)\n";
    VSocket* client = nullptr;
    try {
        client = new Socket('s');
        int r = client->Connect("127.0.0.1", 9); // puerto discard: normalmente nadie escucha ahi
        if (r != 0) throw std::runtime_error("Connect fallo");
        delete client;
        std::cout << "[Intermediario -> Cliente] (el puerto de prueba si contesto)\n";
    } catch (const std::exception& e) {
        std::cout << "[Intermediario -> Cliente] ERR_COMM (" << e.what() << ")\n";
        delete client;
    }
}

int main() {
    std::cout << "--- TicAmazon - Isla 4 ---" << std::endl;
    std::cout << "Conectando a intermediario en " << INTERMEDIO_HOST << ":" << INTERMEDIO_PORT << std::endl;
    std::cout << "Comandos:\n"
                 "  10                       -> pedir categorias\n"
                 "  20 <categoria>           -> pedir productos de una categoria\n"
                 "  30 <producto> <cantidad> -> agregar al carrito\n"
                 "  40                       -> pedir factura\n"
                 "  error                    -> forzar un fallo de conexion\n"
                 "  salir\n";

    std::string linea;
    while (std::cout << "\n> " && std::getline(std::cin, linea)) {
        std::string comando = aMinusculas(linea);

        if (comando == "salir") break;
        if (comando == "error") { pruebaErrComm(); continue; }

        std::istringstream iss(linea);
        std::string codigo;
        iss >> codigo;
        std::string args;
        std::getline(iss, args);
        if (!args.empty() && args[0] == ' ') args.erase(0, 1);

        if (codigo == "10") { pruebaCategorias(); continue; }
        if (codigo == "20") { pruebaListCat(args); continue; }
        if (codigo == "30") { pruebaAgregarCarrito(args); continue; }
        if (codigo == "40") { pruebaFactura(); continue; }

        std::cout << "  Codigo desconocido: '" << codigo << "'\n";
    }

    std::cout << "\n--- Fin ---" << std::endl;
    return 0;
}