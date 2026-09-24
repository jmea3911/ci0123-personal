#!/bin/bash
# Uso: ./test_flujo.sh
#
# compila todo desde cero, formatea bodega.dat, levanta el servidor,
# y corre una serie de casos contra el cliente real (por SSL) y por
# curl, verificando la salida esperada de cada uno.

set -u
cd "$(dirname "$0")"

PASARON=0
FALLARON=0
PUERTO=8080
HOST=127.0.0.1

verde() { printf "\033[32m%s\033[0m\n" "$1"; }
rojo()  { printf "\033[31m%s\033[0m\n" "$1"; }

verificar() {
    # verifica "nombre del caso" "texto que debe aparecer" "$salida"
    local nombre="$1" esperado="$2" salida="$3"
    if echo "$salida" | grep -qF -- "$esperado"; then
        verde "  OK   $nombre"
        PASARON=$((PASARON+1))
    else
        rojo  "  FAIL $nombre  (no se encontro: \"$esperado\")"
        FALLARON=$((FALLARON+1))
    fi
}

echo "--- 1. Compilando todo  ---"
make clean > /dev/null
if ! make > build.log 2>&1; then
    rojo "FALLO LA COMPILACION -- revisa build.log"
    cat build.log
    exit 1
fi
verde "compilo sin errores"
if grep -qi "warning" build.log; then
    echo "  (hay warnings, revisa build.log si quieres verlos)"
fi

echo ""
echo "--- 2. Formateando bodega.dat ---"
rm -f bodega.dat
./make_fileSys > /dev/null
[ -f bodega.dat ] && verde "bodega.dat creado" || { rojo "no se creo bodega.dat"; exit 1; }

echo ""
echo "--- 3. Levantando el servidor ---"
rm -f servidor.log
./servidor $PUERTO bodega.dat > servidor.log 2>&1 &
SRV_PID=$!
sleep 1
if ! kill -0 $SRV_PID 2>/dev/null; then
    rojo "el servidor no arranco -- revisa servidor.log"
    cat servidor.log
    exit 1
fi
verde "servidor corriendo (pid $SRV_PID)"

#matar el servidor al salir
trap 'kill -9 $SRV_PID 2>/dev/null' EXIT

echo ""
echo "--- 4. Casos contra ./cliente ---"

SALIDA=$(printf "10\nsalir\n" | ./cliente $HOST 2>&1)
verificar "10 -- lista todas las categorias" "postres,caramelos,bebidas,reposteria" "$SALIDA"

SALIDA=$(printf "20 postres\nsalir\n" | ./cliente $HOST 2>&1)
verificar "20 postres -- muestra Flan-caramelo" "Flan-caramelo" "$SALIDA"

SALIDA=$(printf "20 POSTRES\nsalir\n" | ./cliente $HOST 2>&1)
verificar "20 POSTRES (mayusculas) -- sigue encontrando productos" "3 producto(s)" "$SALIDA"

SALIDA=$(printf "20 zzz\nsalir\n" | ./cliente $HOST 2>&1)
verificar "20 zzz -- categoria valida pero sin productos" "No hay productos disponibles" "$SALIDA"

SALIDA=$(printf "30 flan-carameLO 2\nsalir\n" | ./cliente $HOST 2>&1)
verificar "30 con mayusculas distintas -- igual lo agrega" "Agregado al carrito" "$SALIDA"

SALIDA=$(printf "30 Flan-caramelo 999\nsalir\n" | ./cliente $HOST 2>&1)
verificar "30 con cantidad mayor al stock -- no se agrega" "No se pudo agregar" "$SALIDA"

SALIDA=$(printf "30 Tres-leches 1\n40\nsalir\n" | ./cliente $HOST 2>&1)
verificar "30 luego 40 -- la factura sale con un total" "Total:" "$SALIDA"

SALIDA=$(printf "40\nsalir\n" | ./cliente $HOST 2>&1)
verificar "40 con el carrito vacio -- avisa que esta vacio" "carrito esta vacio" "$SALIDA"

SALIDA=$(printf "error\nsalir\n" | ./cliente $HOST 2>&1)
verificar "error -- ERR_COMM al forzar un fallo de conexion" "ERR_COMM" "$SALIDA"

echo ""
echo "--- 5. Casos contra curl (acceso externo, sin el cliente propio) ---"

SALIDA=$(curl -sk --max-time 5 "https://$HOST:$PUERTO/")
verificar "GET / por curl -- devuelve el catalogo en HTML" "<h1>Servidor de productos" "$SALIDA"

CODIGO=$(curl -sk --max-time 5 -o /dev/null -w "%{http_code}" "https://$HOST:$PUERTO/")
verificar "GET / por curl -- responde HTTP 200" "200" "$CODIGO"

echo ""
echo "--- 6. El servidor sigue vivo despues de todo esto? ---"
if kill -0 $SRV_PID 2>/dev/null; then
    verde "  OK   el servidor sigue corriendo (no se cayo con ningun caso)"
    PASARON=$((PASARON+1))
else
    rojo  "  FAIL el servidor se murio en algun punto -- revisa servidor.log"
    FALLARON=$((FALLARON+1))
fi

echo ""
echo "------------------------------------"
echo "  Pasaron: $PASARON   Fallaron: $FALLARON"
echo "------------------------------------"

[ $FALLARON -eq 0 ] && exit 0 || exit 1
