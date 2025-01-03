/*
  Practica 0 SO
  Grupo 1.2
  Iván Afonso Cerdeira ivan.afonso@udc.es
  Minerva Antía Lago López minerva.lago.lopez@udc.es
 */



#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <sys/utsname.h>
#include <stdio.h>
#include <errno.h>
#include "list.h"
#define MAX 1024
#define MAXTR 100
#define MAXH 100

int TrocearCadena(char *cadena, char *tr[]);
void procesarEntrada(char *cmd, bool *terminado, char *tr[], tListP *openFilesList);

int history_count = 0;




typedef struct{
    int fd;
    char name[MAX];
    int mode;
}File;

File openFiles[MAX];

int openFilesCount = 0;



typedef struct ComandNode {
    char name[MAX];            
    struct ComandNode *next;      
} ComandNode;

ComandNode *historyList = NULL;



void addCommand(char *name) {
    ComandNode *newNode = (ComandNode *)malloc(sizeof(ComandNode)); 
    if(newNode == NULL){
        fprintf(stderr, "Error"); 
        return;
    }   
    strncpy(newNode->name, name, MAX); 
    newNode->next = historyList; 
    historyList = newNode; 
    
}

int getTotalHistCount() {
    ComandNode *current = historyList; 
    int count = 0;
    while(current != NULL){
        count++;
        current = current->next;
    }
    return count;
}


void printHistory(int i){
    ComandNode *current = historyList;
    int count = 0;
    if(i == -1){
        while(current != NULL){
        printf("%d %s\n",count, current->name);
        current = current->next;
        count++;
    }
  }else{
    while(current != NULL && count < i){
        current = current->next;
        count++;
    }
    if(current != NULL){
        printf("%d %s\n",i, current->name);
    }else{
        fprintf(stderr,"Error");
    }
  }
}

void printLastNCommands(int n) {
    int totalCommands = getTotalHistCount();  
    if (n > totalCommands) {
        n = totalCommands;  
    }
    ComandNode *current = historyList;
    int start = totalCommands - n;
    for (int i = 0; i < start; i++) {
        current = current->next;
    }

    for (int i = start; i < totalCommands; i++) {
        printf("%d %s\n", i, current->name);  
        current = current->next;   
    }
}



