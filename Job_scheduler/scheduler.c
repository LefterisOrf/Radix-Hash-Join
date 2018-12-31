#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "scheduler.h"

void* thread_function();

int scheduler_init(scheduler** sched, int num_of_threads)
{
	(*sched) = malloc(sizeof(scheduler));
	(*sched)-> thread_array = malloc(num_of_threads * sizeof(pthread_t));
	(*sched)->num_of_threads = num_of_threads;
	(*sched)->job_queue = NULL;

	pthread_mutex_init(&((*sched)->barrier_mutex), NULL);
	pthread_mutex_init(&((*sched)->queue_access), NULL);

	pthread_cond_init(&((*sched)->empty), NULL);
	pthread_cond_init(&((*sched)->barrier_cond), NULL);

	(*sched)->exit_all = 0;
	(*sched)->active_jobs = 0;
	(*sched)->answers_waiting = 0;

	for (int i = 0; i < num_of_threads; ++i)
	{
		pthread_create(&((*sched)->thread_array[i]),NULL,thread_function,(void*)(*sched));
	}
}

int push_job(scheduler* sched, int function, void *arguments)
{
	pthread_mutex_lock(&(sched->queue_access));
	if( (sched->job_queue) == NULL)
	{ 
		(sched->job_queue) = malloc (sizeof(jobqueue_node));
		(sched->job_queue)->function = function;
		(sched->job_queue)->arguments = arguments;
		(sched->job_queue)->next = NULL;
	}

	// Insert at end
	else
	{
		jobqueue_node* temp = (sched->job_queue);
		while( temp->next != NULL)
		{
			temp = temp->next;
		}

		temp->next = malloc (sizeof(jobqueue_node));
		temp->next->function = function;
		temp->next->arguments = arguments;
		temp->next->next = NULL;
	}
	sched->active_jobs++;
	pthread_cond_signal(&(sched)->empty);
	pthread_mutex_unlock(&(sched->queue_access));
}

jobqueue_node* pop_job(jobqueue_node** job_queue)
{
	jobqueue_node* temp = (*job_queue);
	(*job_queue) = (*job_queue)->next;
	return temp;
}

void* thread_function(void* arg)
{
	scheduler* sched = (scheduler*)arg;
	while(1)
	{ 
		pthread_mutex_lock(&(sched->queue_access));

		//while the job queue is empty wait
		while(sched->active_jobs <= 0 && sched->exit_all == 0)
			pthread_cond_wait(&(sched->empty), &(sched->queue_access));

		if(sched->exit_all == 1)
		{
			pthread_mutex_unlock(&(sched->queue_access));
			break;
		}


		jobqueue_node* job = NULL;
		job = pop_job(&(sched->job_queue));
		sched->active_jobs --;
		pthread_mutex_unlock(&(sched->queue_access));


		//execute job
		if (job->function == 0)
			HistJob(job->arguments);
		printf("job done\n");
		

		pthread_mutex_lock(&(sched->barrier_mutex));
		sched->answers_waiting--;
		if (sched->answers_waiting == 0)
			pthread_cond_signal(&(sched->barrier_cond));
		pthread_mutex_unlock(&(sched->barrier_mutex));

		free(job);
	}
	printf("Thread %lu exited\n",pthread_self());
	pthread_exit(NULL);
}