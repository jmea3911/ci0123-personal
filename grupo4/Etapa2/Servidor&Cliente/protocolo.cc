#include "protocolo.h"

std::vector<std::string> splitV2(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string tok;
    while (std::getline(ss, tok, delim)) out.push_back(tok);
    return out;
}

//parsea "ORIGEN|DESTINO/TIPO/campo1/campo2/..."
MensajeV2 parsearMensaje(const std::string& linea) {
    MensajeV2 m;
    auto pos = linea.find('|');
    if (pos == std::string::npos) return m; // mensaje mal formado
    m.origen = linea.substr(0, pos);

    auto partes = splitV2(linea.substr(pos + 1), '/');
    if (partes.empty()) return m;
    m.destino = partes[0];
    if (partes.size() > 1) m.tipo = partes[1];
    for (size_t i = 2; i < partes.size(); ++i) m.campos.push_back(partes[i]);
    return m;
}

std::string construirMensaje(const std::string& origen, const std::string& destino,
                              const std::string& tipo,
                              const std::vector<std::string>& campos) {
    std::string s = origen + "|" + destino + "/" + tipo;
    for (auto& c : campos) s += "/" + c;
    return s;
}

void logProtocolo(const std::string& linea) {
    std::cout << linea << std::endl;
}

//arma la respuesta HTTP que el Intermediario le manda al cliente
std::string construirRespuestaHTTP(int codigo, const std::string& cuerpo,
                                    const std::string& tipoContenido) {
    std::string estado = (codigo == 200) ? "200 OK"
                        : (codigo == 400) ? "400 Bad Request"
                        : (codigo == 404) ? "404 Not Found"
                        : "500 Internal Server Error";
    std::ostringstream resp;
    resp << "HTTP/1.1 " << estado << "\r\n"
         << "Content-Type: " << tipoContenido << "\r\n"
         << "Content-Length: " << cuerpo.size() << "\r\n"
         << "Connection: close\r\n"
         << "\r\n"
         << cuerpo;
    return resp.str();
}

//arma la peticion HTTP que el cliente le manda al Intermediario
std::string construirPeticionHTTP(const std::string& ruta, const std::string& host,
                                   const std::string& cuerpo) {
    std::ostringstream req;
    req << "POST " << ruta << " HTTP/1.1\r\n"
        << "Host: " << host << "\r\n"
        << "Content-Type: text/plain\r\n"
        << "Content-Length: " << cuerpo.size() << "\r\n"
        << "Connection: close\r\n"
        << "\r\n"
        << cuerpo;
    return req.str();
}

//extrae el cuerpo de una respuesta HTTP completa ya leida (para el cliente)
std::string extraerCuerpoRespuesta(const std::string& respuestaCompleta) {
    auto pos = respuestaCompleta.find("\r\n\r\n");
    if (pos == std::string::npos) return "";
    return respuestaCompleta.substr(pos + 4);
}

//extrae el codigo (200, 400, etc.) de la primera linea de la respuesta
int extraerCodigoRespuesta(const std::string& respuestaCompleta) {
    if (respuestaCompleta.rfind("HTTP/", 0) != 0) return -1;
    auto e1 = respuestaCompleta.find(' ');
    if (e1 == std::string::npos) return -1;
    auto e2 = respuestaCompleta.find(' ', e1 + 1);
    if (e2 == std::string::npos) return -1;
    try { return std::stoi(respuestaCompleta.substr(e1 + 1, e2 - e1 - 1)); }
    catch (...) { return -1; }
}