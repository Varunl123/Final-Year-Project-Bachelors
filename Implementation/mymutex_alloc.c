#define _GNU_SOURCE
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<sched.h>
#include "mymutex.h"
#define LIMIT 100000000

long long sum = 0;
mutex *mut;
void * increment( void * arg ){
  for ( int i = 0 ; i <= LIMIT ; i ++ ){
    mutex_lock(mut);
    sum += 1;
    mutex_unlock(mut);
  }
  pthread_exit ( 0 );
}

void * decrement( void * arg ){
  for ( int i = 0 ; i <= LIMIT ; i ++ ){
    mutex_lock(mut);
    sum -= 1;
    mutex_unlock(mut);
  }
  pthread_exit ( 0 );
}
int main( void ){
pthread_t t1,t2;
cpu_set_t cpu1, cpu2;
pthread_attr_t *attr1, *attr2;
attr1 = numa_alloc_onnode ( sizeof ( pthread_attr_t) , 0 );
attr2 = numa_alloc_onnode (sizeof (pthread_attr_t),1 );
pthread_attr_init ( attr1 );
CPU_ZERO(&cpu1);
CPU_SET( 1 , &cpu1 );
pthread_attr_setaffinity_np( attr1 , sizeof ( cpu_set_t ) , &cpu1 );
pthread_attr_init ( attr2 );
mut = malloc ( sizeof ( pthread_mutex_t) );
mutex_init ( mut, NULL );
pthread_attr_init ( attr2 );
CPU_ZERO(&cpu2);
CPU_SET( 9 , &cpu2 );
pthread_attr_setaffinity_np( attr2 , sizeof ( cpu_set_t ) , &cpu2 );

mut = numa_alloc_nonode ( sizeof ( pthread_mutex_t) ,  0 );
mutex_init ( mut, NULL );

pthread_create ( &t1 , attr1, increment , NULL );
pthread_create( &t2, attr2 , decrement , NULL );
pthread_join ( t1 , NULL );
pthread_join( t2 , NULL );
printf("\n sum = %lld \n" , sum );
exit(0);
}

