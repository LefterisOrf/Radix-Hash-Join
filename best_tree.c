#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "structs.h"
#include "best_tree.h"
#include "query.h"
#include "stats.h"


void InitBestTree(best_tree_node*** best_tree, int num_of_relations)
{
  int best_tree_size = (int)pow(2, num_of_relations);
  (*best_tree) = malloc( best_tree_size * sizeof(best_tree_node*));
  for (size_t i = 1; i < best_tree_size; i++)
  {
    (*best_tree)[i] = malloc(sizeof(best_tree_node));
    (*best_tree)[i]->best_tree = NULL;
    (*best_tree)[i]->tree_stats = NULL;
    (*best_tree)[i]->num_predicates=0;
    (*best_tree)[i]->cost = 0;
    //Find the number of bits in the i
    int num = i, bit_num = 0;
    while(num != 0)
    {
      if (num % 2 == 1)
        bit_num++;
      num = num >> 1;
    }
    (*best_tree)[i]->active_bits = bit_num;

    //Set the column stats here
    if(bit_num != 1)
    	(*best_tree)[i]->tree_stats = calloc(num_of_relations,sizeof(column_stats**));
  }
  return;
}

void FreeBestTree(best_tree_node **best_tree,batch_listnode *curr_query,relation_map* rel_map)
{
  int size = pow(2, curr_query->num_of_relations);
  for (size_t i = 1; i < size; i++)
  {
    if (best_tree[i]->best_tree != NULL)
    	FreePredicateList(best_tree[i]->best_tree);
    if(best_tree[i]->tree_stats != NULL)
      FreeQueryStats(best_tree[i]->tree_stats,curr_query,rel_map);
    free(best_tree[i]);
  }
  free(best_tree);
}


predicates_listnode* Connected(best_tree_node **best_tree, int num_of_relations, int S, int R, predicates_listnode *list )
{
  //All relations that are active in S will be stored in active_rel array
	//fprintf(stderr, "connected %d %d \n",S,R );
  //Active rel can be malloced at the JoinEnum function so we dont have to malloc it every time
  short int *active_rel = calloc(num_of_relations, sizeof(short int));
  predicates_listnode *head = best_tree[S]->best_tree;

    int num = S , bit_num = 0, i =0;
    while(num != 0)
    {
      if (num % 2 == 1)
        active_rel[i]=1;
      num = num >> 1;
      i++;
    }
    /*for (int i = 0; i < num_of_relations; ++i)
    {
    	fprintf(stderr, "active_rel[%d] %d\n",i,active_rel[i] );
    }*/
  /*while(head != NULL)
  {
    active_rel[ head->join_p->relation1 ] = 1;
    active_rel[ head->join_p->relation2 ] = 1;
    head = head->next;
  }*/

  //Traverse through the query predicates
  while(list != NULL)
  {
    if(list->join_p->relation1 == R ) {
      if(active_rel[list->join_p->relation2] == 1){
        free(active_rel);
        return list;
      }
    }
    else if (list->join_p->relation2 == R)
    {
      if(active_rel[list->join_p->relation1] == 1){
        free(active_rel);
        //fprintf(stderr, "join %d %d\n",list->join_p->relation1,list->join_p->relation2 );
        return list;
      }
    }
    list = list->next;
  }
  free(active_rel);
  return NULL;
}



