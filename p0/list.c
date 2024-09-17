#include list.h
#include <stdio.h>

void createEmptyList(tList *L){
    *L=LNULL;
}

bool createNode(tPos *p){
  *p = malloc(sizeof(struct tNode));
  return *p != LNULL;
}

bool insertElement(tItemL d, tListP *L){
    tPosU q, p;
    if (!createNode(&q)) {
        return false;
    }
  q->data = d;
  q->next = LNULL;

  if (*L == LNULL) {
    *L = q;
  } else {
    p = *L;
    while (p->next != LNULL) {
      p = p->next;
    }
    p->next = q;
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
  tPos p;
  for(p=L; p->next != LNULL;p=p->next){}
  return p;
}

tPos nextP(tPos p, tListP L){
  return p->next;
}

tPos prev(tPos p, tListP L){
  tPos q;
  if(p==L){
    q = LNULL;
  }else{
    for (q = L; q->next != p; q = q->next);
  }
  return q;
}

void RemoveElement(tPos p, tListP *L){
  if(p==*L){
    *L=(*L)->next;
  }
  if(*L!=LNULL){
    (*L)->prev=LNULL;
  }
  else if(p->next!=LNULL){
    p->prev->next=p->next;
    p->next->prev=p->prev;
  }
  else{
    p->prev->next=LNULL;
  }
  free(p);
}

void deleteList(tListP *L){
  tPos p;
  while(*L!=LNULL){
    p=*L;
    *L=(*L)->next;
    free(p);
  }
}