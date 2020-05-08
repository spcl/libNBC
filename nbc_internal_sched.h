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

/* several typedefs for NBC */

/* the function type enum */
typedef enum {
  SEND,
  RECV,
  OP,
  COPY,
  UNPACK
} NBC_Fn_type;

/* the send argument struct */
typedef struct {
  void *buf;
  char tmpbuf;
  int count;
  MPI_Datatype datatype;
  int dest;
} NBC_Args_send;

/* the receive argument struct */
typedef struct {
  void *buf;
  char tmpbuf;
  int count;
  MPI_Datatype datatype;
  int source;
} NBC_Args_recv;

/* the operation argument struct */
typedef struct {
  void *buf1;
  char tmpbuf1;
  void *buf2;
  char tmpbuf2;
  void *buf3;
  char tmpbuf3;
  int count;
  MPI_Op op;
  MPI_Datatype datatype;
} NBC_Args_op;

/* the copy argument struct */
typedef struct {
  void *src; 
  char tmpsrc;
  int srccount;
  MPI_Datatype srctype;
  void *tgt;
  char tmptgt;
  int tgtcount;
  MPI_Datatype tgttype;
} NBC_Args_copy;

/* unpack operation arguments */
typedef struct {
  void *inbuf; 
  char tmpinbuf;
  int count;
  MPI_Datatype datatype;
  void *outbuf; 
  char tmpoutbuf;
} NBC_Args_unpack;