predicates_listnode* JoinEnum(batch_listnode* curr_query, column_stats*** query_stats,relation_map* rel_map)
{
	predicates_listnode *temp_pred = NULL;
	best_tree_node **best_tree = NULL, *curr_tree = NULL;
	InitBestTree(&best_tree, curr_query->num_of_relations);
	int s_new;

  //the size of the tree is equal to 2^relations
	int best_tree_size = (int)pow(2, curr_query->num_of_relations);

	for (int i = 1; i < curr_query->num_of_relations; ++i)
	{
    // For all subsets S of the relations of size i
		for (int s = 1; s < best_tree_size; ++s)
		{
			if(best_tree[s]->active_bits == i)
			{
        // For all the relations that are not in S
				for (int j = 1; j <= curr_query->num_of_relations; ++j)
				{
					if ( (((int)pow(2,j-1)) & s) != 0)continue;
          // If relation Rj is connected to S
					temp_pred = Connected(best_tree,curr_query->num_of_relations, s, j-1,curr_query->predicate_list);
					if (temp_pred == NULL)continue;

					s_new = ( (s | (int)pow(2,j-1)));

          // Create left deep tree that contains this relation
					CreateJoinTree(&curr_tree,best_tree[s],curr_query,rel_map,s_new);
					InserPredAtEnd(curr_tree,temp_pred,query_stats,rel_map,curr_query);

          // Find the cost of the current tree
          if(s_new != best_tree_size-1)
					 CostTree(curr_tree,curr_query,temp_pred,rel_map);

          //Update Best Tree
					if(best_tree[s_new]->best_tree == NULL || best_tree[s_new]->cost > curr_tree->cost)
					{
						if(best_tree[s_new]->best_tree == NULL || best_tree[s_new]->num_predicates == curr_tree->num_predicates)
						{
                FreeQueryStats(best_tree[s_new]->tree_stats,curr_query,rel_map);
  							FreePredicateList(best_tree[s_new]->best_tree);
  							free(best_tree[s_new]);
							  best_tree[s_new] = curr_tree;
						}
            else
            {
              FreeQueryStats(curr_tree->tree_stats,curr_query,rel_map);
              FreePredicateList(curr_tree->best_tree);
              free(curr_tree);
              curr_tree = NULL;
            }

					}
					else
					{
						FreeQueryStats(curr_tree->tree_stats,curr_query,rel_map);
						FreePredicateList(curr_tree->best_tree);
						free(curr_tree);
						curr_tree = NULL;
					}
				}
			}
		}
	}

	predicates_listnode *return_list =best_tree[best_tree_size-1]->best_tree;
	best_tree[best_tree_size-1]->best_tree = NULL;

  /* Some queries have two predicates that contain the same relations
   * JoinEnum returns a list that contain only one of the two and so
   * if the number of predicates of the best tree is less than the
   * number of predicates of the current query, find these predicates
   * and insert them at end */
  predicates_listnode* temp_old = curr_query->predicate_list;

  int found, temp_num_pred = 0;
  while(temp_old!=NULL)
  {
    temp_num_pred++;
    temp_old= temp_old->next;
  }
  temp_old = curr_query->predicate_list;
  if(temp_num_pred != best_tree[best_tree_size-1]->num_predicates )
  {
    //check if there were any predicates left out from JoinEnum
    predicates_listnode* temp_new;
    while(temp_old!=NULL)
    {
      temp_new = return_list;
      while(temp_new!=NULL)
      {
        if((temp_old->join_p->relation1 == temp_new->join_p->relation1 ||
           temp_old->join_p->relation1 == temp_new->join_p->relation2) &&
           (temp_old->join_p->relation2 == temp_new->join_p->relation2 ||
            temp_old->join_p->relation2 == temp_new->join_p->relation1 )&&
           (temp_old->join_p->column1 != temp_new->join_p->column1 ||
           temp_old->join_p->column2 != temp_new->join_p->column2 ))
        {
        predicates_listnode *temp= malloc (sizeof(predicates_listnode));
        temp->next = temp_new->next;
        temp_new->next = temp;
        temp->filter_p = NULL;
        temp->join_p = malloc(sizeof(join_pred));
        temp->join_p->relation1=temp_old->join_p->relation1;
        temp->join_p->relation2=temp_old->join_p->relation2;
        temp->join_p->column1=temp_old->join_p->column1;
        temp->join_p->column2=temp_old->join_p->column2;
        }
        if(temp_new->next == NULL)break;
        temp_new = temp_new->next;
      }
      temp_old = temp_old->next;
    }
  }
  FreePredicateList(curr_query->predicate_list);
	FreeBestTree(best_tree, curr_query,rel_map);
	return return_list;
}

