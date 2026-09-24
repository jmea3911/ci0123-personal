# TicAmazon — Segunda etapa


---

| Rubro | % | Dónde está |
|---|---|---|
| Servidor de productos con la jerarquía de clases y su propio contenedor | **50** | `Etapa2/Servidor&Cliente/` — `Server.cc`, `ServidorProductos.*`, `SSLSocket.*` / `Socket.*` / `VSocket.*`, `FileSystem.*`, `Bodega.*`, `Bitmap.*`, `Formatos.h` |
| Programa cliente para interactuar con este servidor | **5** | `Etapa2/Servidor&Cliente/Client.cc` |
| Implementación del modelo de almacenamiento sin restricciones de almacenamiento | **15** | `Etapa2/Servidor&Cliente/FileSystem.*`, `Bodega.*`, `Bitmap.*`, `Formatos.h`, más las herramientas `make_fileSys.cc`, `read_fileSys.cc`, `fileSys_herr.cc` |
| Protocolo grupal para interacción entre servidores y registro de eventos en bitácoras | **10** | `Etapa2/Protocolos/` — `ProtocoloInternoIsla4.pdf`, `protocoloMancomunado.pdf` |
| Adecuación de la simulación al nuevo protocolo | **10** | `Etapa2/simulacion/` — `Buzon.*`, `Cliente.*`, `Intermediario.*`, `Bodega.*`, `simulacion.cc` |
| Diseño lógico en Packet Tracer del laboratorio 3-5 | **10** | `packettracer/` — capturas `ping`, `ipconfig`, `show interfaces trunk`, `show ip interface brief`, `show vlan brief` |

---

## Estructura general

| Carpeta | Contenido |
|---|---|
| `Etapa2/Servidor&Cliente/` | Cliente HTTP, servidor Intermediario SSL, jerarquía de sockets, sistema de archivos y herramientas de formateo/lectura de `bodega.dat` |
| `Etapa2/simulacion/` | Hilos y buzón System V: Cliente, Intermediario y Bodega hablando el protocolo en terminal |
| `packettracer/` | Capturas del switch 2960 de la isla 4 (trunk, VLAN 240, ping entre equipos) |
| `docs/` | Protocolo en PDF y casos de prueba |
| `entrega/` | Documento de entrega y presentación |

---

## 1. Simulación (`Etapa2/simulacion/`)

Programa con **tres hilos** (`Cliente`, `Intermediario`, `Bodega`) que se comunican a través de un único `Buzon` (cola de mensajes System V: `msgget`/`msgsnd`/`msgrcv`). El campo `mtype` actúa como **canal**, de modo que cada hilo recibe únicamente los mensajes dirigidos a él, sin sockets ni red.

Cubre los casos del protocolo: listado de categoría, categoría con formato inválido (`ERR_FORMAT`), categoría que la bodega no atiende (`PRODUCT_LIST_EMPTY`), detalle de producto, agregar al carrito con reserva de stock, producto no encontrado, carrito vacío y falla de comunicación (`ERR_COMM`, simulada con `RecibirConEspera` + `IPC_NOWAIT`).

### Archivos

| Capa | Archivo | Qué hace |
|---|---|---|
| Base (IPC) | `Buzon.h` / `Buzon.cc` | Cola de mensajes System V. Incluye `RecibirConEspera()` (timeout no bloqueante) para `ERR_COMM` |
| Protocolo | `Protocolo.h` | IDs internos, `canal()`, `CANAL_REGISTRO`, enum `TipoMensaje`, `armarMensaje()`/`parsearMensaje()`, struct `Producto` |
| Validación | `Validaciones.h` / `Validaciones.cc` | Regex del protocolo: origen/destino, tipo, categoría, producto, precio, stock, count |
| Bitácora | `Bitacora.h` / `Bitacora.cc` | Impresión con mutex de los rótulos `[Origen -> Destino] mensaje  explicación` |
| Componentes | `Bodega.*` | Servidor de productos: responde `10/20/50` con `11/22/51`, incluye el caso "silenciar" que fuerza `ERR_COMM` |
| | `Intermediario.*` | Registra la bodega, valida campos, enruta peticiones y mantiene el carrito por cliente |
| | `Cliente.*` | Guion de pruebas del protocolo (todas las operaciones y errores) |
| Entrada | `simulacion.cc` | `main()`: crea bodega, intermediario y cliente; lanza los tres hilos |

### Cómo compilar y correr

```bash
cd Etapa2/simulacion
make
./simulacion
```

---

## 2. Servidor & Cliente (`Etapa2/Servidor&Cliente/`)