void clearHistory(){
    ComandNode *current = historyList;
    ComandNode *next;
    while(current != NULL){
        next = current->next;
        free(current);
        current = next;
    }
    historyList = NULL;
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

char* Mode(int mode){
    static char result[64];
    result[0] = '\0';
    
    if (mode & O_CREAT) strcat(result, "cr ");
    if (mode & O_EXCL) strcat(result, "ex ");
    if (mode & O_RDONLY) strcat(result, "ro ");
    if (mode & O_WRONLY) strcat(result, "wo ");
    if (mode & O_RDWR) strcat(result, "rw ");
    if (mode & O_APPEND) strcat(result, "ap ");
    if (mode & O_TRUNC) strcat(result, "tr ");

    if (result[0] == '\0'){ 
        strcpy(result, "unkown mode");
    }else{
        result[strlen(result)-1] = '\0';
    }
    return result;
}

void AnadirFicherosAbiertos(int fd, const char *name, int mode){
    if(openFilesCount<MAX){
        openFiles[openFilesCount].fd = fd;
        strncpy(openFiles[openFilesCount].name, name,MAX);
        openFiles[openFilesCount].mode = mode;
        openFilesCount++;
    }else{
        fprintf(stderr,"Error");
    }
    
}

void EliminarDeFicherosAbiertos(int fd){
    for (int i = 0; i<openFilesCount;i++){
        if (openFiles[i].fd == fd){
            printf("File descriptor %d closed\n", openFiles[i].fd);
            for (int j = i; j<openFilesCount-1;j++){
                openFiles[j] = openFiles[j+1];
            }
            openFilesCount--;
            
            break;
        }
    }
}

void ListFicherosAbiertos(int fd, tListP *L){
    for(int i = 0; i<openFilesCount;i++){
        printf("Descriptor %d, Name: %s, Mode: %s\n", openFiles[i].fd,openFiles[i].name, Mode(openFiles[i].mode) );
    }
}




char *NombreFicheroDescriptor(int fd){
    for(int i=0; i<openFilesCount;i++){
        if(openFiles[i].fd == fd){
            return openFiles[i].name;
        }
    }
    return NULL;
}



void Cmd_authors(char *tr[], char *cmd){
    if (tr[1] == NULL){
        printf("Ivan Afonso Cerdeira: ivan.afonso@udc.es, Minerva Antia Lago Lopez: minerva.lago.lopez@udc.es\n");
    }else if (strcmp(tr[1], "-l") == 0){
        printf("ivan.afonso@udc.es, minerva.lago.lopez\n");
    }else if(strcmp(tr[1], "-n") == 0){
        printf("Ivan Afonso Cerdeira, Minerva Antia \n");
    }else{
        fprintf(stderr,"%s \n",cmd);
    }
}

void Cmd_pid(char *tr[], char *cmd){
    if (tr[1] == NULL){
        printf("Process ID: %d\n", getpid());
    }else{
        fprintf(stderr,"%s \n",cmd);
    }
}

void Cmd_ppid(char *tr[], char *cmd){
    if (tr[1] == NULL){
        printf("Parent Process ID: %d\n", getppid());
    }else{
        fprintf(stderr,"%s \n",cmd);
    }
}

void Cmd_cd(char *tr[], char *cmd){
    char path[MAX];
    if(tr[1] == NULL){
        if(getcwd(path,MAX)==NULL){
            perror("getcwd");
        }else {
            printf("Directorio actual %s\n", path);
        }
    }else if(tr[2] == NULL){
        if (chdir(tr[1])== -1){
            printf("No se ha podido cambiar de directorio\n");
        }else{
            chdir(tr[1]);
            printf("Directorio cambiado a %s\n", tr[1]);
        }
    }
    else{
        fprintf(stderr,"%s \n",cmd);

   }
}

void Cmd_date(char *tr[], char *cmd){                                  
   if(tr[1] == NULL){
        time_t t;
        struct tm *tm;
        char fechayhora[100];
        t = time(NULL);
        tm = localtime(&t);
        strftime(fechayhora, 100, "%d/%m/%Y %H:%M:%S", tm);
        printf("Fecha y hora: %s\n", fechayhora);
    }else if(strcmp(tr[1],"-d") == 0){
        time_t t;
        struct tm *tm;
        char fechayhora[100];
        t = time(NULL);
        tm = localtime(&t);
        strftime(fechayhora, 100, "%d/%m/%Y", tm);
        printf("Fecha: %s\n", fechayhora);
    }else if(strcmp(tr[1],"-t") == 0){
        time_t t;
        struct tm *tm;
        char fechayhora[100];
        t = time(NULL);
        tm = localtime(&t);
        strftime(fechayhora, 100, "%H:%M:%S", tm);
        printf("Hora: %s\n", fechayhora);
    }else{
        fprintf(stderr,"%s \n",cmd);
    }
}


void Cmd_hist(char *tr[], char *cmd) {
    
    if (tr[1] == NULL) {  // Sin argumentos, imprime todo el historial
         printHistory(-1);
    }else if (tr[2] == NULL) {  // Un argumento, imprime el comando en esa posición
        int n = atoi(tr[1]);
        if(n>=0){
            printHistory(n);
        }else{
            printLastNCommands(-n); 
        }
    }else {
        fprintf(stderr,"%s \n",cmd);
    }    
}  





void Cmd_open (char * tr[], tListP *openFilesList){                                          
    int i,df, mode=0;      
        
    if (tr[1]==NULL) { /*no hay parametro*/                      
            ListFicherosAbiertos(0, openFilesList); /*listar ficheros abiertos*/
        return;                                                  
    }          
    for (i=2; tr[i]!=NULL; i++)
      if (!strcmp(tr[i],"cr")) mode |= O_CREAT;
      else if (!strcmp(tr[i],"ex")) mode |= O_EXCL;                
      else if (!strcmp(tr[i],"ro")) mode |= O_RDONLY; 
      else if (!strcmp(tr[i],"wo")) mode |= O_WRONLY;
      else if (!strcmp(tr[i],"rw")) mode |= O_RDWR;                
      else if (!strcmp(tr[i],"ap")) mode |= O_APPEND;
      else if (!strcmp(tr[i],"tr")) mode |= O_TRUNC; 
      else break;
                     
    if ((df=open(tr[1],mode,0777))==-1)
        perror ("Imposible abrir fichero");
    else{                
        AnadirFicherosAbiertos(df,tr[1],mode);                                                        
        printf ("Anadida entrada a la tabla de ficheros abiertos.\nDescriptor: %d, Name: %s, Mode: %s\n",df, tr[1], Mode(mode));                                               
    }
}     


void Cmd_close (char *tr[], tListP *openFilesList){
    int df;

    if (tr[1]==NULL || (df=atoi(tr[1]))<0) { /*no hay parametro*/
        ListFicherosAbiertos(0,openFilesList); /*o el descriptor es menor que 0*/
        return;
    }

    if (close(df)==-1) {
        perror("Imposible cerrar descriptor");
    }else{
        EliminarDeFicherosAbiertos(df);
    }
}


void Cmd_dup (char * tr[], tListP *L){
    int df, duplicado, ormode;
    char aux[MAX],*orname;;

    if (tr[1]==NULL || (df=atoi(tr[1]))<0) { /*no hay parametro*/
        ListFicherosAbiertos(-1, L);  
        return;               /*o el descriptor es menor que 0*/
    }

    duplicado=dup(df);
    if(duplicado==-1){
        perror("Imposible duplicar descriptor");
        return;
    }

    orname=NombreFicheroDescriptor(df);
    ormode=fcntl(df,F_GETFL);
    if(ormode==-1){
        perror("Imposible obtener modo de apertura");
        return;
    }

    snprintf(aux, MAX, "%s (duplicated)", orname);
    printf("Descriptor: %d, Name: %s, Mode:%s\n", df,orname,Mode(ormode));
    AnadirFicherosAbiertos(duplicado,aux,ormode);


}

void Cmd_infosys(char *tr[], char *cmd){
    struct utsname uts;
    if (tr[1] == NULL){
        uname(&uts);
        printf("Systemname: %s\n", uts.sysname);
        printf("Nodename: %s\n", uts.nodename);
        printf("Release: %s\n", uts.release);
        printf("Version: %s\n", uts.version);
        printf("Machine: %s\n", uts.machine);
    }else{
        fprintf(stderr,"%s \n",cmd);
    }
}



void Cmd_help(char *tr[], char *cmd){
  if(tr[1] == NULL){
    printf("Comandos disponibles: authors, pid, ppid, cd, date, historic, open, close, dup, infosys, help, quit,exit,bye\n");
  }else if(strcmp(tr[1],"authors") == 0){
      printf("Prints the names and logins of the program authors. authors -l prints only the logins and authors -n prints only the names\n");
  }else if(strcmp(tr[1], "pid") == 0){
      printf("Prints the pid of the process executing the shell\n");
  }else if(strcmp(tr[1], "ppid") == 0){
      printf("Prints the pid of the shell’s parent process\n");
  }else if(strcmp(tr[1], "cd") == 0){
      printf("Changes the current working directory of the shell to dir (using the chdir system call). When invoked without auguments it prints the current working directory (using the getcwd system call)\n");
  }else if(strcmp(tr[1], "date") == 0){
      printf("Prints the current date in the format DD/MM/YYYY and the current time in the format hh:mm:ss.\n date -d Prints the current date in the format DD/MM/YYYY\n date -t Prints and the current time in the format hh:mm:ss.\n");
  }else if(strcmp(tr[1], "historic") == 0){
      printf("Shows the historic of commands executed by this shell. In order to do this, a list to store all the commands input to the shell must be implemented.\n – historic Prints all the comands that have been input with their order number\n – historic N Repeats command number N (from historic list)\n – historic -N Prints only the lastN comands\n");
  }else if(strcmp(tr[1], "open") == 0){
      printf("Opens a file and adds it (together with the file descriptor and the opening mode to the list of shell open files.\n For the mode we’ll use cr for O_CREAT, ap for O_APPEND, ex for O_EXCL, ro for O_RDONLY, rw for O_RDWR, wo for O_WRONLY and tr for O_TRUNC\n");
  }else if(strcmp(tr[1], "close") == 0){
      printf("Closes the df file descriptor and eliminates the corresponding item from the list\n");
  }else if(strcmp(tr[1], "dup") == 0){
      printf("Duplicates of the df file descriptor\n");
  }else if(strcmp(tr[1], "infosys") == 0){
      printf("Prints information on the machine running the shell\n");
  }else if(strcmp(tr[1], "help") == 0){
      printf("help displays a list of available commands. help cmd gives a brief help on the usage of comand cmd\n");
  }else if(strcmp(tr[1], "quit") == 0){
      printf("Ends the shell\n");
  }else if(strcmp(tr[1], "exit") == 0){
      printf("Ends the shell\n");
  }else if(strcmp(tr[1], "bye") == 0){
      printf("Ends the shell\n");
  }else{
      fprintf(stderr,"%s \n",cmd);
      }

}


void Cmd_quit(bool *terminado, char *tr[], tListP *openFilesList){
    if(tr[1] == NULL){
        *terminado = true;
        clearHistory();
        if(openFilesList!=NULL){
            deleteList(openFilesList);
        }    
    }else{
        printf("Error: Invalid option\n");
    }
}

void imprimirPrompt(){
    printf(":~>");
}

void leerEntrada(char *cmd, char *tr[], char *entrada){
    fgets(entrada,MAX,stdin);
    strcpy(cmd,entrada);
    strtok(cmd,"\n");
    TrocearCadena(entrada,tr);


}

void guardarLista(char *entrada,char *tr[]){
    if(tr[0]!=NULL){
        addCommand(entrada);
    }
}

void procesarEntrada(char *cmd, bool *terminado, char *tr[], tListP *openFilesList){
   if(tr[0] != NULL){
     if(strcmp(tr[0], "authors") == 0){
       Cmd_authors(tr,cmd);
     }else if(strcmp(tr[0], "pid") == 0){
         Cmd_pid(tr, cmd);
     }else if(strcmp(tr[0], "ppid") == 0){
         Cmd_ppid(tr, cmd); 
     }else if(strcmp(tr[0], "cd") == 0){
         Cmd_cd(tr,cmd);
     }else if(strcmp(tr[0], "date") == 0){
         Cmd_date(tr,cmd);
     }else if(strcmp(tr[0], "historic") == 0){
         Cmd_hist(tr,cmd);
     }else if(strcmp(tr[0], "open") == 0){
         Cmd_open(tr, openFilesList);
     }else if(strcmp(tr[0], "close") == 0){
         Cmd_close(tr, openFilesList);
     }else if(strcmp(tr[0], "dup") == 0){
            Cmd_dup(tr, openFilesList);
     }else if(strcmp(tr[0], "infosys") == 0){
            Cmd_infosys(tr,cmd);
     }else if(strcmp(tr[0], "help") == 0){
            Cmd_help(tr,cmd);
     }else if(strcmp(tr[0], "quit") == 0 || strcmp(tr[0], "exit") == 0 || strcmp(tr[0], "bye") == 0){  
             Cmd_quit(terminado,tr, openFilesList);                                   
     }else{ 
        fprintf(stderr,"%s \n",cmd); 
     }
   }
}


int main(){
    char entrada[MAX];
    char cmd[MAX];
    char *tr[MAXTR];
    bool terminado = false;
    tListP openFilesList;

    
    createEmptyList(&openFilesList);
    while (!terminado){
        imprimirPrompt();
        leerEntrada(cmd,tr,entrada);
        guardarLista(entrada, tr);
        procesarEntrada(cmd,&terminado, tr, &openFilesList);
        }
        return 0;
        
}
