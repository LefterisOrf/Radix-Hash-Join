#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structs.h"
#include "query.h"
#include "results.h"
#include "filter.h"
#include "inter_res.h"

int InitInterData(inter_data** head, int num_of_relations, int num_tuples)
{
	(*head) = malloc(sizeof(struct inter_data));
	(*head)->num_tuples = num_tuples;
	(*head)->table = malloc(num_of_relations * sizeof(uint64_t *));
	for (int i = 0; i < num_of_relations; ++i)
		(*head)->table[i] = NULL;
}

void FreeInterData(inter_data *head, int num_of_relations)
{
	for (size_t i = 0; i < num_of_relations; i++)
		if (head->table[i] != NULL)
			free(head->table[i]);
	free(head->table);
	free(head);
}

int InitInterResults(inter_res** head, int num_of_rel)
{
	(*head) = malloc(sizeof(inter_res));
	(*head)->next = NULL;
	(*head)->num_of_relations = num_of_rel;
	InitInterData(&(*head)->data, num_of_rel, 0);
}

int InsertJoinToInterResults(inter_res** head, int rel1, int rel2, result* res)
{
	printf("\n\n\nStarted insert. Rel1: %d , Rel2: %d\n", rel1, rel2);
	while(1)
	{
		//If this is the first instance of the inter_res node
		if ((*head)->data->num_tuples == 0)
		{
			printf("\n\nInterRes: Num_tuples == 0\n\n");
			//Insert everything from the result to the inter_res
			(*head)->data->num_tuples = GetResultNum(res);
			(*head)->data->table[rel1] = malloc((*head)->data->num_tuples * sizeof(uint64_t));
			(*head)->data->table[rel2] = malloc((*head)->data->num_tuples * sizeof(uint64_t));
			for (size_t i = 0; i < (*head)->data->num_tuples; i++)
			{
				result_tuples *temp = FindResultTuples(res, i);
				(*head)->data->table[rel1][i] = temp->tuple_R.row_id;
				(*head)->data->table[rel2][i] = temp->tuple_S.row_id;
			}
			return 1;
		}
		// If rel1 is the one active in the intermediate results.
		else if((*head)->data->table[rel1] != NULL && (*head)->data->table[rel2] == NULL)
		{
			printf("\n\nInterRes: Rel1 is the only one active!\n\n");
			//Allocate and initialise the new inter_data variable.
			printf("Previous num_tuples = %lu\n", (*head)->data->num_tuples);
			(*head)->data->num_tuples = GetResultNum(res);
			printf("New num_tuples = %lu\n", (*head)->data->num_tuples);
			inter_data *temp_array = NULL;
			InitInterData(&temp_array, (*head)->num_of_relations, (*head)->data->num_tuples);
			for (size_t i = 0; i < (*head)->num_of_relations; i++)
				if((*head)->data->table[i] != NULL)
					temp_array->table[i] = malloc(((*head)->data->num_tuples) * sizeof(uint64_t));
			// [rel2] is still inactive , so we have to manually alocate it
			temp_array->table[rel2] = malloc((*head)->data->num_tuples * sizeof(uint64_t));

			//Insert the results.
			result_tuples *temp;
			int old_pos;
			for (size_t i = 0; i < (*head)->data->num_tuples; i++)
			{
				//Insert the results that are stored in the res variable.
				temp = FindResultTuples(res, i);
				//printf("Old_pos = %lu\n", temp->tuple_R.row_id);
				/* tuple_R.row_id is an index of the previous instance of the inter_res.*/
				old_pos = temp->tuple_R.row_id;
				temp_array->table[rel2][i] = temp->tuple_S.row_id;
				for (size_t j = 0; j < (*head)->num_of_relations; j++)
					if ((*head)->data->table[j] != NULL)
						temp_array->table[j][i] = (*head)->data->table[j][old_pos];
			}
			FreeInterData((*head)->data, (*head)->num_of_relations);
			(*head)->data = temp_array;
			return 1;
		}
		// If rel2 is the one active in the intermediate results.
		else if ((*head)->data->table[rel2] != NULL && (*head)->data->table[rel1] == NULL)
		{
			printf("\n\nInterRes: Rel2 is the only one active!\n\n");
			//Allocate and initialise the new inter_data variable.
			(*head)->data->num_tuples = GetResultNum(res);
			inter_data *temp_array = NULL;
			InitInterData(&temp_array, (*head)->num_of_relations, (*head)->data->num_tuples);
			for (size_t i = 0; i < (*head)->num_of_relations; i++)
				if((*head)->data->table[i] != NULL)
					temp_array->table[i] = malloc(((*head)->data->num_tuples) * sizeof(uint64_t));
			// [rel2] is still inactive , so we have to manually alocate it
			temp_array->table[rel1] = malloc((*head)->data->num_tuples * sizeof(uint64_t));

			//Insert the results.
			result_tuples *temp;
			int old_pos;
			for (size_t i = 0; i < (*head)->data->num_tuples; i++)
			{
				//Insert the results that are stored in the res variable.
				temp = FindResultTuples(res, i);

				/* Old_pos refers to the current result's row_id in the inter_res data table.*/
				old_pos = temp->tuple_S.row_id;
				temp_array->table[rel1][i] = temp->tuple_R.row_id;
				for (size_t j = 0; j < (*head)->num_of_relations; j++)
					if ((*head)->data->table[j] != NULL)
						temp_array->table[j][i] = (*head)->data->table[j][old_pos];
			}
			FreeInterData((*head)->data, (*head)->num_of_relations);
			(*head)->data = temp_array;
			return 1;
		}
		// If both relations are active
		else if((*head)->data->table[rel1] != NULL && (*head)->data->table[rel2] != NULL)
		{
			printf("Both relations are active\n");
			exit(2);
		}
		if((*head)->next == NULL)break;
		(*head) = (*head)->next;
	}
	//Allocate a new inter_res node
	InitInterResults(&(*head)->next, (*head)->num_of_relations);
	//Insert the results to the new node
	InsertJoinToInterResults(&(*head)->next, rel1, rel2, res);
	return 0;
}

