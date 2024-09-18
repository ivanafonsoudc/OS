#include "list.h"


void createEmptyList(tList *L){
    *L=LNULL;
}
bool createNode(tPosL *p){
    *p= malloc(sizeof(**p));
    return (*p!=LNULL);
}

bool insertItem(tItemL d,tPosL p, tList *L){
    tPosL q,aux;
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
tPosL first(tList L){
    return L;
}

tPosL last(tList L){
    tPosL i;
    for(i=L;i->next!=LNULL;i=i->next);
    return i;
}

tPosL next(tPosL p, tList L){
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

