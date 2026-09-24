# Casos de prueba 

## 1. Flujo completo (categorías - productos - carrito - factura)

**Pasos:**
```
./servidor 8080 bodega.dat   (en una terminal)
./cliente                    (en otra)
> 10
> 20 postres
> 30 Flan-caramelo 2
> 40
```

**Esperado:** el cliente recibe la lista de categorías, los productos de
`postres` con precio y stock, confirma que agregó 2 `Flan-caramelo` al
carrito, y la factura final muestra el total correcto (`3.20 × 2 = 6.40`).

**Resultado:** - la factura salió `6.40`, coincide con el
cálculo

---

## 2. Categoría sin productos

**Pasos:** pedir una categoría que existe en el directorio pero no tiene
productos cargados (`20/reposteria` antes de insertarle nada).

**Esperado:** el servidor responde `23` (`EMPTY_CATEGORY`) con un mensaje
legible, no `22` con una lista vacía 

**Resultado:**   - el cliente mostró
`"No hay productos disponibles en la categoria..."`.

---

## 3. Nombre de producto largo

El diseño original de `RegistroProducto` solo tenía
24 bytes para el nombre (23 caracteres usables), pero el protocolo permite
hasta 50. Se amplió a 64 bytes por registro.

**Pasos:** insertar un producto con un nombre de exactamente 50 caracteres
(`Torta-de-chocolate-especial-para-cumpleanos-grand`), listarlo, y
confirmar que no se caerse.

**Resultado:**   - el nombre completo de 50 caracteres se
guardó y se leyó sin caerse.

---

## 4. Separación entre bodegas

**Pasos:** crear `Bodega1` con categoría `postres` y `Bodega2` con
categoría `bebidas`, en el mismo archivo `.dat`. Pedir `listarProductos`
de `bebidas` usando el bloque raíz de `Bodega1`.

**Esperado:** debe devolver una lista vacía - `bebidas` no existe dentro
de `Bodega1`, aunque ambas estén en el mismo archivo físico.

**Resultado:**  .

---

## 5. Tres bodegas (agregando una)

**Pasos:**
```
./make_fileSys                                        # Bodega1, Bodega2 de fabrica
./fileSys_herr bodega.dat crear-bodega Bodega3
./fileSys_herr bodega.dat Bodega3 agregar-categoria snacks
./fileSys_herr bodega.dat Bodega3 insertar snacks Papitas 50 150
./servidor 8080 bodega.dat                             # se reinicia para verla
./cliente
> 10
> 20 snacks
> 30 Papitas 3
> 40
```

**Esperado:** el servidor debe listar `snacks` entre las categorías al
arrancar, y el cliente debe poder comprar `Papitas` normalmente.

**Resultado:**   - categorías mostradas:
`postres,caramelos,bebidas,reposteria,snacks`; factura final `4.50`.

---

## 6. Archivo inválido/inexistente en `fileSys_herr`

Al pasarle a `fileSys_herr` un nombre de archivo que no
existe, el programa se quedaba colgado indefinidamente en vez de fallar
con un error - causado por leer datos sin inicializar de un `fstream` que
nunca se abrió.

**Pasos:** `./fileSys_herr archivo_que_no_existe.dat Bodega3 agregar-categoria snacks`

**Esperado :** el programa debe terminar de
inmediato con un mensaje de error, código de salida distinto de 0.

**Resultado:**   - imprime
`"... no existe o no es un archivo valido"` y sale con código 1.

---

## 7. SSL - autogeneración de certificado

**Pasos:** borrar cualquier `ci0123.pem` de la carpeta, y correr
`./servidor 8080 bodega.dat` directo.

**Esperado:** el servidor debe detectar que no existe el certificado,
generarlo automáticamente con `openssl`, y arrancar normalmente sin
intervención manual.

**Resultado:**   - el log mostró
`"No se encontro ci0123.pem -- generando un certificado autofirmado
nuevo..."` seguido de un arranque normal, y el cliente pudo conectarse
por TLS sin ningún paso manual previo.

---

## 8. Acceso desde un cliente HTTP externo (no el `./cliente` propio)

**Pasos:** `curl -sk https://<ip>:8080/` (el catálogo en HTML) y
`curl -sk -X POST --data "INT_04|SERV/10" https://<ip>:8080/mensaje`
(el protocolo en texto plano).

**Esperado:** el navegador/curl debe poder ver el catálogo sin necesitar
ningún certificado propio, y recibir
respuesta HTML válida.

**Resultado:**   - `curl -k` mostró el catálogo completo con
las categorías de todas las bodegas. También  desde un
navegador de celular y de otra computadora del laboratorio.

---

## 9. Error de validación - categoría con formato inválido

**Pasos:** pedir una categoría que no cumple el regex
(`^[a-zA-Z]{3,20}$`), por ejemplo con un número o un símbolo.

**Esperado:** el servidor debe responder `90` (error de formato) con el
campo inválido señalado, no intentar procesar la categoría de todas
formas.

**Resultado:**   en el escenario de ejemplo del documento de
protocolo (`"torta#3"` como categoría - `90/20/categoria/"torta#3"`).

---

## 10. Prueba reinicio

**Pasos:** agregar al carrito (lo que resta stock del archivo), cerrar el
servidor, volver a arrancarlo, y verificar con `read_fileSys` que el
stock decrementado se mantuvo.

**Esperado:** el cambio de stock debe sobrevivir al reinicio, porque vive
en el archivo `.dat`, no en memoria.

**Resultado:**   - el stock de `Flan-caramelo` bajó de 20 a
18 después de una compra, y siguió en 18 en la corrida siguiente del
servidor.

---

## 11. Reserva con stock insuficiente

**Pasos:** pedir agregar al carrito una cantidad mayor a la disponible
(por ejemplo, 999 unidades de un producto con 20 en stock).

**Esperado:** el servidor no debe reservar nada (`51` con cantidad `0`),
y el cliente debe indicar claramente que no se pudo agregar - sin
reventar ni dejar el stock en un estado negativo.

**Resultado:**   - el cliente mostró
`"No se pudo agregar (sin stock o producto no encontrado)"`, y el stock
del producto no cambió.