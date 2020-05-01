/*
 * Copyright (c) 2006 The Trustees of Indiana University and Indiana
 *                    University Research and Technology
 *                    Corporation.  All rights reserved.
 * Copyright (c) 2006 The Technical University of Chemnitz. All 
 *                    rights reserved.
 *
 * Author(s): Torsten Hoefler <htor@cs.indiana.edu>
 *
 */
#include "nbc_internal.h"

// cannot cache schedules because one cannot check locally if the pattern is the same!!
#undef NBC_CACHE_SCHEDULE

#ifdef NBC_CACHE_SCHEDULE
/* tree comparison function for schedule cache */
int NBC_Ineighbor_alltoallv_args_compare(NBC_Ineighbor_alltoallv_args *a, NBC_Ineighbor_alltoallv_args *b, void *param) {

	if( (a->sbuf == b->sbuf) && 
      (a->scount == b->scount) && 
      (a->stype == b->stype) &&
      (a->rbuf == b->rbuf) && 
      (a->rcount == b->rcount) && 
      (a->rtype == b->rtype) ) {
      return  0;
    }
	if( a->sbuf < b->sbuf ) {	
      return -1;
	}
	return +1;
}
#endif


#ifdef HAVE_SYS_WEAK_ALIAS_PRAGMA
#pragma weak NBC_Ineighbor_alltoallv=PNBC_Ineighbor_alltoallv
#define NBC_Ineighbor_alltoallv PNBC_Ineighbor_alltoallv
#endif
int NBC_Ineighbor_alltoallv(void *sbuf, int *scounts, int *sdispls, MPI_Datatype stype,
        void *rbuf, int *rcounts, int *rdispls, MPI_Datatype rtype, MPI_Comm comm, NBC_Handle* handle) {
  int rank, size, res, worldsize, i;
  MPI_Aint sndext, rcvext, lb;
  
  double t[10];
  t[0] = PMPI_Wtime();

  res = NBC_Init_handle(handle, comm);
  if(res != NBC_OK) { printf("Error in NBC_Init_handle(%i)\n", res); return res; }
  res = MPI_Comm_size(comm, &size);
  if (MPI_SUCCESS != res) { printf("MPI Error in MPI_Comm_size() (%i)\n", res); return res; }
  res = MPI_Comm_size(MPI_COMM_WORLD, &worldsize);
  if (MPI_SUCCESS != res) { printf("MPI Error in MPI_Comm_size() (%i)\n", res); return res; }
  res = MPI_Comm_rank(comm, &rank);
  if (MPI_SUCCESS != res) { printf("MPI Error in MPI_Comm_rank() (%i)\n", res); return res; }

  res = MPI_Type_get_extent(stype, &lb, &sndext);
  if (MPI_SUCCESS != res) { printf("MPI Error in MPI_Type_get_extent() (%i)\n", res); return res; }
  res = MPI_Type_get_extent(rtype, &lb, &rcvext);
  if (MPI_SUCCESS != res) { printf("MPI Error in MPI_Type_get_extent() (%i)\n", res); return res; }

  t[1] = PMPI_Wtime();

  char inplace;
  NBC_Schedule *schedule;
#ifdef NBC_CACHE_SCHEDULE
  NBC_Ineighbor_alltoallv_args *args, *found, search;
#endif

  NBC_IN_PLACE(sbuf, rbuf, inplace);
  
  handle->tmpbuf=NULL;

#ifdef NBC_CACHE_SCHEDULE
  /* search schedule in communicator specific tree */
  search.sbuf=sbuf;
  search.scount=scount;
  search.stype=stype;
  search.rbuf=rbuf;
  search.rcount=rcount;
  search.rtype=rtype;
  found = (NBC_Ineighbor_alltoallv_args*)hb_tree_search((hb_tree*)handle->comminfo->NBC_Dict[NBC_NEIGHBOR_ALLTOALLV], &search);
  if(found == NULL) {
#endif
    schedule = (NBC_Schedule*)malloc(sizeof(NBC_Schedule));
    
    res = NBC_Sched_create(schedule);
    if(res != NBC_OK) { printf("Error in NBC_Sched_create, res = %i\n", res); return res; }

    {
      int indegree, outdegree, weighted, *srcs, *dsts, i;
      res = NBC_Comm_neighbors_count(comm, &indegree, &outdegree, &weighted);
      if(res != NBC_OK) return res;

      srcs = (int*)malloc(sizeof(int)*indegree);
      dsts = (int*)malloc(sizeof(int)*outdegree);

      res = NBC_Comm_neighbors(comm, indegree, srcs, MPI_UNWEIGHTED, outdegree, dsts, MPI_UNWEIGHTED);
      if(res != NBC_OK) return res;

      if(inplace) { /* we need an extra buffer to be deadlock-free */
        int sumrcounts=0;
        int offset=0;
        for(i=0; i<indegree; ++i) sumrcounts += rcounts[i];
        handle->tmpbuf = malloc(rcvext*sumrcounts);

        for(i = 0; i < indegree; i++) {
          if(srcs[i] != MPI_PROC_NULL) {
            res = NBC_Sched_recv((char*)0+offset, true, rcounts[i], rtype, srcs[i], schedule);
            if (NBC_OK != res) { printf("Error in NBC_Sched_recv() (%i)\n", res); return res; }
          }
          offset += rcounts[i]*rcvext;
        }

        for(i = 0; i < outdegree; i++) {
          if(dsts[i] != MPI_PROC_NULL) {
            res = NBC_Sched_send((char*)sbuf+sdispls[i]*sndext, false, scounts[i], stype, dsts[i], schedule);
            if (NBC_OK != res) { printf("Error in NBC_Sched_send() (%i)\n", res); return res; }
          }
        }
        /* unpack from buffer */
        offset=0;
        for(i = 0; i < indegree; i++) {
          if(srcs[i] != MPI_PROC_NULL) {
            res = NBC_Sched_barrier(schedule);
            if (NBC_OK != res) { printf("Error in NBC_Sched_barrier() (%i)\n", res); return res; }
            res = NBC_Sched_copy((char*)0+offset, true, rcounts[i], rtype, (char*)rbuf+rdispls[i]*rcvext, false, rcounts[i], rtype, schedule);
            if (NBC_OK != res) { printf("Error in NBC_Sched_copy() (%i)\n", res); return res; }
          }
          offset += rcounts[i]*rcvext;
        }
      } else { /* non INPLACE case */
        /* simply loop over neighbors and post send/recv operations */
        for(i = 0; i < indegree; i++) {
          if(srcs[i] != MPI_PROC_NULL) {
            res = NBC_Sched_recv((char*)rbuf+rdispls[i]*rcvext, false, rcounts[i], rtype, srcs[i], schedule);
            if (NBC_OK != res) { printf("Error in NBC_Sched_recv() (%i)\n", res); return res; }
          }
        }
        for(i = 0; i < outdegree; i++) {
          if(dsts[i] != MPI_PROC_NULL) {
            res = NBC_Sched_send((char*)sbuf+sdispls[i]*sndext, false, scounts[i], stype, dsts[i], schedule);
            if (NBC_OK != res) { printf("Error in NBC_Sched_send() (%i)\n", res); return res; }
          }
        }
      }
    }
    
    res = NBC_Sched_commit(schedule);
    if (NBC_OK != res) { printf("Error in NBC_Sched_commit() (%i)\n", res); return res; }
#ifdef NBC_CACHE_SCHEDULE
    /* save schedule to tree */
    args = (NBC_Ineighbor_alltoallv_args*)malloc(sizeof(NBC_Ineighbor_alltoallv_args));
    args->sbuf=sbuf;
    args->scount=scount;
    args->stype=stype;
    args->rbuf=rbuf;
    args->rcount=rcount;
    args->rtype=rtype;
    args->schedule=schedule;
	  res = hb_tree_insert ((hb_tree*)handle->comminfo->NBC_Dict[NBC_NEIGHBOR_ALLTOALLV], args, args, 0);
    if(res != 0) printf("error in dict_insert() (%i)\n", res);
    /* increase number of elements for A2A */
    if(++handle->comminfo->NBC_Dict_size[NBC_NEIGHBOR_ALLTOALLV] > NBC_SCHED_DICT_UPPER) {
      NBC_SchedCache_dictwipe((hb_tree*)handle->comminfo->NBC_Dict[NBC_NEIGHBOR_ALLTOALLV], &handle->comminfo->NBC_Dict_size[NBC_NEIGHBOR_ALLTOALLV]);
    }
  } else {
    /* found schedule */
    schedule=found->schedule;
  }
#endif
  
  res = NBC_Start(handle, schedule);
  if (NBC_OK != res) { printf("Error in NBC_Start() (%i)\n", res); return res; }

  return NBC_OK;
}

