#include "list.h"


bool isEmptyList(tListP L){
    return L==LNULL;
}

void createEmptyList(tListP *L){
    *L=LNULL;
}

bool createNode(tPos *p){
    *p = malloc(sizeof(struct tNode));
    return (*p) !=LNULL;
}

bool insertItem(tItemL d, tListP *L){
    tPos q, r;

    if(!createNode(&q)){
        return false;
    }
    q->data = malloc((strlen(d) + 1) * sizeof (char));
    if(!q->data){
        free(q);
        return false;
    }
    
    strcpy(q->data, d);
    q->next = LNULL;

    if(*L == LNULL){
        *L = q;
    }else{
        r = last(*L);
        r->next = q;
    }
    return true;
    
}


tItemL getItemP(tPos p, tListP L){
    return p->data;
}
tPos first(tListP L){
    return L;
}

tPos last(tListP L){
    if(isEmptyList(L)){
        return LNULL;
    }
    tPos i;
    for(i = L; i->next != LNULL; i = i->next);
    return i;
}

tPos next(tPos p, tListP L){
    return p->next;
}
void RemoveElement(tPos p, tListP *L){
    *L=p->next;
    free(p->data);
    free(p);

}
void deleteList(tListP *L){
    while(!isEmptyList(*L)){
        RemoveElement(first(*L),L);
    }
}

