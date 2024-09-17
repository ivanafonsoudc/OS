#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "list.h"
#define MAX 1024




int TrocearCadena(char * cadena, char * tr[]);
void procesarEntrada(char * tr[]);


typedef struct{
    int fd;
    char name[MAX];
    int mode;
}File;

File openFiles[MAX];
int openFilesCount = 0;

void ListFicherosAbiertos(){
    printf("Descriptor\tNombre\tModo\n");

    for(int i = 0; i < openFilesCount; i++){
        printf("%d\t%s\t%d\n", openFiles[i].fd, openFiles[i].name, openFiles[i].mode);
    }
}

int TrocearCadena(char * cadena, char * tr[]){ 
    int i=1;

    if ((tr[0]=strtok(cadena," \n\t"))==NULL){
        return 0;
    }
    while ((tr[i]=strtok(NULL," \n\t"))!=NULL){
        i++;
    }
    return i;
    
}

void AnadirFicherosAbiertos(int fd, const char *name, int mode){
    if (openFilesCount < MAX){
        openFiles[openFilesCount].fd = fd;
        strcpy(openFiles[openFilesCount].name, name);
        openFiles[openFilesCount].mode = mode;
        openFilesCount++;   
    }
    else{
        printf("Error: Maximum number of open files reached\n");
    
    }
}

void EliminarDeFicherosAbiertos(int fd){
    for (int i = 0; i<openFilesCount;i++){
        if (openFiles[i].fd == fd){
            for (int j = i; j<openFilesCount-1;j++){
                openFiles[j] = openFiles[j+1];
            }
            openFilesCount--;
            printf("File descriptor %d closed\n", fd);
            return;
        }
    }
}

char *NombreFicheroDescriptor(int fd){
    return openFiles[fd-1].nombre;
}

void Cmd_authors(char *tr[]){
    if (tr[1] == NULL){
        printf("Ivan Afonso Cerdeira: ivan.afonso@udc.es, Minerva Antia Lago Lopez: minerva.lago.lopez@udc.es\n");
    }else if (strcmp(tr[1], "-l") == 0){
        printf("ivan.afonso@udc.es, minerva.lago.lopez\n");
    }else if(strcmp(tr[1], "-n") == 0){
        printf("Ivan Afonso Cerdeira, Minerva Antia \n");
    }else{
        printf("Error: Invalid option\n");
    }
}

void Cmd_pid(char *tr[]){
    if (tr[1] == NULL){
        printf("Process ID: %d\n", getpid());
    }else{
        printf("Error: Invalid option\n");
    }
}

void Cmd_ppid(char *tr[]){
    if (tr[1] == NULL){
        printf("Parent Process ID: %d\n", getppid());
    }else{
        printf("Error: Invalid option\n");
    }
}

void Cmd_cd(char *tr[]){
    char path[MAX]
    if(tr[1] == NULL){
        if(getcwd(path,MAX)==NULL){
            perror("getcwd");
        }else {
            printf("Directorio actual %s\n", path);
        }
    }

    else if(tr[2] == NULL){
        if (chdir(tr[1])== -1){
            printf("No se ")
        }
    }

}

void Cmd_date(char *tr[]){}

void Cmd_hist(char *tr[]){

}




void Cmd_open (char * tr[]){                                          
    int i,df, mode=0;      
        
    if (tr[0]==NULL) { /*no hay parametro*/                      
            ListFicherosAbiertos;
        return;                                                  
    }          
    for (i=1; tr[i]!=NULL; i++)
      if (!strcmp(tr[i],"cr")) mode|=O_CREAT;
      else if (!strcmp(tr[i],"ex")) mode|=O_EXCL;                
      else if (!strcmp(tr[i],"ro")) mode|=O_RDONLY; 
      else if (!strcmp(tr[i],"wo")) mode|=O_WRONLY;
      else if (!strcmp(tr[i],"rw")) mode|=O_RDWR;                
      else if (!strcmp(tr[i],"ap")) mode|=O_APPEND;
      else if (!strcmp(tr[i],"tr")) mode|=O_TRUNC; 
      else break;
                          
    if ((df=open(tr[0],mode,0777))==-1)
        perror ("Imposible abrir fichero");
    else{                
        AnadirFicherosAbiertos(df, tr[0], mode);                                                        
        printf ("Anadida entrada a la tabla ficheros abiertos: Descriptor: %d, Name: %s, Mode: %d\n",df, tr[0], mode);                                               
    }
}     

void Cmd_close (char *tr[]){                                                      
    int df;                                         
                                                   
    if (tr[0]==NULL || (df=atoi(tr[0]))<0) { /*no hay parametro*/
        ListFicherosAbiertos;/*o el descriptor es menor que 0*/                              
        return;  
    }                     
                                           
    if (pclose(df)==-1) {  
        perror("Inposible cerrar descriptor");
    }else{                                                         
        EliminarDeFicherosAbiertos(df);
    }
}  

void Cmd_dup (char * tr[]) 
{                     
    int df, duplicado;                        
    char aux[MAXNAME],*p;
                                      
    if (tr[0]==NULL || (df=atoi(tr[0]))<0) { /*no hay parametro*/
        ListOpenFiles(-1);                 /*o el descriptor es menor que 0*/ 
    }

    duplicado=dup(df);
    p=NombreFicheroDescriptor(df);
      printf (aux,"dup %d (%s)",df, p);
       AnadirFicherosAbiertos(duplicado,aux,fcntl(duplicado,F_GETFL));
}

void imprimirPrompt(){
    printf(":~>");
}

void leerEntrada(){
    char entrada[256];
    fgets(entrada, 256, stdin);
    char *trozos[256];


}

void procesarEntrada(){
     
}



main{
   while (!terminado){
       imprimirPrompt();
       leerEntrada();
       procesarEntrada();
    }
    
}
