#ifndef BODEGA_H
#define BODEGA_H

#include <memory>
#include <string>

#include "Buzon.h"
#include "Protocolo.h"
#include "FileSystem.h"
#include "Inventario.h"

// Representa un "servidor de productos". No sabe nada de clientes, solo recibe mensajes del intermediario
// y responde con los productos que lee del almacenamiento: el disco (un solo archivo, por ejemplo
// fs1.dat.ci0123) que crean las herramientas de la carpeta almacenamiento/. Categorias, listados
// y reservas de stock se consultan y se escriben en ese disco, no hay datos propios en memoria.
// Puede haber varios (SERV_01, SERV_02...), cada uno con su id interno y su propio disco, y se pueden
// incorporar en cualquier momento: al arrancar se registra ante el intermediario.
class Bodega {
public:
    // ip y puerto son los que se publican en el registro (en la simulación no se usan para
    // conectar, porque la comunicación va por el Buzon). rutaDisco es el archivo del
    // almacenamiento; si no existe o no es un disco valido el constructor lanza std::runtime_error.
    Bodega( Buzon & buzon, int miId, const std::string & nombre,
            const std::string & ip, int puerto, const std::string & rutaDisco );

    // Ciclo principal del hilo: se registra una vez y luego atiende mensajes hasta recibir EXIT.
    void ejecutar();

private:
    Buzon & buzon_;
    int miId_;
    std::string nombre_;
    std::string ip_;
    int puerto_;
    std::string rutaDisco_;
    std::unique_ptr<FileSystem> fs_;
    std::unique_ptr<Inventario> inventario_;
    std::string idDisco_;   // "I04-S1", para los rotulos

    bool registrarse();
    void manejarRequestCategories();
    void manejarRequestProducts( const std::string & categoria );
    void manejarReserveStock( const std::string & producto, const std::string & cantidadTexto );
};

#endif