void PrintInterResults(inter_res *head)
{
	int i = 0;
	while(head != NULL)
	{
		printf("Intermediate results node[%d]: \n", i);
		for (size_t i = 0; i < head->data->num_tuples; i++) {
			printf("Tuple: %5lu|||||", i);
			for (size_t j = 0; j < head->num_of_relations; j++) {
				if (head->data->table[j] != NULL)
					printf(" %5lu |", head->data->table[j][i]);
				else printf(" NULL |");
			}
			printf("\n");
		}
		printf("--------------------------------------------------\n" );
		head = head->next;
		i++;
	}
}

void FreeInterResults(inter_res* var)
{
	if (var->next != NULL) FreeInterResults(var->next);
	if (var->data->num_tuples > 0)
		for (int i = 0; i < var->num_of_relations; ++i)
			if(var->data->table[i] != NULL)
				free(var->data->table[i]);
	free(var->data->table);
	free(var->data);
	free(var);
}

relation* ScanInterResults(int given_rel,int column, inter_res* inter, relation_map* map,int* query_relations)
{
	if (inter->num_of_relations <= given_rel || given_rel < 0 )
	{
		fprintf(stderr, "given_rel out of bounds\n");
		exit(2);
	}

	int found_flag = 1;
	while(found_flag == 1 && inter!=NULL)
	{
		if (inter->data->table[given_rel] != NULL) found_flag = 0;
		else inter = inter->next;
	}
	if (found_flag == 1) return NULL;

	// Allocate a new struct relation
	relation *new_rel = malloc(sizeof(relation));
	new_rel->num_tuples = inter->data->num_tuples;
	new_rel->tuples = malloc(new_rel->num_tuples * sizeof(tuple));

	//Get a pointer to the correct column of the mapped relation
	uint64_t* col = map[query_relations[given_rel]].columns[column];
	int i;
	for (i = 0; i < inter->data->num_tuples; ++i)
	{
		new_rel->tuples[i].row_id = i;
		new_rel->tuples[i].value = col[inter->data->table[given_rel][i]];
		printf("\t\tNum_tuples: %lu | Row_id: %d Value: %ld \n", inter->data->num_tuples, i ,  col[ inter->data->table[given_rel][i] ]);
	}
	return new_rel;
}

relation* GetRelation(int given_rel, int column, inter_res* inter, relation_map* map,int* query_relations)
{
	relation *new_rel = NULL;
	int i;

	// If the relation is in the intermediate results
	if ( (new_rel = ScanInterResults(given_rel, column, inter, map, query_relations)) != NULL)
		return new_rel;
	// If the relation is only in the map
	else
	{
		relation* new_rel = malloc(sizeof(relation));
		new_rel->num_tuples = map[query_relations[given_rel]].num_tuples;
		new_rel->tuples = malloc(new_rel->num_tuples * sizeof(tuple));

		uint64_t *col = map[query_relations[given_rel]].columns[column];
		for (i = 0; i < new_rel->num_tuples; ++i)
		{
			new_rel->tuples[i].row_id = i;
			new_rel->tuples[i].value = col[i];
		}
		return new_rel;
	}

}


