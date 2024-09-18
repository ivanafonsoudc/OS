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

bool insertItem(tItemL d,tPos p, tListP *L){
    tPos q,aux;
    if(!createNode(&q)){
        return false;
    }
    else{
        q->data=d;
        q->next=LNULL;
        if(isEmptyList(*L)){
            *L=q;
        }
        else if(p==LNULL){
            for(aux= first(*L);aux->next!=LNULL;aux=aux->next);
            aux->next=q;
        }
        else if(p==*L){
            q->next=p;
            *L=q;
        }
        else{
            q->data=p->data;
            p->data=d;
            q->next=p->next;
            p->next=q;
        }
        return true;
    }

}


tItemL getItemP(tPos p, tListP L){
    return p->data;
}
tPos first(tListP L){
    return L;
}

tPos last(tListP L){
    tPos i;
    for(i=L;i->next!=LNULL;i=i->next);
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
    while(first!=LNULL){
        RemoveElement(first(*L),L);
    }
}

