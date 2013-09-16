#pragma once

#include <unistd.h>
#include "blis.h"
#include <omp.h>



typedef unsigned int rank_t;
typedef unsigned int thread_count_t;
typedef omp_lock_t lock_t;


typedef struct
{
  void*   sent_object;
  thread_count_t num_threads_in_group;

  //The communicator above has X sub-communicators.  That is the multiplicative_factor_above;
  thread_count_t multiplicative_factor_above;
  
  bool_t  barrier_sense;
  lock_t  barrier_lock;
  thread_count_t   barrier_threads_arrived;
} thread_comm_t;

void    th_setup_comm( thread_comm_t *comm, 
		       thread_count_t threads_in_group, 
		       thread_count_t multiplicative_factor_above );
void    th_release_comm( thread_comm_t *comm );
void    th_broadcast( thread_comm_t *comm, rank_t root, void *send, void *recv, unsigned int size );
void    th_broadcast_without_second_barrier( thread_comm_t *comm, rank_t root, 
					     void *send, void *recv, unsigned int size );
void    th_barrier( thread_comm_t *comm );
void    th_set_lock( lock_t *lock );
void    th_unset_lock( lock_t *lock );
void    th_init_lock( lock_t *lock );
void    th_destroy_lock( lock_t *lock );

bool_t  th_am_root( thread_comm_t *comm );

rank_t   th_thread_id( thread_comm_t *comm );
rank_t   th_group_id( thread_comm_t *comm );
rank_t   th_globa_group_id( thread_comm_t *comm );
rank_t   th_global_thread_id();

void th_shift_start_end(dim_t *start, dim_t *end, thread_comm_t *comm, dim_t round_factor);