int SelfJoin(int given_rel, int column1, int column2, inter_res** inter, relation_map* map, int* query_relations)
{
	result *res = NULL;
	uint64_t i;
	uint64_t* col1 = map[query_relations[given_rel]].columns[column1];
	uint64_t* col2 = map[query_relations[given_rel]].columns[column2];

	int found_flag = 1;
	inter_res* temp = (*inter);
	printf("First instance of temp: %p\n", temp);
	while(found_flag == 1 && temp !=NULL)
	{
		printf("Instance of temp: %p\n", temp);
		if (temp->data->table[given_rel] != NULL)
			found_flag = 0;
		else temp = temp->next;
	}
	printf("Final instance of temp: %p\n", temp);
	//the relation is not in the intermediate result
	if (found_flag == 1)
	{
		for (i = 0; i < map[given_rel].num_tuples; ++i)
			if( col1[i] == col2[i]) InsertRowIdResult(&res, &i);
	}
	else
	{
		for (i = 0; i < temp->data->num_tuples; ++i)
			if ( col1[temp->data->table[given_rel][i]] == col2[temp->data->table[given_rel][i]])
				InsertRowIdResult(&res, temp->data->table[i]);
	}
	//Insert the results to inter_res
	InsertSingleRowIdsToInterResult(inter, given_rel, res);
	FreeResult(res);
	return 1;
}

void MergeInterNodes(inter_res **inter)
{
	if((*inter)->next == NULL)return;
	// for each relation
	for (size_t i = 0; i < (*inter)->num_of_relations; i++)
	{
		inter_res *temp = (*inter);
		// Check if it's active in multiple nodes
		if( (*inter)->data->table[i] == NULL )continue;
		while(temp->next != NULL)
		{
			//If relation i is active on both inter_res nodes, merge them
			if (temp->next->data->table[i] != NULL)
				Merge(inter, &temp, i);
			temp = temp->next;
			if(temp == NULL)break;
		}
	}
	if( (*inter)->next != NULL )MergeInterNodes(&(*inter)->next);
	printf("Exiting MergeInterNodes\n");
}


void Merge(inter_res **head, inter_res **node, int rel_num)
{
	/* Node refers to the previous node of the one we want to use. That way
	 * we can easily free the node we just merged while updating the inter_res list.
	 */
	//First allocate space in head for each active relation in node.
	for (size_t i = 0; i < (*head)->num_of_relations; i++)
		if ((*node)->next->data->table[i] != NULL && (*head)->data->table[i] == NULL)
			(*head)->data->table[i] = malloc( (*head)->data->num_tuples * sizeof(uint64_t) );

	/* (*head)->data->table[rel_num] values are row_ids pointing to tuples of
	 * (*node)->data->table . So for each tuple in (*head) insert all relations
	 * from (*node) that are active.
	 */
	for (size_t i = 0; i < (*head)->data->num_tuples; i++)
	{
		//Position is the (*node)'s row_id corresponding to the i-th element in (*head)
		uint64_t position = (*head)->data->table[rel_num][i];
		for (size_t j = 0; j < (*head)->num_of_relations; j++)
		{
				if((*node)->next->data->table[j] == NULL) continue;
				(*head)->data->table[j][i] = (*node)->next->data->table[j][position];
		}
	}
	inter_res *temp = (*node)->next;
	(*node)->next = (*node)->next->next;
	FreeInterData(temp->data, (*node)->num_of_relations);
	free(temp);
}

void CalculateQueryResults(inter_res *inter, relation_map *map, batch_listnode *query)
{
	//For every different sum
	for (size_t i = 0; i < query->views->num_of_elements; i++)
	{
		//printf("View[%lu] = %s\n", i, views->data[i]);
		int index = query->views->data[i][0] - '0';// Index to the query->relations
		int relation = query->relations[index]; // rel number
		int column = query->views->data[i][2] - '0';//column number
		//printf("View[%lu]-> Relation: %d Column: %d\n", i, relation, column);
		int temp_sum = 0;
		/* Intermediate result should be only one node at this point! */
		for (size_t j = 0; j < inter->data->num_tuples; j++)
		{
			printf("index %d\n", index );
			printf("Rel[%d] size is: %lu\n", relation, map[relation].num_tuples);
			printf("Accessing: rel_map[Rel:%d][Col:%d][Tup: %lu]\n", relation, column, (inter->data->table[index][j]));
			//printf ("value %ld\n",map[relation].columns[column][ (inter->data->table[index][j]) ]);
			temp_sum += map[relation].columns[column][ (inter->data->table[index][j]) ];
		}
		if (temp_sum == 0)printf("NULL ");
		else printf(" temp sum %d ", temp_sum);
	}
	printf("\n");
}
