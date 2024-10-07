/*
  Practica 1 SO
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
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
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

char LetraTF (mode_t m){
     switch (m&S_IFMT) { /*and bit a bit con los bits de formato,0170000 */
        case S_IFSOCK: return 's'; /*socket */
        case S_IFLNK: return 'l'; /*symbolic link*/
        case S_IFREG: return '-'; /* fichero normal*/
        case S_IFBLK: return 'b'; /*block device*/
        case S_IFDIR: return 'd'; /*directorio */
        case S_IFCHR: return 'c'; /*char device*/
        case S_IFIFO: return 'p'; /*pipe*/
        default: return '?'; /*desconocido, no deberia aparecer*/
     }
}
/*las siguientes funciones devuelven los permisos de un fichero en formato rwx----*/
/*a partir del campo st_mode de la estructura stat */
/*las tres son correctas pero usan distintas estrategias de asignaciÃ³n de memoria*/



char * ConvierteModo2 (mode_t m)                      
{                                                     
    static char permisos[12];                     
    strcpy (permisos,"---------- ");                                 
                                                      
    permisos[0]=LetraTF(m);            
    if (m&S_IRUSR) permisos[1]='r';    /*propietario*/
    if (m&S_IWUSR) permisos[2]='w';                              
    if (m&S_IXUSR) permisos[3]='x';             
    if (m&S_IRGRP) permisos[4]='r';    /*grupo*/
    if (m&S_IWGRP) permisos[5]='w';                                                 
    if (m&S_IXGRP) permisos[6]='x';                  
    if (m&S_IROTH) permisos[7]='r';    /*resto*/                                    
    if (m&S_IWOTH) permisos[8]='w';
    if (m&S_IXOTH) permisos[9]='x';            
    if (m&S_ISUID) permisos[3]='s';    /*setuid, setgid y stickybit*/
    if (m&S_ISGID) permisos[6]='s';
    if (m&S_ISVTX) permisos[9]='t';
                                              
    return permisos;                                  
} 



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






void Cmd_makedir(char *tr[]) {
    // Intentar crear el directorio
    if(tr[1] != NULL){
        if (mkdir(tr[1], 0755) == -1) { //0755 da permisos de lectura, escritura y ejecucion para el propietario. El resto lectura y ejecución
            // Manejar errores
            switch (errno) {
                case EEXIST:
                    printf("Error: El directorio '%s' ya existe.\n", tr[1]);
                break;
                case ENOENT:
                    printf("Error: El directorio padre de '%s' no existe.\n", tr[1]);
                break;
                case EACCES:
                    printf("Error: No tienes permisos para crear el directorio '%s'.\n", tr[1]);
                break;
                case ENOSPC:
                    printf("Error: No hay suficiente espacio en el dispositivo para crear '%s'.\n", tr[1]);
                break;
                default:
                    printf("Error desconocido al crear el directorio '%s': %s\n", tr[1], strerror(errno));
                break;
            }
        } else {
            printf("Directorio '%s' creado correctamente.\n", tr[1]);
        }
    }
}


//makefile, crea un fichero con el nombre que se le pase
void Cmd_makefile(char *tr[], char *cmd){
    if(tr[1] != NULL && tr[2] == NULL){
        int fd = open(tr[1], O_CREAT | O_WRONLY, 0644);
        if(fd == -1){
            perror("open");
        }else{
            printf("File %s created with descriptor %d\n", tr[1], fd);
            AnadirFicherosAbiertos(fd, tr[1], O_CREAT | O_WRONLY);
        }
    }else{
        fprintf(stderr,"%s \n",cmd);
    }
}


void fileinfo(const char *path, const struct stat *file_stat, int longFormat){
    if(longFormat){
        printf("%lu\t", file_stat->st_ino);
        printf("%lu\t", file_stat->st_nlink);
        printf("%s\t", getpwuid(file_stat->st_uid)->pw_name);
        printf("%s\t", getgrgid(file_stat->st_gid)->gr_name);
        printf("%o\t", file_stat->st_mode & 0777);
        printf("%ld\t", file_stat->st_size);
        printf("%s\n", path);
    }else{
        printf("%ld\t", file_stat->st_size);
        printf("%s\n", path);
    }
}

//listfile, muestra: nombre y tamaño
// ls -l

//listfile -long, muestra: nº enlaces, inodo, user,group,permisos,tamaño,nombre
//ls -l -i 
void Cmd_listfile(char *tr[], char *cmd){
    DIR *dir = opendir(".");
    if(dir == NULL){
        perror("opendir");
        return;
    }

    struct dirent *entry;
    int long_format = (tr[1] != NULL && strcmp(tr[1], "-long") == 0);

    while ((entry = readdir(dir)) != NULL){
        if(strcmp(entry->d_name, ".") == 0 ){
            continue;
        }

        struct stat file_stat;
        if(stat(entry->d_name, &file_stat) == 0){
            fileinfo(entry->d_name, &file_stat, long_format);
        }else{
            perror("stat");
        }
    }
}

