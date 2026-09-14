# Grupo4-CI0123

PROYECTO INTEGRADOR DE SISTEMAS OPERATIVOS Y REDES DE COMUNICACIÓN DE DATOS

**TicAmazon — primera etapa.** Grupo 2, Equipo 4 · Isla 4 · VLAN 240, puertos 13–24.

## Dónde está cada entregable

| Rubro | % | Carpeta |
|---|---|---|
| Elaboración del cliente con jerarquía de clases provista | 50 | `socketClass` |
| Producto propio, su listado y casos de prueba | 5 | `docs/` y `socketClass/`|
| Diseño lógico Packet Tracer del switch | 5 | `packettracer/` |
| Propuesta de modelo de almacenamiento | 10 | `almacenamiento/` |
| Protocolo propio y registro de eventos en bitácoras | 10 | `docs/`|
| Simulación de componentes del sistema | 20 | `simulacion/` |
| Utilización de SSL | 5 | `socketClass/` |

## Estructura

| Carpeta | Contenido |
|---|---|
| `socketClass/` | Cliente HTTP: categorías, limpieza HTML, factura proforma & jerarquía Socket / VSocket / SSL & Script casos prueba|
| `simulacion/` | Hilos y buzón: cliente, intermediario, bodega |
| `almacenamiento/` | File System de la cafetería |
| `packettracer/` | Switch 2960 de la isla 4 y sus capturas |
| `docs/` | Protocolo, casos de prueba |
| `entrega/` | Documento de entrega y presentación |

## socketClass (`socketClass/`)
Cliente HTTP que habla directo con el servidor real de bodega (SSL sobre `os.ecci.ucr.ac.cr`). Usa la jerarquía de clases `VSocket` / `Socket` / `SSLSocket` provista, sobre IPv4 únicamente. Limpia el HTML de la respuesta y lo parsea a productos estructurados (`Bodega-`, categoría, descripción, cantidad, precio), lo que permite listar por categoría, buscar producto por nombre en todo el catálogo, armar un carrito y generar una factura proforma. Todo se maneja con comandos de texto, así que se puede correr manualmente con múltiples entradas o utilizar un archivo de casos de prueba.

**Archivos:**
| Capa | Archivo | Qué hace |
|---|---|---|
| Base (jerarquía de sockets) | `VSocket.h` / `VSocket.cc` | Clase base abstracta: creación del socket, `TryToConnect`, `Close` |
| | `Socket.h` / `Socket.cc` | Socket TCP plano (sin SSL) |
| | `SSLSocket.h` / `SSLSocket.cc` | Socket TCP con SSL/TLS (OpenSSL) |
| Cliente | `Client.cc` | Parseo HTML a productos, `RE_CAT`/`RE_PROD`, carrito, factura proforma
| Pruebas | `pruebas.sh` | Casos de prueba automatizados, deja evidencia en `resultados.txt` |
| Build | `makefile` | Compila `Client` con la jerarquía de sockets y enlaza OpenSSL (`-lssl -lcrypto`) |

### `VSocket.h` / `VSocket.cc`

Clase base abstracta de la jerarquía de sockets. Encapsula la creación, configuración y cierre del socket Unix (IPv4/IPv6, TCP/UDP), dejando a las subclases (Socket, SSLSocket) la responsabilidad de implementar cómo se leen y escriben los bytes.

| Método | Llamada Unix |
|--------|--------------|
| `Init(tipo, ipv6)` | `socket` |
| `TryToConnect(ip, puerto)` y `TryToConnect(host, servicio)` | `connect` / `getaddrinfo` |
| `Close()` (y destructor) | `close` |
                               

### `Socket.h` / `Socket.cc`

Hereda de `VSocket` e implementa la conexión **en claro**. Su gemela es
`SSLSocket`, que hace lo mismo pero cifrado.

`VSocket` ya crea el socket y lo conecta; `Socket` solo completa los métodos
que dependen de cómo se mueven los bytes:

| Método | Llamada Unix |
|---|---|
| `Connect(host, puerto)` y `Connect(host, servicio)` | `connect` |
| `Read(buffer, n)` | `read` — devuelve 0 cuando el servidor cierra |
| `Write(buffer, n)` y `Write(texto)` | `write` |

Como el cliente usa un puntero `VSocket*`, puede sostener un `Socket` o un
`SSLSocket` sin cambiar nada más del código.

### `SSLSocket.h` / `SSLSocket.cc`

También hereda de `VSocket`, pero envuelve la conexión en TLS con OpenSSL.
Reutiliza `TryToConnect` de la base para abrir el socket normal, y le
monta el SSL antes de dejarlo listo para usar.

