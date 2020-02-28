#include <mpi.h>
#include <nbc.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char** argv) {

	int commsize, myrank;
	int *mpisendbuf, *nbcsendbuf;
	int *mpirecvbuf, *nbcrecvbuf;
	NBC_Handle handle;
	MPI_Status status;

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &commsize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

	mpisendbuf = (int*) malloc(commsize*sizeof(int));
	mpirecvbuf = (int*) malloc(commsize*sizeof(int));
	nbcsendbuf = (int*) malloc(commsize*sizeof(int));
	nbcrecvbuf = (int*) malloc(commsize*sizeof(int));
	assert(mpisendbuf);
	assert(mpirecvbuf);
	assert(nbcsendbuf);
	assert(nbcrecvbuf);
	for (int r=0; r<commsize; r++) {
		mpisendbuf[r] = r*r+myrank;
		nbcsendbuf[r] = r*r+myrank;
	}

	MPI_Alltoall(mpisendbuf, 1, MPI_INT, mpirecvbuf, 1, MPI_INT, MPI_COMM_WORLD);
	NBC_Ialltoall(nbcsendbuf, 1, MPI_INT, nbcrecvbuf, 1, MPI_INT, MPI_COMM_WORLD, &handle);
	NBC_Wait(&handle, &status);
  
	// validate
	for (int r=0; r<commsize; r++) {
		assert(mpirecvbuf[r] == nbcrecvbuf[r]);
	}

	free(mpirecvbuf);
	free(mpisendbuf);
	free(nbcsendbuf);
	free(nbcrecvbuf);

	MPI_Finalize();

}