void Cmd_listdir(char *tr[], char *cmd){
    DIR *dir = opendir(".");
    if(dir == NULL){
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while((entry = readdir(dir)) != NULL){
        if((entry->d_name[0]) == '.'){
            continue;
            printf("%s\n", entry->d_name);
        }     
    }
    closedir(dir);
}

//cwd muestra el directorio actual y todo lo que hay en el

void Cmd_cwd(){
    char path[MAX];
    if(getcwd(path,MAX)==NULL){
        perror("getcwd");
    }else{
        printf("Directorio actual %s\n", path);
        Cmd_listdir(NULL, NULL);
    }
}




//lista directorios de forma recursiva
//rec list empieza de fuera hacia adentro
void Cmd_reclist(char *path){
    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while((entry = readdir(dir))!=NULL){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        struct stat path_stat;
        if(stat(full_path, &path_stat) == 0){
            if(S_ISDIR(path_stat.st_mode)){
                printf("[DIR] %s\n", full_path);
                Cmd_reclist(full_path);
            }else{
                printf("[FILE] %s\n", full_path);
            }
        }else{
            perror("stat");
        } 
    }
    closedir(dir);
}

//rev list muestra de dentro hacia afuera
void Cmd_revlist(char *path){
    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while((entry = readdir(dir))!=NULL){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            continue;
        }
        
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        struct stat path_stat;
        if(stat(full_path, &path_stat) == 0){
            if(S_ISDIR(path_stat.st_mode)){
                Cmd_revlist(full_path);
                printf("[DIR] %s\n", full_path);
            }else{
                printf("[FILE] %s\n", full_path);
            }
        }else{
            perror("stat");
        } 
    }
    closedir(dir);    


}

//erase borra directorio si es un fichero o si es un directorio vacio
void Cmd_erase(char *tr[], char *cmd){
    if (tr[1] == NULL){
        fprintf(stderr,"%s \n",cmd);
        return;
    }
    if(remove(tr[1])==0){
        printf("Fichero o directorio vacio %s borrado\n", tr[1]);
    }else{
        perror("remove");
    }

}

//delrec borra directorio si es un fichero o si es un directorio no vacio de forma recursiva
void Cmd_delrec(char *tr[], char *cmd){
    if(tr[1] == NULL){
        fprintf(stderr,"%s \n",cmd);
        return;
    }

    char *path = tr[1];
    struct stat path_stat;
    if(stat(path, &path_stat) != 0){
        perror("stat");
        return;
    } 
    if (S_ISDIR(path_stat.st_mode)) {
        DIR *dir = opendir(path);
        if (dir == NULL) {
            perror("opendir");
            return;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
            char *new_tr[] = {tr[0], full_path, NULL};
            Cmd_delrec(new_tr, cmd);
        }

        closedir(dir);

        if (rmdir(path) == 0) {
            printf("Directorio %s borrado\n", path);
        } else {
            perror("rmdir");
        }
    } else {
        if (remove(path) == 0) {
            printf("Fichero %s borrado\n", path);
        } else {
            perror("remove");
        }
    }
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
  }else if(strcmp(tr[1], "makedir") == 0){
      printf("Creates a directory\n");  
  }else if(strcmp(tr[1], "makefile") == 0){
      printf("Creates a file\n");
  }else if(strcmp(tr[1], "listfile") == 0){
      printf("Gives information on files or directories\n");
  }else if(strcmp(tr[1], "cwd") == 0){
      printf("Prints current working directory\n");
  }else if(strcmp(tr[1], "listdir") == 0){
      printf("Lists directories contents\n");
  }else if(strcmp(tr[1], "erase") == 0){
      printf("Deletes files and/or empty directories\n");
  }else if(strcmp(tr[1], "delrec") == 0){
      printf("Deletes files and/or non empty directories recursively\n");
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
     }else if(strcmp(tr[0], "makedir") == 0){
            Cmd_makedir(tr);
     }else if(strcmp(tr[0], "makefile") == 0){
            Cmd_makefile(tr, cmd);
     }else if(strcmp(tr[0], "listfile") == 0){
            Cmd_listfile(tr, cmd);
     }else if(strcmp(tr[0], "cwd") == 0){
            Cmd_cwd();
     }else if(strcmp(tr[0], "listdir") == 0){
            Cmd_listdir(tr,cmd);
     }else if(strcmp(tr[0], "erase") == 0){
            Cmd_erase(tr,cmd);
     }else if(strcmp(tr[0], "delrec") == 0){
            Cmd_delrec(tr, cmd);
     }else if(strcmp(tr[0], "revlist") == 0){
            Cmd_revlist(tr[1]);
     }else if(strcmp(tr[0], "reclist") == 0){
            Cmd_reclist(tr[1]);       
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
