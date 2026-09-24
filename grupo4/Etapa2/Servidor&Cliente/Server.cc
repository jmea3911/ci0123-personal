#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <regex>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>

#include "Socket.h"
#include "protocolo.h"
#include "ServidorProductos.h"
#include "SSLSocket.h"

static const char* CERT_FILE = "ci0123.pem";
static const char* KEY_FILE  = "ci0123.pem";

static const std::string MI_ID = "INT_04"; // identidad de isla frente al cliente

// Confirma si un archivo ya existe en disco 
static bool archivoExiste(const std::string& ruta) {
    struct stat buffer;
    return stat(ruta.c_str(), &buffer) == 0;
}

// Genera un certificado SSL autofirmado con openssl si el archivo todavia no existe
static void generarCertificadoSiHaceFalta(const std::string& archivo) {
    if (archivoExiste(archivo)) return;

    std::cout << "No se encontro " << archivo << " -- generando un certificado autofirmado nuevo...\n";
    std::string comando =
        "openssl req -x509 -newkey rsa:2048 -keyout /tmp/tic_key_" + std::to_string(getpid()) + ".pem "
        "-out /tmp/tic_cert_" + std::to_string(getpid()) + ".pem -days 365 -nodes "
        "-subj \"/C=CR/O=UCR/CN=ci0123\" >/dev/null 2>&1 && "
        "cat /tmp/tic_key_" + std::to_string(getpid()) + ".pem /tmp/tic_cert_" + std::to_string(getpid()) + ".pem > " + archivo + " && "
        "rm -f /tmp/tic_key_" + std::to_string(getpid()) + ".pem /tmp/tic_cert_" + std::to_string(getpid()) + ".pem";

    int st = std::system(comando.c_str());
    if (st != 0 || !archivoExiste(archivo)) {
        std::cerr << "No se pudo generar el certificado automaticamente. Corre esto a mano:\n"
                      "  openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -days 365 -nodes "
                      "-subj \"/C=CR/O=UCR/CN=ci0123\"\n"
                      "  cat key.pem cert.pem > " << archivo << "\n";
        std::exit(1);
    }
    std::cout << "Certificado generado en " << archivo << "\n";
}

//regex de validacion
static const std::regex RE_CATEGORIA("^[a-zA-Z]{3,20}$");
static const std::regex RE_PRODUCTO("^[a-zA-Z0-9\\-]{1,50}$");
static const std::regex RE_COUNT("^[0-9]{1,3}$");

//carrito por cliente
struct ItemCarrito { std::string nombre; double precio; int cantidad; };
static std::map<std::string, std::vector<ItemCarrito>> carritos;
static std::mutex mtxCarritos;

// Responde el tipo 10 le pide a ServidorProductos la lista completa de categorias y la reenvia al cliente como 11 
static std::string manejar10ListarCategorias(const MensajeV2& msg, ServidorProductos* bodega) {
    std::string peticion = construirMensaje(MI_ID, "SERV", "10", {});
    MensajeV2 r = parsearMensaje(bodega->procesarMensaje(peticion));
    return construirMensaje(MI_ID, msg.origen, "11", r.campos);
}

// Responde el tipo 20 y valida la categoria con regex, la pide a ServidorProductos, y devuelve 22 o 23 si esa categoria no tiene productos
static std::string manejar20ListarProductos(const MensajeV2& msg, ServidorProductos* bodega) {
    std::string categoria = msg.campos.empty() ? "" : msg.campos[0];

    if (!std::regex_match(categoria, RE_CATEGORIA)) {
        return construirMensaje(MI_ID, msg.origen, "90", {"20", "categoria", categoria});
    }

    std::string peticion = construirMensaje(MI_ID, "SERV", "20", {categoria});
    std::string respuesta = bodega->procesarMensaje(peticion);

    MensajeV2 r = parsearMensaje(respuesta);
    std::string count = r.campos.empty() ? "0" : r.campos[0];
    if (count == "0") {
        return construirMensaje(MI_ID, msg.origen, "23",
            {"No hay productos disponibles en la categoria \"" + categoria + "\""});
    }
    return construirMensaje(MI_ID, msg.origen, "22", r.campos);
}

// Responde el tipo 30 y valida producto y cantidad, le pide a ServidorProductos que reserve stock, y guarda en el carrito de cliente
static std::string manejar30AgregarCarrito(const MensajeV2& msg, ServidorProductos* bodega) {
    if (msg.campos.size() < 2) return construirMensaje(MI_ID, msg.origen, "90", {"30", "count", ""});
    std::string producto = msg.campos[0];
    std::string countStr = msg.campos[1];

    if (!std::regex_match(producto, RE_PRODUCTO))
        return construirMensaje(MI_ID, msg.origen, "90", {"30", "producto", producto});
    if (!std::regex_match(countStr, RE_COUNT))
        return construirMensaje(MI_ID, msg.origen, "90", {"30", "count", countStr});

    std::string reserva = construirMensaje(MI_ID, "SERV", "50", {producto, countStr});
    MensajeV2 r = parsearMensaje(bodega->procesarMensaje(reserva));
    if (r.tipo != "51" || r.campos.size() < 3) {
        return construirMensaje(MI_ID, msg.origen, "90", {"30", "producto", producto});
    }

    double precio = std::stod(r.campos[1]);
    int cantidadReservada = std::stoi(r.campos[2]);

    //32 lleva la cantidad que pudo reservar
    if (cantidadReservada > 0) {
        std::lock_guard<std::mutex> lock(mtxCarritos);
        carritos[msg.origen].push_back({producto, precio, cantidadReservada});
    }

    char precioBuf[16];
    snprintf(precioBuf, sizeof(precioBuf), "%.2f", precio);
    return construirMensaje(MI_ID, msg.origen, "32", {producto, precioBuf, std::to_string(cantidadReservada)});
}

