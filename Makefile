CXX = gcc
APP=$(shell pwd)
CXX_FLAGS = -Wall -Wextra -O3 -g -DOMPI_SKIP_MPICXX -lpthread -lcurl -ljansson
DMRFLAGS = -I$(APP) -L$(APP) -ldmr -ldyn
FLAGS  = -O3 -Wall -g

all: dyn lib sessions sleep deneme

api: api.c 
	mpicc  -o api api.c -Wall -Wextra -O3 -g -DOMPI_SKIP_MPICXX -lulfius

deneme: deneme.c 
	mpicc  -o deneme deneme.c -Wall -Wextra -O3 -g -DOMPI_SKIP_MPICXX

mpi: mpii.c
	mpicc  -o mpii mpii.c $(CXX_FLAGS) 

sessions: sessions.c
	mpic++ -o sessions sessions.c $(DMRFLAGS) $(CXX_FLAGS) 

lib: dmr.c
	mpic++ $(FLAGS) $(MPIFLAGS) -c -fPIC dmr.c -o dmr.o
	mpic++ dmr.o -shared -o libdmr.so


dyn: dyn.c
	gcc -shared -o dyn.so dyn.c -ljansson

sleep: sleep.c
	mpic++ $(MPIFLAGS) $(FLAGS) $(DMRFLAGS)  $(CXX_FLAGS)  sleep.c -o tt


clean:
	rm -f denem denem.env *.o