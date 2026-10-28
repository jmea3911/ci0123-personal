ALMACEN = ../almacenamiento
FUENTES = Buzon.cc Validaciones.cc Bitacora.cc Bodega.cc Intermediario.cc Cliente.cc simulacion.cc \
          $(ALMACEN)/FileSystem.cc $(ALMACEN)/Inventario.cc

simulacion: $(FUENTES) $(wildcard *.h) $(wildcard $(ALMACEN)/*.h)
	g++ -std=c++17 -pthread -Wall -I$(ALMACEN) $(FUENTES) -o simulacion

# crea (o recrea, con el stock original) los discos fs1.dat.ci0123 y fs2.dat.ci0123 que usa la simulacion
discos:
	$(MAKE) -C $(ALMACEN) discos

clean:
	rm -f simulacion
	-ipcrm -M 0xB54321 2>/dev/null