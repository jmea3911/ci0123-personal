#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cctype>

//protocolo
//formato del mensaje: ORIGEN|DESTINO/TIPO_MENSAJE/campo1/campo2/...

struct MensajeV2 {
    std::string origen;
    std::string destino;
    std::string tipo;
    std::vector<std::string> campos;
};

//split generico, lo usa el parseo de mensajes y tambien quien liste productos/detalle de factura
std::vector<std::string> splitV2(const std::string& s, char delim);

//parsea "ORIGEN|DESTINO/TIPO/campo1/campo2/..."
MensajeV2 parsearMensaje(const std::string& linea);

//arma la linea inversa a parsearMensaje: recibe los campos sueltos y los deja en formato protocolo
std::string construirMensaje(const std::string& origen, const std::string& destino,
                              const std::string& tipo,
                              const std::vector<std::string>& campos = {});

//mensaje para terminal
void logProtocolo(const std::string& linea);

//HTTP
//lo necesario para mandar un mensaje del protocolo en el cuerpo de una peticion/respuesta HTTP 

struct PeticionHTTP {
    std::string metodo;
    std::string ruta;
    std::string cuerpo;
    bool valida = false;
};

//lee del socket byte a byte (a traves de Read) hasta armar una peticion HTTP completa
template <typename LeerFn>
inline PeticionHTTP leerPeticionHTTP(LeerFn leer) {
    PeticionHTTP req;
    std::string buffer;
    char chunk[1024];
    size_t finHeaders;

    //primero se juntan bytes hasta encontrar la linea en blanco que separa headers del cuerpo
    while ((finHeaders = buffer.find("\r\n\r\n")) == std::string::npos) {
        size_t n = leer(chunk, sizeof(chunk));
        if (n == 0) return req; // conexion cerrada antes de completar headers
        buffer.append(chunk, n);
        if (buffer.size() > 8192) return req; // headers absurdamente grandes
    }

    std::string headers = buffer.substr(0, finHeaders);
    std::string cuerpoParcial = buffer.substr(finHeaders + 4);

    //primera linea: "METODO RUTA HTTP/1.1"
    std::istringstream hs(headers);
    std::string primeraLinea;
    std::getline(hs, primeraLinea);
    if (!primeraLinea.empty() && primeraLinea.back() == '\r') primeraLinea.pop_back();

    std::istringstream pl(primeraLinea);
    pl >> req.metodo >> req.ruta;
    if (req.metodo.empty() || req.ruta.empty()) return req;

    //resto de headers
    size_t contentLength = 0;
    std::string linea;
    while (std::getline(hs, linea)) {
        if (!linea.empty() && linea.back() == '\r') linea.pop_back();
        if (linea.empty()) continue;
        auto dosPuntos = linea.find(':');
        if (dosPuntos == std::string::npos) continue;
        std::string clave = linea.substr(0, dosPuntos);
        std::transform(clave.begin(), clave.end(), clave.begin(),
                        [](unsigned char c) { return std::tolower(c); });
        if (clave == "content-length") {
            std::string valor = linea.substr(dosPuntos + 1);
            size_t inicio = valor.find_first_not_of(" \t");
            if (inicio != std::string::npos) contentLength = std::stoul(valor.substr(inicio));
        }
    }

    //puede que ya hayan llegado bytes del cuerpo junto con los headers; se completa lo que falte
    req.cuerpo = cuerpoParcial;
    while (req.cuerpo.size() < contentLength) {
        size_t n = leer(chunk, sizeof(chunk));
        if (n == 0) break;
        req.cuerpo.append(chunk, n);
    }
    if (req.cuerpo.size() > contentLength) req.cuerpo.resize(contentLength);

    req.valida = true;
    return req;
}

//arma la respuesta HTTP que el Intermediario le manda al cliente
std::string construirRespuestaHTTP(int codigo, const std::string& cuerpo,
                                    const std::string& tipoContenido = "text/plain; charset=utf-8");

//arma la peticion HTTP que el cliente le manda al Intermediario
std::string construirPeticionHTTP(const std::string& ruta, const std::string& host,
                                   const std::string& cuerpo);

//extrae el cuerpo de una respuesta HTTP completa ya leida (para el cliente)
std::string extraerCuerpoRespuesta(const std::string& respuestaCompleta);

//extrae el codigo 200, 400, de la primera linea de la respuesta
int extraerCodigoRespuesta(const std::string& respuestaCompleta);