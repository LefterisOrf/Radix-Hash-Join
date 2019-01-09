#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>

#define CACHE_SIZE 32768 //L1 cache is 32 KB
#define RESULT_MAX_BUFFER 131072 //number of bits in a result node buffer
#define RESULT_FINAL_BUFFER 1048576
#define N_LSB 2 //number of least significant bits used in H1

/** Type definition for a tuple */
typedef struct tuple
{
	uint64_t value;
	uint64_t row_id;
}tuple;

/*
 * Type definition for a relation
 * It consists of an array of tuples and the size of a relation
 */
typedef struct relation
{
	tuple* tuples;
	uint64_t num_tuples;
}relation;

/*
 * Type definition for a result node
 * It consists of a buffer that will contain result_tuples
 * A pointer to the next node
 * and the number of records in the node
 */
typedef struct result
{
	char *buff;
	struct result* next;
	uint64_t current_load;

}result;

/** Struct that contains two tuples as a result from the join */
typedef struct result_tuple
{
	uint64_t row_idR;
	uint64_t row_idS;
}result_tuple;

typedef struct reorder_relation
{
	int hist_size;
	int64_t *psum;
	uint64_t *hist;
	relation* rel_array;
} reordered_relation;

/*
 * THe index that uses H2 as the hash function
 * Contains:
 * -the size of the index in buckets
 * -the start and end are pointers to the full array's start and end
 * of the bucket on which the index is created
 * -the index and chain arrays
 */
typedef struct bc_index
{
	int index_size;
	int start;
	int end;
	int64_t* bucket;
	int64_t* chain;
}bc_index;

/* list that holds the names of the files containing the relations
 * and their respective file descriptors
 */

typedef struct relation_listnode
{
	char* filename;
	int fd;
	struct relation_listnode *next;
}relation_listnode;

typedef struct column_stats
{
	uint64_t l;
	uint64_t u;
	double f;
	double d;
}column_stats;

/* struct that holds the relations after being mapped
 * from the relation files given. it contains
 * the number of rows, the number of comuns
 * and an array that has pointers to the start of each column
 */
typedef struct relation_map
{
	uint64_t num_tuples;
	uint64_t num_columns;
	uint64_t **columns;
	column_stats *col_stats;
}relation_map;

/* Struct that holds the info for a filter predicate */
typedef struct filter_pred
{
	int relation;
	int column;
	int value;
	char comperator;
}filter_pred;

/* Struct that holds the info for a join predicate */

typedef struct join_pred
{
	int relation1;
	int relation2;
	int column1;
	int column2;
}join_pred;

/*
 * List that holds the predicates of a query. If a node contains a filter predicate
 * filter_p holds it and join_p is NULL. Else if it contains a join filter, join_p
 * holds it and filter_p is NULL. The filter predicates are stored in the head of the list
 * and join predicates are stored at the end.
 */
typedef struct predicates_listnode
{
	filter_pred *filter_p;
	join_pred *join_p;
	struct predicates_listnode *next;
}predicates_listnode;

/*
 * Struct used to hold the predicates or views given in a query in
 * the places of data.
 */
typedef struct query_string_array
{
  char **data;
  int num_of_elements;
}query_string_array;

/*
 * List whose every node holds everything needed for the execution of a batch.
 * predicate_list holds the predicates, views holds the views and relations, holds
 * the number of the relations as stored in the relation map.
 */
typedef struct query_batch_listnode
{
	int num_of_relations;
	int *relations;
	predicates_listnode * predicate_list;
	query_string_array *views;
	struct query_batch_listnode *next;
}batch_listnode;

typedef struct jobqueue_node
{
  int function;
  void *arguments;
  struct jobqueue_node* next;
}jobqueue_node;

typedef struct scheduler
{
	int num_of_threads;
	pthread_t *thread_array;
	jobqueue_node* job_queue;

	pthread_cond_t barrier_cond;
	pthread_mutex_t queue_access;
	sem_t queue_sem;

	int exit_all;
	int answers_waiting;

}scheduler;

typedef struct hist_arguments
{
  int **hist;
  uint64_t start;
  uint64_t end;
  uint64_t n_lsb;
  uint64_t hist_size;
  relation *rel;
}hist_arguments;

typedef struct partition_arguments
{
	relation *reordered;
	relation *original;
	uint64_t start;
	uint64_t end;
  	uint64_t hist_size;
	uint64_t n_lsb;
	uint64_t *psum;
}part_arguments;

typedef struct join_arguments
{
	reordered_relation *NewR;
	reordered_relation *NewS;
	result **res;
	uint64_t bucket_num;
}join_arguments;

typedef struct best_tree_node
{
	predicates_listnode* best_tree;
	int single_relation;
	int active_bits;
	double cost;
	column_stats ***tree_stats;
}best_tree_node;


#endif
