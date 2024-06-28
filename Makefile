CXX = gcc
APP=$(shell pwd)
CXX_FLAGS = -Wall -Wextra -O3 -g -DOMPI_SKIP_MPICXX -lpthread -lcurl -ljansson
DMRFLAGS = -I$(APP) -L$(APP) -ldyn -lset
FLAGS  = -O3 -Wall -g
DYN_PSETS := /opt/hpc/install/dyn_psets

all: set dyn sessions

api: api.c 
	mpicc  -o api api.c -Wall -Wextra -O3 -g -DOMPI_SKIP_MPICXX -lulfius

deneme: deneme.c 
	mpicc  -o deneme deneme.c -Wall -Wextra -O3 -g -DOMPI_SKIP_MPICXX

mpi: mpii.c
	mpicc  -o mpii mpii.c $(CXX_FLAGS) 

sessions: sessions.c
	mpicc $(CXX_FLAGS) $(DMRFLAGS) -lmpi -o sessions sessions.c

grow: grow.c
	mpic++ -o grow grow.c $(DMRFLAGS) $(CXX_FLAGS) 

lib: dmr.c
	mpic++ $(FLAGS) $(MPIFLAGS) -c -fPIC dmr.c -o dmr.o
	mpic++ dmr.o -shared -o libdmr.so

set: dyn_psets.c
	mpicc $(FLAGS) $(MPIFLAGS) -c -fPIC dyn_psets.c -o set.o
	mpicc set.o -shared -o libset.so

dyn: dyn.c
	mpicc $(FLAGS) $(MPIFLAGS) -c -fPIC dyn.c -o dyn.o
	mpicc dyn.o -shared -o libdyn.so

sleep: sleep.c
	mpic++ $(MPIFLAGS) $(FLAGS) $(DMRFLAGS)  $(CXX_FLAGS)  sleep.c -o tt

clean:
	rm -f denem denem.env *.o

working:
	mpic++ -shared -o dyn.so dyn.c -fPIC -ljansson