#ifdef __cplusplus
extern "C" {
#endif


#ifdef HAVE_SYS_WEAK_ALIAS_PRAGMA
NBC_F77_ALLFUNC_(nbc_ineighbor_alltoallv,NBC_INEIGHBOR_ALLTOALLV,(void *sbuf, int *scounts, int* sdispls, int *stype, void *rbuf, int *rcounts,
        int *rdispls, int *rtype, int *fcomm, int *fhandle, int *ierr));
#pragma weak NBC_INEIGHBOR_ALLTOALLV = nbc_ineighbor_alltoallv_f
#pragma weak nbc_ineighbor_alltoallv = nbc_ineighbor_alltoallv_f
#pragma weak nbc_ineighbor_alltoallv_ = nbc_ineighbor_alltoallv_f
#pragma weak nbc_ineighbor_alltoallv__ = nbc_ineighbor_alltoallv_f
#pragma weak PNBC_INEIGHBOR_ALLTOALLV = nbc_ineighbor_alltoallv_f
#pragma weak pnbc_ineighbor_alltoallv = nbc_ineighbor_alltoallv_f
#pragma weak pnbc_ineighbor_alltoallv_ = nbc_ineighbor_alltoallv_f
#pragma weak pnbc_ineighbor_alltoallv__ = nbc_ineighbor_alltoallv_f
void nbc_ineighbor_alltoallv_f(void *sbuf, int *scounts, int *sdispls,  int *stype, void *rbuf, int *rcounts,
        int *rdispls, int *rtype, int *fcomm, int *fhandle, int *ierr) 
#else
void NBC_F77_FUNC_(nbc_ineighbor_alltoallv,NBC_INEIGHBOR_ALLTOALLV)(void *sbuf, int *scounts, int *sdispls, int *stype, void *rbuf, int *rcounts,
        int *rdispls, int *rtype, int *fcomm, int *fhandle, int *ierr);
void NBC_F77_FUNC_(nbc_ineighbor_alltoallv,NBC_INEIGHBOR_ALLTOALLV)(void *sbuf, int *scounts, int *sdispls, int *stype, void *rbuf, int *rcounts,
        int *rdispls, int *rtype, int *fcomm, int *fhandle, int *ierr)
#endif
{
  MPI_Datatype sdtype, rdtype;
  MPI_Comm comm;
  NBC_Handle *handle;

  /* this is the only MPI-2 we need :-( */
  sdtype = MPI_Type_f2c(*stype);
  rdtype = MPI_Type_f2c(*rtype);
  comm = MPI_Comm_f2c(*fcomm);

  /* create a new handle in handle table */
  NBC_Create_fortran_handle(fhandle, &handle);

  /* call NBC function */
  *ierr = NBC_Ineighbor_alltoallv(sbuf, scounts, sdispls, sdtype, rbuf, rcounts,
           rdispls, rdtype, comm, handle);
}
#ifdef __cplusplus
}
#endif


