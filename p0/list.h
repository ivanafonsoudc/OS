#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define LNULL NULL

typedef char *tItemL;

typedef struct tNode *tPos;

struct tNode{
    tItemL data;
    tPos next;
};

typedef tPos tListP;

bool isEmptyList(tListP L);
void createEmptyList(tListP *L);
bool createNode(tPos *p);
bool insertItem(tItemL d, tListP *L);
tItemL getItemP(tPos p, tListP L);
tPos first(tListP L);
tPos last(tListP L);
tPos next(tPos p, tListP L);
void RemoveElement(tPos p, tListP *L);
void deleteList(tListP *L);

#endif