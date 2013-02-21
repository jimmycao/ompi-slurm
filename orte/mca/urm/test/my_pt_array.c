/*
 * my_pt_array.c
 *
 *  Created on: Feb 5, 2013
 *      Author: caoj7
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "opal_config.h"
#include "opal/class/opal_pointer_array.h"

typedef struct {
	int age;
	char *name;
}person_t;

#define NUM 	5
#define SIZE 	128

static void print_meta(opal_pointer_array_t *array);
static void print_element(opal_pointer_array_t *array);
static void free_array(opal_pointer_array_t *array);

int main()
{
	opal_pointer_array_t *array;
	person_t *tmp_person;
	char tmp[SIZE];
	int i, index, rc;

	array=OBJ_NEW(opal_pointer_array_t);
	assert(array);

	print_meta(array);

	/* add element into array [opal_pointer_array_add]*/
	for(i = 0; i < NUM; i++){
		tmp_person = (person_t *)malloc(sizeof(person_t));
		tmp_person->age = i;
		sprintf(tmp, "name_%d", i);
		tmp_person->name = strdup(tmp);
		opal_pointer_array_add(array, tmp_person);
	}

	print_meta(array);
	print_element(array);

	/* reset the index_th element [opal_pointer_array_set_item]*/
	tmp_person = (person_t *)malloc(sizeof(person_t));
	tmp_person->age = 99;
	sprintf(tmp, "name_%d", 99);
	tmp_person->name = strdup(tmp);
	index = 0;
	rc = opal_pointer_array_set_item(array, index, tmp_person);
	/* printf the index_th element after resetting */
	tmp_person = (person_t *)array->addr[index];
	printf("%d: age = %d, name = %s\n", index, tmp_person->age, tmp_person->name);

	print_meta(array);
	print_element(array);

	puts("======================");
	/*get the i-th element and print */
	/*  */
	for(i = 0; i < array->size; i++){
		if(NULL == opal_pointer_array_get_item(array,i))
			continue;
		tmp_person = (person_t *)opal_pointer_array_get_item(array,i);
		printf("%d: age = %d, name = %s\n", i, tmp_person->age, tmp_person->name);
	}

	/*free all*/
	free_array(array);

	return 0;
}

static void free_array(opal_pointer_array_t *array)
{
	void *tmp_object;
	int i;

	if(NULL == array)
		return;

	for(i = 0; i < array->size; i++) {
		tmp_object = opal_pointer_array_get_item(array,i);
		if(NULL != tmp_object)
			free(tmp_object);
	}

	free(array);
}


static void print_meta(opal_pointer_array_t *array)
{
	printf("array->size = %d\n", array->size);
	printf("array->number_free = %d\n", array->number_free);
	printf("array->lowest_free = %d\n", array->lowest_free);
	printf("array->block_size = %d\n", array->block_size);
	printf("array->max_size = %d\n", array->max_size);
}

static void print_element(opal_pointer_array_t *array)
{
	person_t *tmp_person;
	int i;

	for(i = 0; i < array->size; i++){
		if(NULL == array->addr[i])
			continue;
		tmp_person = (person_t *)array->addr[i];
		printf("%d: age = %d, name = %s\n", i, tmp_person->age, tmp_person->name);
	}
}