El **cliente HTTP** que habla con el intermediario y el **intermediario SSL** que atiende a los clientes, consulta la bodega y genera la factura proforma. Todo el catálogo está guardado en un **sistema de archivos propio** (`bodega.dat`).

### 2.1 Jerarquía de sockets

Igual que en la primera etapa, pero ahora la usa el **servidor** (SSL) y el cliente puede elegir entre TCP plano o SSL. La idea es que el código de alto nivel trabaje siempre con un `VSocket*` y no sepa si por debajo hay cifrado o no.

| Capa | Archivo | Qué hace |
|---|---|---|
| Base abstracta | `VSocket.h` / `VSocket.cc` | `Init`, `TryToConnect`, `Bind`, `MarkPassive`, `WaitForConnection`, `Shutdown`, `sendTo`/`recvFrom`, `Close` |
| TCP plano | `Socket.h` / `Socket.cc` | `connect` + `read`/`write`; `AcceptConnection` y `CompletarConexion` triviales |
| SSL/TLS | `SSLSocket.h` / `SSLSocket.cc` | Envuelve la conexión en OpenSSL: `SSL_connect`/`SSL_accept`, `SSL_read`/`SSL_write`, `ShowCerts`, `GetCipher`, `UseCertificate` (certificado de cliente) y `CompletarConexion` con timeout de 10 s para no colgarse si alguien abre TCP y no hace el handshake |

### 2.2 Protocolo propio (`protocolo.h` / `protocolo.cc`)

Formato de mensaje: `ORIGEN|DESTINO/TIPO/campo1/campo2/...`

| Tipo | Nombre | Sentido |
|---|---|---|
| 10 | `REQUEST_CATEGORIES` | Cliente - Intermediario |
| 11 | `CATEGORY_LIST` | Intermediario - Cliente |
| 20 | `REQUEST_PRODUCTS` | Cliente - Intermediario |
| 22 | `PRODUCT_LIST` | Intermediario - Cliente |
| 23 | `PRODUCT_LIST_EMPTY` | Intermediario - Cliente |
| 30 | `ADD_TO_CART` | Cliente - Intermediario |
| 32 | `CART_UPDATED` | Intermediario - Cliente |
| 40 | `REQUEST_FACTURA` | Cliente - Intermediario |
| 41 | `FACTURA` | Intermediario - Cliente |
| 42 | `CART_EMPTY` | Intermediario - Cliente |
| 50 | `RESERVE_STOCK` | Intermediario - Bodega |
| 51 | `RESERVED` | Bodega - Intermediario |
| 90 | `ERR_FORMAT` | Cualquiera |
| 91 | `ERR_SIZE` | Cualquiera |
| 92 | `ERR_COMM` | Cualquiera |

Incluye también las funciones HTTP: `construirPeticionHTTP`, `construirRespuestaHTTP`, `extraerCuerpoRespuesta`, `extraerCodigoRespuesta` y un `leerPeticionHTTP` que arma la petición byte a byte leyendo del socket.

## 2.3 Compilación y ejecución

Desde la carpeta `Etapa2/Servidor&Cliente/`:

```bash
make
make fileSys
```

Para ejecutar el sistema se utilizan dos terminales.

**Terminal 1 — Servidor:**

```bash
./servidor 8080 bodega.dat
```

**Terminal 2 — Cliente:**

```bash
./cliente
```

El servidor debe iniciarse primero y mantenerse ejecutándose mientras se utiliza el cliente.

### 2.4 Sistema de archivos propio (`FileSystem.*`, `Bitmap.*`, `Bodega.*`, `Formatos.h`)

Todo el catálogo vive dentro de **un único archivo** `bodega.dat`

- **Bloques**: 1024 bloques de 256 bytes.
- **Superbloque** (bloque 0): firma `"CAFE02"`, nombre del negocio, número de bodegas, bloque del directorio de bodegas.
- **Bitmap** (bloque 1): 1024 bits, un bit por bloque.
- **Directorios con crecimiento por punteros**: bodegas y categorías se guardan en bloques de 7 casillas + encabezado; cuando se llenan, se reserva un bloque nuevo y se enlaza por `siguienteBloque`.
- **Bloque índice** por categoría: lista de bloques de datos + contadores.
- **Registro de producto**: 64 bytes (nombre 56, cantidad 2, precio en centavos 4, estado 1) - **4 productos por bloque**.