| Método | Qué hace |
|---|---|
| `Connect(host, puerto)` y `Connect(host, servicio)` | `TryToConnect` + `SSL_connect` — primero conecta el socket, luego negocia TLS |
| `Read(buffer, n)` | `SSL_read` sobre el canal cifrado |
| `Write(buffer, n)` y `Write(texto)` | `SSL_write` sobre el canal cifrado |
| `ShowCerts()` | Imprime el certificado del servidor (subject/issuer) |
| `GetCipher()` | Devuelve el cipher suite negociado en el handshake |

Al igual que `Socket`, expone la misma interfaz de `VSocket*`, así que el
cliente la usa sin saber si por debajo hay un socket en claro o cifrado,
solo cambia qué clase se instancia (`new Socket(...)` vs `new SSLSocket()`).

### Cómo compilar y correr (`socketClass/`)
Requiere OpenSSL (`libssl-dev`) instalado.
```bash
cd socketClass
make
./Client
```
Comandos disponibles una vez corriendo: `<categoria>` (listado), `PRODUCTO <nombre>` (búsqueda), `AGREGAR <nombre> <cantidad>` (carrito), `FACTURA`, `FALLA` (simula error de comunicación), `salir`. Los casos de prueba automatizados corren con `./pruebas.sh` (o `make test`).

## FileSystem (`Almacenamiento/`)

Diseño del sistema de archivos propio del equipo para el negocio de la cafetería (`cafeteria.img`): asignación indexada de 256 bloques de 256 bytes, con superbloque, mapa de bits y directorio de categorías, organizados en 5 categorías de 10 productos fijos cada una. Documenta la estructura de bloques, el registro de producto de 32 bytes y el catálogo completo con precios y stock. Es el formato de disco que leen y escriben las bodegas del proyecto; el intermediario nunca lo abre directamente, solo enruta las peticiones según la tabla de categorías que cada bodega le reporta al arrancar.

| Archivo | Qué hace |
|---|---|
| `Plan_Filesystem_Cafeteria.pdf` | Diseño de `cafeteria.img`: layout de bloques, formato del directorio de categorías, registro de producto y catálogo de los 50 productos |

## Protocolo (`docs/`)
Diseño del protocolo propio del equipo para la comunicación Cliente - Intermediario - Servidor: formato de mensaje, tabla de campos con sus tamaños y regex de validación, y los flujos de cada operación (listado de categoría, detalle de producto, agregar al carrito, factura proforma, y los tres tipos de error: formato, tamaño y comunicación). Es el protocolo que implementa `simulacion/` con hilos y buzón, el cliente HTTP de `socketClass/` no lo envía por la red exactamente, pero reutiliza el mismo vocabulario en los rótulos de log para que ambos se lean igual.

**Archivos:**
| Archivo | Qué hace |
|---|---|
| `Protocolo.pdf` | Formato de mensaje, campos, regex de validación y los tipos de mensaje del protocolo |



## Simulación (`simulacion/`)

Programa con hilos que simula la interacción entre el intermediario y la bodega usando el protocolo propio del equipo, sin sockets ni red. Tres hilos (`Cliente`, `Intermediario`, `Bodega`) se comunican a través de un único `Buzon` (cola de mensajes System V, `msgget`/`msgsnd`/`msgrcv`), usando el campo `mtype` como "canal" para saber quién le habla a quién. El programa despliega en terminal un rótulo por cada mensaje intercambiado (`[Origen -> Destino] mensaje  explicación`), cubriendo los casos del protocolo: listado de categoría, categoría sin formato válido, categoría que la bodega no atiende, detalle de producto, producto no encontrado, y falla de comunicación (`ERR_COMM`).


**Archivos:**

| Capa | Archivo | Qué hace |
|---|---|---|
| Base (IPC y protocolo) | `Buzon.h` / `Buzon.cc` | Cola de mensajes System V (provista por el profesor, completada por el equipo) |
| | `protocolo.h` | IDs de componentes, `canal()`, `CANAL_REGISTRO`, struct `Producto` |
| | `validaciones.h` / `validaciones.cc` | Validación de campos según las regex del protocolo |
| | `Bitacora.h` / `Bitacora.cc` | Impresión de los rótulos de interacción |
| Componentes | `Bodega.h` / `Bodega.cc` | Servidor de productos |
| | `Intermediario.h` / `Intermediario.cc` | Registro de la bodega y manejo de peticiones del cliente |
| | `Cliente.h` / `Cliente.cc` | Guion de pruebas de los casos del protocolo |
| Punto de entrada | `simulacion.cc` | `main()`: arma los objetos y lanza los hilos |
| | `Makefile` | Compila todos los `.cc` de la carpeta |

### Cómo compilar y correr

Requiere Linux 

```bash
cd simulacion
make
./simulacion
```

`make clean` borra el ejecutable y cualquier cola de mensajes que haya quedado huérfana en el kernel si el programa se interrumpió a la mitad (`Ctrl+C`) antes de poder liberarla.




## Fechas

| Fecha | Qué |
|---|---|
| miércoles 26 de agosto | **Code freeze** |
| viernes 28 de agosto | Presentación de la primera etapa |