int CreateJoinTree(best_tree_node **dest, best_tree_node* source ,batch_listnode* curr_query,relation_map* rel_map,int s_new)
{
	(*dest) = malloc(sizeof(best_tree_node));
    (*dest)->best_tree = NULL;
    //Find the number of bits in the i
    int num = s_new, bit_num = 0;
    while(num != 0)
    {
      if (num % 2 == 1)
        bit_num++;
      num = num >> 1;
    }
    (*dest)->active_bits = bit_num;
    (*dest)->num_predicates = 0;
    //Set the column stats here

    (*dest)->tree_stats = calloc(curr_query->num_of_relations,sizeof(column_stats**));
    predicates_listnode* source_tree = source->best_tree;
    while(source_tree!=NULL)
    {
    	InserPredAtEnd((*dest) ,source_tree, source->tree_stats,rel_map,curr_query);
    	source_tree=source_tree->next;
    }
    (*dest)->cost = source->cost;
}

int InserPredAtEnd(best_tree_node* tree, predicates_listnode* pred,column_stats ***query_stats,relation_map* rel_map,batch_listnode* curr_query)
{
  tree->num_predicates++;
  if (tree->tree_stats[pred->join_p->relation1] == NULL)
  {
      tree->tree_stats[pred->join_p->relation1] =
    calloc(rel_map[curr_query->relations[pred->join_p->relation1]].num_columns , sizeof(column_stats*));

    for (int i = 0; i < rel_map[curr_query->relations[pred->join_p->relation1]].num_columns ; ++i)
    {
      if(query_stats[pred->join_p->relation1][i] == NULL)continue;

      tree->tree_stats[pred->join_p->relation1][i] =  malloc(sizeof(column_stats));
      column_stats* stats = tree->tree_stats[pred->join_p->relation1][i];
      stats->l = query_stats[pred->join_p->relation1][i]->l;
      stats->u = query_stats[pred->join_p->relation1][i]->u;
      stats->f = query_stats[pred->join_p->relation1][i]->f;
      stats->d = query_stats[pred->join_p->relation1][i]->d;
    }
  }
  if (tree->tree_stats[pred->join_p->relation2] == NULL)
  {
      tree->tree_stats[pred->join_p->relation2] =
    calloc(rel_map[curr_query->relations[pred->join_p->relation2]].num_columns , sizeof(column_stats*));

    for (int i = 0; i < rel_map[curr_query->relations[pred->join_p->relation2]].num_columns ; ++i)
    {
      if(query_stats[pred->join_p->relation2][i] == NULL)continue;

      tree->tree_stats[pred->join_p->relation2][i] =  malloc(sizeof(column_stats));
      column_stats* stats = tree->tree_stats[pred->join_p->relation2][i];
      stats->l = query_stats[pred->join_p->relation2][i]->l;
      stats->u = query_stats[pred->join_p->relation2][i]->u;
      stats->f = query_stats[pred->join_p->relation2][i]->f;
      stats->d = query_stats[pred->join_p->relation2][i]->d;
    }
  }
  if(tree->best_tree== NULL )
  {
    tree->best_tree = malloc(sizeof(predicates_listnode));
    tree->best_tree->next = NULL;
    tree->best_tree->filter_p = NULL;
    tree->best_tree->join_p = malloc(sizeof(predicates_listnode));
    tree->best_tree->join_p->relation1 = pred->join_p->relation1;
    tree->best_tree->join_p->relation2 = pred->join_p->relation2;
    tree->best_tree->join_p->column1 = pred->join_p->column1;
    tree->best_tree->join_p->column2 = pred->join_p->column2;
  }
  else
  {
    predicates_listnode *temp = tree->best_tree;
    while(temp->next != NULL)
       temp = temp->next;

    temp->next = malloc(sizeof(predicates_listnode));
    temp->next->filter_p = NULL;
    temp->next->next = NULL;
    temp->next->join_p = malloc(sizeof(predicates_listnode));
    temp->next->join_p->relation1 = pred->join_p->relation1;
    temp->next->join_p->relation2 = pred->join_p->relation2;
    temp->next->join_p->column1 = pred->join_p->column1;
    temp->next->join_p->column2 = pred->join_p->column2;
  }
}

void CostTree(best_tree_node *curr_tree, batch_listnode* curr_query, predicates_listnode* pred,relation_map *rel_map)
{
	ValuePredicate(curr_tree->tree_stats,curr_query,pred,rel_map);
	curr_tree->cost = (curr_tree->cost + curr_tree->tree_stats[pred->join_p->relation1][pred->join_p->column1]->f);
}