| Archivo | Qué hace |
|---|---|
| `Formatos.h` | Constantes y structs empaquetados (`Superbloque`, `BloqueDirBodegas`, `BloqueDirCategorias`, `BloqueIndice`, `RegistroProducto`) |
| `Bitmap.*` | `pedirBloque`, `liberarBloque`, `bloquesLibres` |
| `FileSystem.*` | Abre/crea `bodega.dat`, `leerBloque`/`escribirBloque`, `registrarBodega`, `buscarBodega`, `listarBodegas` |
| `Bodega.*` | `crearCategoria`, `listarCategorias`, `agregarProducto`, `listarProductos`, `ajustarCantidad` (usado en la reserva de stock) |
| `make_fileSys.cc` | Formatea `bodega.dat` con dos bodegas (`Bodega1`: postres/caramelos, `Bodega2`: bebidas/repostería) y su catálogo inicial |
| `read_fileSys.cc` | Reporte por consola de bodegas, categorías, productos, cantidades y precios |
| `fileSys_herr.cc` | Herramienta CLI: `bodegas`, `crear-bodega`, `categorias`, `agregar-categoria`, `listar`, `insertar` |

Compilación (dentro de `Etapa2/Servidor&Cliente/`):

```bash
make
make fileSys          # compila make_fileSys y formatea bodega.dat con las dos bodegas
./read_fileSys
./fileSys_herr bodega.dat bodegas
```

### 2.5 Servidor Intermediario (`Server.cc` + `ServidorProductos.*`)

El servidor de la isla 4 es quien atiende a los clientes HTTP y traduce al protocolo propio.

- **`ServidorProductos`** encapsula el `Filesystem`, carga todas las bodegas al arrancar y construye un índice `categoria - bodega`. Implementa la lógica de los tipos 10, 20 y 50; también genera una **página HTML** con el catálogo completo cuando recibe `GET`.
- **`Server.cc`** es el `main` del intermediario:
  - Escucha con **SSL/TLS** (`SSLSocket`), usando `ci0123.pem` como certificado y llave. Si el archivo no existe, **lo genera automáticamente** con `openssl req -x509` (autofirmado).
  - Un **hilo por conexión**, con `WaitForConnection` + `CompletarConexion` en el hilo hijo para que aceptar nunca se cuelgue si alguien abre TCP sin hacer el handshake.
  - Valida campos con regex (`RE_CATEGORIA`, `RE_PRODUCTO`, `RE_COUNT`) y devuelve `ERR_FORMAT` (90) cuando algo no calza.
  - Mantiene un **carrito por cliente** (clave = `origen` del mensaje), calcula la factura con totales y subtotales, y la devuelve como mensaje 41.

Compilación y arranque:

```bash
cd "Etapa2/Servidor&Cliente"
make
./servidor 8080 bodega.dat
```

### 2.6 Cliente HTTP (`Client.cc`)

Cliente que habla directo con el intermediario por SSL.

- Menú de comandos:
  - `10` - pedir categorías
  - `20 <categoria>` - listar productos de una categoría
  - `30 <producto> <cantidad>` - agregar al carrito
  - `40` - pedir factura
  - `error` - forzar un fallo de conexión (para ver `ERR_COMM`)
  - `salir`
- Envuelve cada mensaje del protocolo en una **petición HTTP POST** y lee la respuesta hasta que el servidor cierra la conexión.
- Lleva un carrito local (`carritoLocal`) para poder armar la petición de factura.

Compilación:

```bash
cd "Etapa2/Servidor&Cliente"
make
./cliente
```
## Pruebas

`test_flujo.sh` compila todo desde cero, formatea `bodega.dat`, levanta
el servidor, y corre 12 casos contra el cliente real y contra `curl`
(categorías, productos, insensibilidad a mayúsculas, stock insuficiente,
carrito, factura, `ERR_COMM`, y acceso externo sin pasar por `Client.cc`).
Al final reporta cuántos pasaron y cuántos fallaron.

```bash
chmod +x test_flujo.sh
./test_flujo.sh
```

---

## 3. Red (Packet Tracer)

Capturas del switch 2960 de la isla 4 incluidas en este repositorio:

- `ping 172.16.123.20` entre un equipo y otro dentro de la misma VLAN: 0 % de pérdida, TTL 127.
- `ipconfig` del equipo: IP `172.16.123.68/28`, gateway `172.16.123.65`.
- `show interfaces trunk`: `Fa0/24` en modo trunk 802.1q, VLANs permitidas `1–1005`, activas `1,240`.
- `show ip interface brief`: `Vlan240 = 172.16.123.66`, interfaces físicas en down salvo `Fa0/1`, `Fa0/2` y `Fa0/24`.
- `show vlan brief`: VLAN 1 `default` activa en `Fa0/3…Fa0/23` y `Gig0/1-2`; **VLAN 240 `Equipo4`** activa en `Fa0/1` y `Fa0/2`.

---

## Fechas

| Fecha | Qué |
|---|---|
| miércoles 23 de septiembre | **Code freeze** (segunda etapa) |
| viernes 25 de septiembre | Presentación de la segunda etapa |
---

