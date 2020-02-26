#include <mpi.h>
#include <nbc.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char** argv) {

	int commsize, myrank;
	int *mpibuf, *nbcbuf;
	NBC_Handle handle;
	MPI_Status status;

	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &commsize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

	mpibuf = (int*) malloc(commsize*sizeof(int));
	nbcbuf = (int*) malloc(commsize*sizeof(int));
	assert(mpibuf);
	assert(nbcbuf);
	for (int r=0; r<commsize; r++) {
		mpibuf[r] = r*r+myrank;
		nbcbuf[r] = r*r+myrank;
	}

	MPI_Bcast(mpibuf, commsize, MPI_INT, 0, MPI_COMM_WORLD);
	NBC_Ibcast(nbcbuf, commsize, MPI_INT, 0, MPI_COMM_WORLD, &handle);
	NBC_Wait(&handle, &status);

	// validate
	for (int r=0; r<commsize; r++) {
		assert(mpibuf[r] == nbcbuf[r]);
	}

	free(mpibuf);
	free(nbcbuf);

	MPI_Finalize();

}