// Responde 40 arma la factura con lo que ya esta confirmado en el carrito de cliente y lo vacia
static std::string manejar40Factura(const MensajeV2& msg) {
    std::string countStr = msg.campos.empty() ? "0" : msg.campos[0];

    std::lock_guard<std::mutex> lock(mtxCarritos);
    auto& carrito = carritos[msg.origen];

    if (countStr == "0" || carrito.empty()) {
        return construirMensaje(MI_ID, msg.origen, "42", {"El carrito esta vacio"});
    }

    //la factura se arma con lo que ya se confirmo en ADD_TO_CART
    std::ostringstream detalle;
    double total = 0;
    for (size_t i = 0; i < carrito.size(); ++i) {
        double subtotal = carrito[i].precio * carrito[i].cantidad;
        total += subtotal;
        char sub[16]; snprintf(sub, sizeof(sub), "%.2f", subtotal);
        detalle << carrito[i].nombre << "," << carrito[i].cantidad << "," << sub;
        if (i + 1 < carrito.size()) detalle << ";";
    }
    char totalBuf[16]; snprintf(totalBuf, sizeof(totalBuf), "%.2f", total);
    std::string respuesta = construirMensaje(MI_ID, msg.origen, "41",
        {totalBuf, std::to_string(carrito.size()), detalle.str()});

    carrito.clear(); // el pedido queda cerrado
    return respuesta;
}

//parsea la linea recibida y la manda al manejador correcto segun el tipo de mensaje, cualquier tipo desconocido devuelve error 90
static std::string despachar(const std::string& lineaProtocolo, ServidorProductos* bodega) {
    MensajeV2 msg = parsearMensaje(lineaProtocolo);
    if (msg.tipo.empty()) return construirMensaje(MI_ID, "CLI_00", "90", {"?", "mensaje", "malformado"});

    if (msg.tipo == "10") return manejar10ListarCategorias(msg, bodega);
    if (msg.tipo == "20") return manejar20ListarProductos(msg, bodega);
    if (msg.tipo == "30") return manejar30AgregarCarrito(msg, bodega);
    if (msg.tipo == "40") return manejar40Factura(msg);

    return construirMensaje(MI_ID, msg.origen, "90", {msg.tipo, "tipo_mensaje", msg.tipo});
}

// Atiende una conexion ya con el handshake TLS hecho: si es GET, sirve el catalogo en HTML si es POST, lee el mensaje del protocolo y lo despacha
static void atenderCliente(VSocket* cliente, ServidorProductos* bodega) {
    auto leer = [cliente](void* buf, size_t n) -> size_t {
        return cliente->Read(buf, n);
    };

    PeticionHTTP req = leerPeticionHTTP(leer);
    if (!req.valida) { cliente->Close(); delete cliente; return; }

    std::string respuesta, http;
    if (req.metodo == "GET") {
        respuesta = bodega->generarPaginaHTML();
        http = construirRespuestaHTTP(200, respuesta, "text/html; charset=utf-8");
        std::cout << "[GET] pagina de catalogo servida" << std::endl;
    } else {
        std::cout << ">> " << req.metodo << " " << req.ruta << " | cuerpo: " << req.cuerpo << std::endl;
        respuesta = despachar(req.cuerpo, bodega);
        std::cout << "<< " << respuesta << std::endl;
        http = construirRespuestaHTTP(200, respuesta);
    }

    cliente->Write(http.c_str(), http.size());
    cliente->Close();
    delete cliente;
}

//arma ServidorProductos, genera el certificado, abre el socket SSL, y entra en un bucle infinito aceptando conexiones y despachando cada una a un hilo separado
int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " <puerto> <archivo_bodega.dat>\n";
        return 1;
    }

    int puertoHTTP = std::atoi(argv[1]);
    std::string archivo = argv[2];

    ServidorProductos bodega(archivo);

    generarCertificadoSiHaceFalta(CERT_FILE);

    VSocket* s1 = new SSLSocket((char*)CERT_FILE, (char*)KEY_FILE, false);
    s1->Bind(puertoHTTP);
    s1->MarkPassive(10);

    std::cout << "== Servidor " << MI_ID << " ==" << std::endl;
    std::cout << "Escuchando HTTP en puerto " << puertoHTTP << std::endl;
    std::cout << "Archivo: " << archivo << std::endl;
    std::cout << "Categorias: ";
    for (auto& c : bodega.categorias()) std::cout << c << " ";
    std::cout << std::endl;

    for (;;) {
        int rawFd = s1->WaitForConnection(); //
        if (rawFd == -1) continue;

        std::thread([rawFd, s1, &bodega]() {
            // el saludo TLS 
            VSocket* cliente = nullptr;
            try {
                cliente = s1->CompletarConexion(rawFd);
                if (cliente == nullptr) {
                    std::cerr << "[Servidor] SSL fallido con un cliente\n";
                    return;
                }
                atenderCliente(cliente, &bodega);
            } catch (const std::exception& e) {
                // si un cliente cierra la conexion de golpe Read()/Write() lanzan una excepcion 
                std::cerr << "[Servidor] conexion con un cliente fallo (" << e.what()
                           << "), se descarta esa conexion, el servidor sigue\n";
                delete cliente; // por si atenderCliente no alcanzo a limpiarlo
            }
        }).detach();
    }
}