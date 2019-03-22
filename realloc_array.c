#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
	double *data;
	size_t size;
} double_list_t;

typedef struct {
	char **strings;
	size_t size;
} string_list_t;

void string_list_init(string_list_t *string_list) {
	string_list->size = 0;
	string_list->strings = (char**)malloc(sizeof(char*));
}

void string_list_add(string_list_t *string_list, const char *string) {
	string_list->size += 1;
	string_list->strings = (char**)realloc(string_list->strings, string_list->size * sizeof(char*));
	string_list->strings[string_list->size - 1] = string;
}

int main(void) {
	double_list_t double_list;
	double_list.size = 10;
	double_list.data = (double*)malloc(double_list.size * sizeof(double));
	char *some_string = (char*)calloc(64, sizeof(char));
	strncpy(some_string, "hello", 5);

	for (register size_t i = 0; i < 80; ++i) {
		if (i >= double_list.size) {
			double_list.size *= 2;
			double_list.data = (double*)realloc(double_list.data, double_list.size * sizeof(double));
		}

		double_list.data[i] = sqrt((double)i);
	}

	for (register size_t i = 0; i < 80; ++i) {
		printf("data = %f\n", double_list.data[i]);
	}

	printf("some_string = %s\n", some_string);

	free(double_list.data);
	free(some_string);


	char *strings_to_add[] = {
		"FOX",
		"CBS",
		"ABC",
		"FOX NEWS",
		"CNN",
		"MSNBC",
		"BBC",
		"CNBC",
		"NBC",
		"RUSSIA TODAY",
		"UNIVISION",
		"TELEMUNDO",
		"PBS",
		"ESPN",
		"FOX BUSINESS"
	};

	string_list_t string_list;
	string_list_init(&string_list);

	for (register size_t i = 0; i < 15; ++i) {
		string_list_add(&string_list, strings_to_add[i]);
	}

	for (register size_t i = 0; i < 15; ++i) {
		printf("string = %s\n", string_list.strings[i]);
	}

	free(string_list.strings);

	return EXIT_SUCCESS;
}
