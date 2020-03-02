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

	
        MPI_Info info;
        MPI_Info_create(&info);
	MPI_Info_set(info, NBC_BCAST_ALG_INFO_KEY, NBC_BCAST_ALG_BINOMIAL);
        NBC_Comm_set_info(MPI_COMM_WORLD, info);

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
  
  
	MPI_Info_free(&info);


	// validate
	for (int r=0; r<commsize; r++) {
		assert(mpibuf[r] == nbcbuf[r]);
	}

	free(mpibuf);
	free(nbcbuf);

	MPI_Finalize();

}
