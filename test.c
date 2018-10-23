#include "common.h"

char patha[MAX_DIRECTORY_SIZE], pathb[MAX_DIRECTORY_SIZE];

#define test_path_join(A, B)     strcpy(patha, A); \
    strcpy(pathb, B); \
    join_path(patha, pathb); \
    printf(patha); \
    printf("\r\n"); \

int main()
{
    // strcpy(patha, "/a/a");
    // strcpy(pathb, "a/d");
    // join_path(patha, pathb);
    // printf(patha);
    // printf("\r\n"); 
    test_path_join("/a/a", "b/g");
    test_path_join("/a/a/", "b/g");
    test_path_join("/a/a", "b/g/");
    test_path_join("/a/a/", "b/g/");
}