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
#include <libgen.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "list.h"

#define TAMANO 2048
#define MAX 1024
#define MAXTR 100
#define MAXH 100

int TrocearCadena(char *cadena, char *tr[]); //Funcion que trocea la cadena de entrada
void procesarEntrada(char *cmd, bool *terminado, char *tr[], tListP *openFilesList); //Funcion que procesa la entrada

int max_depth=0; // Profundidad máxima para find

char *history[MAXH]; // Historial de comandos
int history_count = 0; // Número de comandos en el historial

typedef struct File{ // Estructura para almacenar los ficheros abiertos
    int fd;
    char name[MAX];
    int mode;
}File;

File openFiles[MAX]; // Array de ficheros abiertos
int openFilesCount = 0; // Número de ficheros abiertos

typedef struct ComandNode { // Estructura para almacenar los comandos en el historial
    char *name;       
    struct ComandNode *next;      
} ComandNode;

ComandNode *historyList = NULL; // Lista de comandos en el historial
int totalCommands = 0; // Número total de comandos en el historial
static char *lastCommand = NULL; // Último comando ejecutado

typedef struct { // Estructura para almacenar las entradas de los directorios
    char path[MAX];
    int level;
    int is_dir;
    off_t size; 
} FileEntry;

FileEntry entries[MAX]; // Array de entradas de directorios
int entry_count = 0; // Número de entradas de directorios

void freeEntries() { // Función para liberar las entradas de directorios
    entry_count = 0;
}   


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

char * ConvierteModo3 (mode_t m)
{
    char *permisos;

    if ((permisos=(char *) malloc (12))==NULL)
        return NULL;
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

void Recursiva (int n){
  char automatico[TAMANO];
  static char estatico[TAMANO];

  printf ("parametro:%3d(%p) array %p, arr estatico %p\n",n,&n,automatico, estatico);

  if (n>0)
    Recursiva(n-1);
}

void LlenarMemoria (void *p, size_t cont, unsigned char byte)
{
  unsigned char *arr=(unsigned char *) p;
  size_t i;

  for (i=0; i<cont;i++)
                arr[i]=byte;
}


void * ObtenerMemoriaShmget (key_t clave, size_t tam)
{
    void * p;
    int aux,id,flags=0777;
    struct shmid_ds s;

    if (tam)     /*tam distito de 0 indica crear */
        flags=flags | IPC_CREAT | IPC_EXCL; /*cuando no es crear pasamos de tamano 0*/
    if (clave==IPC_PRIVATE)  /*no nos vale*/
        {errno=EINVAL; return NULL;}
    if ((id=shmget(clave, tam, flags))==-1)
        return (NULL);
    if ((p=shmat(id,NULL,0))==(void*) -1){
        aux=errno;
        if (tam)
             shmctl(id,IPC_RMID,NULL);
        errno=aux;
        return (NULL);
    }
    shmctl (id,IPC_STAT,&s); /* si no es crear, necesitamos el tamano, que es s.shm_segsz*/
 /* Guardar en la lista   InsertarNodoShared (&L, p, s.shm_segsz, clave); */
    return (p);
}

void do_AllocateCreateshared (char *tr[])
{
   key_t cl;
   size_t tam;
   void *p;

   if (tr[0]==NULL || tr[1]==NULL) {
                ImprimirListaShared(&L);
                return;
   }
  
   cl=(key_t)  strtoul(tr[0],NULL,10);
   tam=(size_t) strtoul(tr[1],NULL,10);
   if (tam==0) {
        printf ("No se asignan bloques de 0 bytes\n");
        return;
   }
   if ((p=ObtenerMemoriaShmget(cl,tam))!=NULL)
                printf ("Asignados %lu bytes en %p\n",(unsigned long) tam, p);
   else
                printf ("Imposible asignar memoria compartida clave %lu:%s\n",(unsigned long) cl,strerror(errno));
}


void do_AllocateShared (char *tr[])
{
   key_t cl;
   size_t tam;
   void *p;

   if (tr[0]==NULL) {
                ImprimirListaShared(&L);
                return;
   }
  
   cl=(key_t)  strtoul(tr[0],NULL,10);

   if ((p=ObtenerMemoriaShmget(cl,0))!=NULL)
                printf ("Asignada memoria compartida de clave %lu en %p\n",(unsigned long) cl, p);
   else
                printf ("Imposible asignar memoria compartida clave %lu:%s\n",(unsigned long) cl,strerror(errno));
}


void * MapearFichero (char * fichero, int protection)
{
    int df, map=MAP_PRIVATE,modo=O_RDONLY;
    struct stat s;
    void *p;

    if (protection&PROT_WRITE)
          modo=O_RDWR;
    if (stat(fichero,&s)==-1 || (df=open(fichero, modo))==-1)
          return NULL;
    if ((p=mmap (NULL,s.st_size, protection,map,df,0))==MAP_FAILED)
           return NULL;
/* Guardar en la lista    InsertarNodoMmap (&L,p, s.st_size,df,fichero); */
/* Gurdas en la lista de descriptores usados df, fichero*/
    return p;
}

void do_AllocateMmap(char *arg[])
{ 
     char *perm;
     void *p;
     int protection=0;
     
     if (arg[0]==NULL)
            {ImprimirListaMmap(&L); return;}
     if ((perm=arg[1])!=NULL && strlen(perm)<4) {
            if (strchr(perm,'r')!=NULL) protection|=PROT_READ;
            if (strchr(perm,'w')!=NULL) protection|=PROT_WRITE;
            if (strchr(perm,'x')!=NULL) protection|=PROT_EXEC;
     }
     if ((p=MapearFichero(arg[0],protection))==NULL)
             perror ("Imposible mapear fichero");
     else
             printf ("fichero %s mapeado en %p\n", arg[0], p);
}

void do_DeallocateDelkey (char *args[])
{
   key_t clave;
   int id;
   char *key=args[0];

   if (key==NULL || (clave=(key_t) strtoul(key,NULL,10))==IPC_PRIVATE){ 
        printf ("      delkey necesita clave_valida\n");
        return;
   }
   if ((id=shmget(clave,0,0666))==-1){
        perror ("shmget: imposible obtener memoria compartida");
        return;
   }
   if (shmctl(id,IPC_RMID,NULL)==-1)
        perror ("shmctl: imposible eliminar memoria compartida\n");
}


ssize_t LeerFichero (char *f, void *p, size_t cont)
{
   struct stat s;
   ssize_t  n;  
   int df,aux;

   if (stat (f,&s)==-1 || (df=open(f,O_RDONLY))==-1)
        return -1;     
   if (cont==-1)   /* si pasamos -1 como bytes a leer lo leemos entero*/
        cont=s.st_size;
   if ((n=read(df,p,cont))==-1){
        aux=errno;
        close(df);
        errno=aux;
        return -1;
   }
   close (df);
   return n;
}

void Cmd_ReadFile (char *ar[])
{
   void *p;
   size_t cont=-1;  /*si no pasamos tamano se lee entero */
   ssize_t n;
   if (ar[0]==NULL || ar[1]==NULL){
        printf ("faltan parametros\n");
        return;
   }
   p=cadtop(ar[1]);  /*convertimos de cadena a puntero*/
   if (ar[2]!=NULL)
        cont=(size_t) atoll(ar[2]);

   if ((n=LeerFichero(ar[0],p,cont))==-1)
        perror ("Imposible leer fichero");
   else
        printf ("leidos %lld bytes de %s en %p\n",(long long) n,ar[0],p);
}

void Do_pmap (void) /*sin argumentos*/
 { pid_t pid;       /*hace el pmap (o equivalente) del proceso actual*/   
   char elpid[32];
   char *argv[4]={"pmap",elpid,NULL};   
                         
   sprintf (elpid,"%d", (int) getpid()); 
   if ((pid=fork())==-1){
      perror ("Imposible crear proceso");
      return;  
      }                            
   if (pid==0){                                         
      if (execvp(argv[0],argv)==-1)
         perror("cannot execute pmap (linux, solaris)");
         
      argv[0]="procstat"; argv[1]="vm"; argv[2]=elpid; argv[3]=NULL;    
      if (execvp(argv[0],argv)==-1)/*No hay pmap, probamos procstat FreeBSD */
         perror("cannot execute procstat (FreeBSD)");  
            
      argv[0]="procmap",argv[1]=elpid;argv[2]=NULL;    
            if (execvp(argv[0],argv)==-1)  /*probamos procmap OpenBSD*/ 
         perror("cannot execute procmap (OpenBSD)");
         
      argv[0]="vmmap"; argv[1]="-interleave"; argv[2]=elpid;argv[3]=NULL;                                                     
      if (execvp(argv[0],argv)==-1) /*probamos vmmap Mac-OS*/
         perror("cannot execute vmmap (Mac-OS)");      
      exit(1);         
  }
  waitpid (pid,NULL,0);
}



 
void addCommand(char *tr[]) { // Función para añadir un comando al historial
    char command[MAX] = ""; // Comando a añadir
    for (int i = 0; tr[i] != NULL; i++) { // Recorrer los argumentos del comando
        strcat(command, tr[i]); // Añadir el argumento al comando
        if (tr[i + 1] != NULL) { // Si no es el último argumento
            strcat(command, " "); // Añadir un espacio al comando
        }
    }
    ComandNode *newNode = (ComandNode *)malloc(sizeof(ComandNode)); // Crear un nuevo nodo para el comando
    newNode->name = strdup(command); // Copiar el comando al nodo
    newNode->next = historyList; // El siguiente nodo es el actual
    historyList = newNode; // El nuevo nodo es el actual
    totalCommands++; // Incrementar el número total de comandos
}


int getTotalHistCount() { // Función para obtener el número total de comandos en el historial
    ComandNode *current = historyList;  // Nodo actual
    int count = 0; // Contador
    while(current != NULL){ // Mientras haya nodos
        count++; // Incrementar el contador
        current = current->next; // Pasar al siguiente nodo
    }
    return count; // Devolver el contador
}

void printHistory() { // Función para imprimir el historial de comandos
    ComandNode *current = historyList; // Nodo actual
    int index = totalCommands - 1; // Índice
    while (current != NULL) { // Mientras haya nodos
        printf("%d-> %s\n", index, current->name); // Imprimir el índice y el comando
        current = current->next; // Pasar al siguiente nodo
        index--; // Decrementar el índice
    }
}

void printLastNCommands(int n) { // Función para imprimir los últimos N comandos
    if (n > totalCommands) { // Si el número de comandos a imprimir es mayor que el total de comandos
        n = totalCommands; // El número de comandos a imprimir es igual al total de comandos
    }
    ComandNode *current = historyList; // Nodo actual
    int start = totalCommands - n; // Índice de inicio
    for (int i = totalCommands - 1; i >= start; i--) { // Recorrer los comandos
        printf("%d-> %s\n", i, current->name); // Imprimir el índice y el comando
        current = current->next; // Pasar al siguiente nodo
    }
}

void clearHistory(){ // Función para limpiar el historial de comandos
    ComandNode *current = historyList; // Nodo actual
    ComandNode *next; // Nodo siguiente
    while(current != NULL){ // Mientras haya nodos
        next = current->next; // Guardar el siguiente nodo
        free(current->name); // Liberar la memoria del nombre del comando
        free(current); // Liberar la memoria del nodo
        current = next; // Pasar al siguiente nodo
    }
    historyList = NULL; // El historial es NULL
    totalCommands = 0; // El número total de comandos es 0
}   

int TrocearCadena(char * cadena, char * tr[]){  //Función que trocea la cadena de entrada
    int i=1; //Contador
    if ((tr[0]=strtok(cadena," \n\t"))==NULL){  //Trocea la cadena
        return 0;  //Devuelve 0 si no hay nada
    }
    while ((tr[i]=strtok(NULL," \n\t"))!=NULL){ //Trocea la cadena
        i++; //Incrementa el contador
    }
    return i; //Devuelve el contador   
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

void AnadirFicherosAbiertos(int fd, const char *name, int mode){ //Función para añadir ficheros abiertos
    if(openFilesCount<MAX){ //Si el número de ficheros abiertos es menor que el máximo
        openFiles[openFilesCount].fd = fd; //El descriptor de fichero es igual al descriptor de fichero
        strncpy(openFiles[openFilesCount].name, name,MAX); //Copia el nombre del fichero
        openFiles[openFilesCount].mode = mode; //El modo es igual al modo
        openFilesCount++; //Incrementa el número de ficheros abiertos
    }else{
        fprintf(stderr,"Error");
    }
    
}

void EliminarDeFicherosAbiertos(int fd){ //Función para eliminar de ficheros abiertos
    for (int i = 0; i<openFilesCount;i++){ //Recorre los ficheros abiertos
        if (openFiles[i].fd == fd){ //Si el descriptor de fichero es igual al descriptor de fichero
            printf("File descriptor %d closed\n", openFiles[i].fd); //Imprime que el descriptor de fichero se ha cerrado
            for (int j = i; j<openFilesCount-1;j++){ //Recorre los ficheros abiertos
                openFiles[j] = openFiles[j+1]; //El fichero actual es igual al siguiente fichero
            }
            openFilesCount--; //Decrementa el número de ficheros abiertos
            break;
        }
    }
}

void ListFicherosAbiertos(int fd, tListP *L){ //Función para listar los ficheros abiertos
    for(int i = 0; i<openFilesCount;i++){ //Recorre los ficheros abiertos
        printf("Descriptor %d, Name: %s, Mode: %s\n", openFiles[i].fd,openFiles[i].name, Mode(openFiles[i].mode) ); //Imprime el descriptor de fichero, el nombre y el modo
    }
}

char *NombreFicheroDescriptor(int fd){ //Función para obtener el nombre del fichero descriptor
    for(int i=0; i<openFilesCount;i++){ //Recorre los ficheros abiertos
        if(openFiles[i].fd == fd){ //Si el descriptor de fichero es igual al descriptor de fichero
            return openFiles[i].name; //Devuelve el nombre del fichero
        }
    }
    return NULL;
}

void Cmd_cwd(){ //Función para obtener el directorio actual
    char path[MAX]; //Directorio actual
    if(getcwd(path,MAX)==NULL){ //Si no se puede obtener el directorio actual
        perror("getcwd"); //Imprime el error
    }else{
        printf("Directorio actual %s\n", path); //Imprime el directorio actual
    }
}

void Cmd_makedir(char *tr[]) { //Función para crear un directorio
    // Intentar crear el directorio
    if(tr[1] != NULL){ //Si el directorio no es nulo
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
            printf("Directorio '%s' creado correctamente.\n", tr[1]); //Imprime que el directorio se ha creado correctamente
        }
    }
}

//makefile, crea un fichero con el nombre que se le pase
void Cmd_makefile(char *tr[], char *cmd){ //Función para crear un fichero
    if(tr[1] != NULL && tr[2] == NULL){ //Si el fichero no es nulo y el segundo argumento es nulo
        int fd = open(tr[1], O_CREAT | O_WRONLY, 0644); //Abre el fichero
        if(fd == -1){ //Si el descriptor de fichero es -1
            perror("open"); //Imprime el error
        }else{
            printf("File %s created with descriptor %d\n", tr[1], fd); //Sino, imprime que el fichero se ha creado con el descriptor de fichero
            AnadirFicherosAbiertos(fd, tr[1], O_CREAT | O_WRONLY); //Añade el fichero a los ficheros abiertos
        }
    }else{
        fprintf(stderr,"%s \n",cmd);
    }
}


void fileinfo(const char *path, struct stat *file_stat, int long_format, int acc_time, int link_info) { //Función para obtener información del fichero
    const char *filename = strrchr(path, '/') ? strrchr(path, '/') + 1 : path; //Nombre del fichero

    if (long_format) { //Si el formato es largo
        char timebuf[80]; //Buffer de tiempo
        struct tm *tm_info = localtime(&file_stat->st_mtime); //Información de tiempo
        strftime(timebuf, sizeof(timebuf), "%Y/%m/%d-%H:%M", tm_info); //Formatea el tiempo

        char *mode_str = ConvierteModo3(file_stat->st_mode); //Convierte el modo

        printf("%s %3lu (%8ld) %8s %8s %12s %8ld %s\n",
               timebuf, 
               file_stat->st_nlink,
               (long)file_stat->st_ino,
                getpwuid(file_stat->st_uid)->pw_name,
                getpwuid(file_stat->st_uid)->pw_name,
                mode_str,
               file_stat->st_size,
               filename); //Imprime la informacion del fichero en formato largo
        free(mode_str); //Libera la memoria del modo   
    } else if (acc_time) { //Si se quiere mostrar el tiempo de acceso
        char timebuf[80]; //Buffer de tiempo
        struct tm *tm_info = localtime(&file_stat->st_atime); //Información de tiempo
        strftime(timebuf, sizeof(timebuf), "%Y/%m/%d-%H:%M", tm_info); //Formatea el tiempo

        printf("%8ld  %s %s\n", file_stat->st_size, timebuf, filename); //Imprime el tamaño, el tiempo y el nombre del fichero
    } else if (link_info && S_ISLNK(file_stat->st_mode)) { //Si se quiere mostrar el enlace simbolico
        char link_target[MAX]; //Enlace simbolico
        ssize_t len = readlink(path, link_target, sizeof(link_target) - 1); //Lee el enlace simbolico
        if (len != -1) { //Si la longitud es diferente de -1
            link_target[len] = '\0'; //El enlace simbolico es nulo
            printf("%s -> %s\n", filename, link_target); //Imprime el nombre del fichero y el enlace simbolico
        } else {
            perror("readlink");
        }
    } else {
        printf("%8ld  %s\n", file_stat->st_size, filename); //Si no se le indica ningun flag, imprime el tamaño y el nombre del fichero
    }
    
}

//listfile, muestra: nombre y tamaño
// ls -l

//listfile -long, muestra: nº enlaces, inodo, user,group,permisos,tamaño,nombre
//ls -l -i 
void Cmd_listdir(char *tr[], char *cmd) { //Función para listar directorios
    int long_format = 0; //Flag -long
    int acc_time = 0; //Flag -acc
    int link_info = 0; //Flag -link
    int show_hidden = 0; //Flag -hid
    int start_index = 1;

    //flags
    for (int i = 1; tr[i] != NULL; i++) { //Recorre los argumentos
        if (strcmp(tr[i], "-long") == 0) { //Si el flag es -long
            long_format = 1; //Flag -long a 1
            start_index++; //Incrementa el contador
        } else if (strcmp(tr[i], "-acc") == 0) { //Si el flag es -acc
            acc_time = 1; //Flag -acc a 1
            start_index++; //Incrementa el contador
        } else if (strcmp(tr[i], "-link") == 0) { //Si el flag es -link
            link_info = 1; //Flag -link a 1
            start_index++; //Incrementa el contador
        } else if (strcmp(tr[i], "-hid") == 0) { //Si el flag es -hid
            show_hidden = 1; //Flag -hid a 1
            start_index++; //Incrementa el contador
        } else {
            break;
        }
    }

    // Check if no directories are provided
    if (tr[start_index] == NULL) { //Si no se proporcionan directorios
        Cmd_cwd(); //Obtiene el directorio actual
        return;
    }

    for (int i = start_index; tr[i] != NULL; i++) { //Recorre los directorios
        DIR *dir = opendir(tr[i]); //Abre el directorio
        if (dir == NULL) { //Si el directorio es nulo
            perror(tr[i]); //Imprime el error
            continue;
        }

        struct dirent *entry; //Entrada
        while ((entry = readdir(dir)) != NULL) { //Mientras haya entradas
            if (!show_hidden && entry->d_name[0] == '.') { //Si no se muestran los ocultos y el archivo es oculto continua
                continue; 
            }

            char full_path[MAX]; //Path completo
            snprintf(full_path, sizeof(full_path), "%s/%s", tr[i], entry->d_name); //Copia el path completo

            struct stat file_stat; //Información del fichero //corregir lstat
            if (stat(full_path, &file_stat) == 0 || (link_info && lstat(full_path, &file_stat) == 0)) { //Si se puede obtener la información del fichero
                fileinfo(full_path, &file_stat, long_format, acc_time, link_info); //Obtiene la información del fichero
            } else {
                perror(full_path); //Si no se puede obtener, imprime el error
            }
        }

        closedir(dir); //Cierra el directorio
    }
}

void Cmd_listfile(char *tr[], char *cmd) { //Función para listar ficheros
    int long_format = 0; //Flag -long
    int acc_time = 0; //Flag -acc
    int link_info = 0; //Flag -link
    int start_index = 1;

    // flags
    for (int i = 1; tr[i] != NULL; i++) {  //Recorre los argumentos
        if (strcmp(tr[i], "-long") == 0) { //Si el flag es -long
            long_format = 1; //Flag -long a 1
            start_index++; //Incrementa el contador
        } else if (strcmp(tr[i], "-acc") == 0) { //Si el flag es -acc
            acc_time = 1; //Flag -acc a 1
            start_index++; //Incrementa el contador
        } else if (strcmp(tr[i], "-link") == 0) { //Si el flag es -link
            link_info = 1; //Flag -link a 1
            start_index++; //Incrementa el contador
        } else {
            break;
        }
    }

    if (tr[start_index] == NULL) { //Si no se proporcionan directorios
        Cmd_cwd(); //Obtiene el directorio actual
        return;
    }

    struct stat file_stat; //Información del fichero //corregir lstat
    for (int i = start_index; tr[i] != NULL; i++) { //Recorre los directorios
        if (stat(tr[i], &file_stat) == 0 || (link_info && lstat(tr[i], &file_stat) == 0)) { //Si se puede obtener la información del fichero
            fileinfo(tr[i], &file_stat, long_format, acc_time, link_info); //Obtiene la información del fichero
        } else {
            perror(tr[i]); //Si no se puede obtener, imprime el error
        }
    }
}

void find_max_depth(const char *path, int level) { //Función para encontrar la profundidad máxima
    DIR *dir = opendir(path); //Abre el directorio
    if (dir == NULL) { //Si el directorio es nulo
        perror("opendir"); //Imprime el error
        return;
    }

    struct dirent *entry; //Entrada
    while ((entry = readdir(dir)) != NULL) { //Mientras haya entradas
        char full_path[MAX]; //Path completo
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name); //Copia el path completo

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) { //Si el nombre de la entrada es . o .. continua
            continue; 
        }

        struct stat path_stat; //Información del fichero
        if (lstat(full_path, &path_stat) == 0) { //Si se puede obtener la información del fichero
            if (S_ISDIR(path_stat.st_mode)) { //Si es un directorio
                if (level + 1 > max_depth) { //Si el nivel + 1 es mayor que la profundidad máxima
                    max_depth = level + 1; //La profundidad máxima es igual al nivel + 1
                }
                find_max_depth(full_path, level + 1); //Llama recursivamente a la función
            }
        } else {
            perror("lstat");
        }
    }
    closedir(dir); //Cierra el directorio
}

int get_max_depth(const char *path) { //Función que devuelve la profundidad máxima
    max_depth = 0; //Profundidad máxima a 0
    find_max_depth(path, 0); //Llama a la función para encontrar la profundidad máxima
    return max_depth; //Devuelve la profundidad máxima
}

void reclist_aux(const char *path, int level, int show_hidden, int long_format, int acc_time, int link_info) { //Función auxiliar para listar recursivamente de fuera hacia dentro
    DIR *dir = opendir(path); //Abre el directorio
    if (dir == NULL) { //Si el directorio es nulo
        perror("opendir"); //Imprime el error
        return;
    }                                                     
    
    struct dirent *entry; //Entrada                 
    struct stat path_stat; //Información del fichero                              
    char full_path[MAX]; //Path completo

    printf("************%s\n", path);  // Imprime el directorio actual al iniciar                                                            

    while ((entry = readdir(dir)) != NULL) { //Mientras haya entradas
        if (!show_hidden && entry->d_name[0] == '.') { //Si no se muestran los ocultos y el archivo es oculto continua   
            continue;
        }   
    
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name); //Copia el path completo                                                
    
        if (lstat(full_path, &path_stat) == 0) { //Si se puede obtener la información del fichero
            fileinfo(full_path, &path_stat, long_format, acc_time, link_info);  //Obtiene la información del fichero                                                      
        }
    }                                                         
       
    rewinddir(dir); //Reinicia el directorio
    while ((entry = readdir(dir)) != NULL) {    //Mientras haya entradas                    
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) { //Si el nombre de la entrada es . o .. continua
            continue;                                        
        }   
    
        if (!show_hidden && entry->d_name[0] == '.') { //Si no se muestran los ocultos y el archivo es oculto continua
            continue;
        }         
    
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name); //Copia el path completo                                                           
    
        if (lstat(full_path, &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {  //Si se puede obtener la información del fichero y es un directorio
            reclist_aux(full_path, level + 1, show_hidden, long_format, acc_time, link_info); //Llama recursivamente a la función              
        }   
    }                                                        
            
    closedir(dir); //Cierra el directorio                           
}  

void Cmd_reclist(char *tr[], char *cmd) { //Función para listar recursivamente de fuera hacia dentro
    char path[MAX]; //Path
    int level = 0; //Nivel
    int show_hidden = 0; //Flag -hid
    int long_format = 0; //Flag -long
    int acc_time = 0; //Flag -acc
    int link_info = 0; //Flag -link
    int start_index = 1;

    //flags
    for (int i = start_index; tr[i] != NULL; i++) {
        if (strcmp(tr[i], "-?") == 0) { //Si el flag es -?
            printf("reclist [-hid][-long][-link][-acc] n1 n2 ..\tlista recursivamente contenidos de directorios (subdirs antes)\n");
            printf("\t-hid: incluye los ficheros ocultos\n");
            printf("\t-long: listado largo\n");
            printf("\t-acc: acesstime\n");
            printf("\t-link: si es enlace simbolico, el path contenido\n"); //Imprime la ayuda
            return;
        }if (strcmp(tr[i], "-hid") == 0) { //Si el flag es -hid
            show_hidden = 1; //Flag -hid a 1
            start_index++; //Incrementa el contador
        } else if (strcmp(tr[i], "-long") == 0) { //Si el flag es -long
            long_format = 1; //Flag -long a 1
            start_index++; //Incrementa el contador
        } else if (strcmp(tr[i], "-acc") == 0) { //Si el flag es -acc
            acc_time = 1; //Flag -acc a 1
            start_index++; //Incrementa el contador
        } else if (strcmp(tr[i], "-link") == 0) { //Si el flag es -link 
            link_info = 1; //Flag -link a 1 
            start_index++; //Incrementa el contador
        } else {
            break;
        }
    }

    if (tr[start_index] == NULL) { //Si no se proporcionan directorios
        if (getcwd(path, sizeof(path)) != NULL) { //Si se puede obtener el directorio actual
            printf("Current directory: %s\n", path); //Imprime el directorio actual
        } else {
            perror("getcwd");
        }
    } else {
        for (int i = start_index; tr[i] != NULL; i++) { //Recorre los directorios
            strncpy(path, tr[i], sizeof(path) - 1); //Copia el directorio
            path[sizeof(path) - 1] = '\0'; // El path es nulo
            printf("Listing directory: %s\n", path); //Imprime el directorio
            reclist_aux(path, level, show_hidden, long_format, acc_time, link_info); //Llama a la función auxiliar para listar recursivamente de fuera hacia dentro
        }
    }

}    

void guardar_entries_revlist(const char *path, int level, int show_hidden) { //Función para guardar las entradas de los directorios de dentro hacia fuera en orden inverso                                      
    DIR *dir = opendir(path); //Abre el directorio                         
    if (dir == NULL) {  //Si el directorio es nulo                      
        perror("opendir");                           
        return;                                           
    }   
                                                                 
    struct dirent *entry; //Entrada                 
    struct stat dir_stat; //Información del fichero           
    if (lstat(path, &dir_stat) == 0 && entry_count < MAX) { //Si se puede obtener la información del fichero y el contador de entradas es menor que el máximo
        strncpy(entries[entry_count].path, path, MAX);  //Copia el path al array de entradas
        entries[entry_count].level = level; //Nivel                       
        entries[entry_count].is_dir = 1; // Marca como directorio
        entries[entry_count].size = dir_stat.st_size; //Tamaño                                                         
        entry_count++; //Incrementa el contador de entradas
    }     
            
    while ((entry = readdir(dir)) != NULL) {  //Mientras haya entradas                                                  
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {  //Si el nombre de la entrada es . o .. continua                                                        
            continue;
        }                                                   
            
        if (!show_hidden && entry->d_name[0] == '.') { //Si no se muestran los ocultos y el archivo es oculto continua
            continue;                                  
        }                                                         
                                   
        char full_path[MAX]; //Path completo
        struct stat path_stat; //Información del fichero 
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name); //Copia el path completo                                  
        if (lstat(full_path, &path_stat) == 0) { //Si se puede obtener la información del fichero        
            if (S_ISDIR(path_stat.st_mode)) { //Si es un directorio
                guardar_entries_revlist(full_path, level + 1, show_hidden);                                                         
            } else {                           
                int is_duplicate = 0; //Indica si es un duplicado               
                for (int i = 0; i < entry_count; i++) { //Recorre las entradas
                    if (strcmp(entries[i].path, full_path) == 0) { //Si el path es igual al path completo
                        is_duplicate = 1; //Es un duplicado
                        break;                                    
                    }
                }
                if (!is_duplicate && entry_count < MAX) { //Si no es un duplicado y el contador de entradas es menor que el máximo
                strncpy(entries[entry_count].path, full_path, MAX); //Copia el path al array de entradas                                                                
                entries[entry_count].level = level; //Nivel   
                entries[entry_count].is_dir = 0; // Marca como archivo
                entries[entry_count].size = path_stat.st_size; //Tamaño 
                entry_count++; //Incrementa el contador de entradas              
                }                                           
            }                    
        } else {                                         
            perror("lstat");                        
        }
    }                  
    closedir(dir); //Cierra el directorio                
} 

void print_entries_revlist(int long_format, int acc_time, int link_info, int show_hidden) { //Función para imprimir las entradas de los directorios de dentro hacia fuera en orden inverso              
    int printed[MAX]; // Array para marcar entradas ya impresas
    memset(printed, 0, sizeof(printed)); // Inicializa el array a 0
                            
    for (int i = entry_count - 1; i >= 0; i--) { //Recorre las entradas en orden inverso
        if (entries[i].is_dir) { //Si es un directorio                            
            struct stat dir_stat; //Información del fichero
            if (lstat(entries[i].path, &dir_stat) == 0) { //Si se puede obtener la información del fichero
                printf("************%s\n", entries[i].path); //Imprime el directorio actual al iniciar

                for (int j = entry_count - 1; j >= 0; j--) { //Recorre las entradas en orden inverso
                    if (strncmp(entries[j].path, entries[i].path, strlen(entries[i].path)) == 0) { //Si el path es igual al path de la entrada
                        const char *relative_path = entries[j].path + strlen(entries[i].path) + 1; //Path relativo

                        if (strchr(relative_path, '/') == NULL && strcmp(entries[j].path, entries[i].path) != 0) { //Si no hay una barra y el path no es igual al path de la entrada
                            if (!printed[j]) { //Si no está impreso
                                struct stat file_stat; //Información del fichero
                                if (lstat(entries[j].path, &file_stat) == 0) { //Si se puede obtener la información del fichero
                                    fileinfo(entries[j].path, &file_stat, long_format, acc_time, link_info); //Obtiene la información del fichero
                                    printed[j] = 1; // Marca como impreso
                                }
                            }
                        }
                    }
                }

                if (show_hidden) { //Si se muestran los ocultos
                    char dot_path[MAX + 3]; // Aumentar el tamaño del buffer para acomodar caracteres adicionales
                    char dotdot_path[MAX + 4]; // Aumentar el tamaño del buffer para acomodar caracteres adicionales
                    snprintf(dot_path, sizeof(dot_path), "%s/.", entries[i].path); //Copia el path .
                    snprintf(dotdot_path, sizeof(dotdot_path), "%s/..", entries[i].path); //Copia el path ..

                    struct stat dot_stat; //Información del fichero .
                    struct stat dotdot_stat; //Información del fichero ..

                    if (lstat(dot_path, &dot_stat) == 0) { //Si se puede obtener la información del fichero .
                        fileinfo(dot_path, &dot_stat, long_format, acc_time, link_info); //Obtiene la información del fichero
                    }
                    if (lstat(dotdot_path, &dotdot_stat) == 0) { //Si se puede obtener la información del fichero ..
                        fileinfo(dotdot_path, &dotdot_stat, long_format, acc_time, link_info); //Obtiene la información del fichero
                    }
                }
            }
        }
    }
}


void revlist_aux(const char *path, int level, int show_hidden, int long_format, int acc_time, int link_info) { //Función auxiliar para listar recursivamente de dentro hacia fuera en orden inverso
    entry_count = 0; // Reinicia el contador de entradas
    guardar_entries_revlist(path, level, show_hidden); // Guarda las entradas del directorio y sus subdirectorios
    print_entries_revlist(long_format, acc_time, link_info, show_hidden); // Imprime las entradas guardadas
}


void Cmd_revlist(char *tr[], char *cmd){ //Función para listar recursivamente de dentro hacia fuera en orden inverso
    char path[MAX]; //Path
    int show_hidden = 0; //Flag -hid
    int long_format = 0; //Flag -long
    int acc_time = 0; //Flag -acc
    int link_info = 0; //Flag -link
    int start_index = 1;

    //flags
    for (int i = start_index; tr[i] != NULL; i++) {
        if (strcmp(tr[i], "-?") == 0) { //Si el flag es -?
            printf("revlist [-hid][-long][-link][-acc] n1 n2 ..\tlista recursivamente contenidos de directorios (subdirs antes)\n");
            printf("%s", "\t-hid: incluye los ficheros ocultos\n");
            printf("%s", "\t-long: listado largo\n");
            printf("%s", "\t-acc: acesstime\n");
            printf("%s", "\t-link: si es enlace simbolico, el path contenido\n"); //Imprime la ayuda
            return;
        }if (strcmp(tr[i], "-hid") == 0) { //Si el flag es -hid
            show_hidden = 1; //Flag -hid a 1
            start_index++; //Incrementa el contador
        } else if (strcmp(tr[i], "-long") == 0) { //Si el flag es -long
            long_format = 1; //Flag -long a 1
            start_index++; //Incrementa el contador
        } else if (strcmp(tr[i], "-acc") == 0) { //Si el flag es -acc
            acc_time = 1; //Flag -acc a 1
            start_index++; //Incrementa el contador
        } else if (strcmp(tr[i], "-link") == 0) { //Si el flag es -link
            link_info = 1; //Flag -link a 1
            start_index++; //Incrementa el contador 
        } else {
            break;
        }
    }

    if (tr[start_index] == NULL) { //Si no se proporcionan directorios
        if (getcwd(path, sizeof(path)) != NULL) { //Si se puede obtener el directorio actual
            printf("Current directory: %s\n", path); //Imprime el directorio actual
        } else {
            perror("getcwd");
        }
    } else {
        for (int i = start_index; tr[i] != NULL; i++) { //Recorre los directorios
            strncpy(path, tr[i], sizeof(path) - 1); //Copia el directorio
            path[sizeof(path) - 1] = '\0'; // El path es nulo 
            revlist_aux(path, 0, show_hidden, long_format, acc_time, link_info); //Llama a la función auxiliar para listar recursivamente de dentro hacia fuera en orden inverso
        }
    }

    
}

void Cmd_erase(char *tr[], char *cmd){  //Función para borrar ficheros 
   for(int i = 1; tr[i] != NULL; i++){ //Recorre los ficheros
        if(tr[i] != NULL){ //Si el fichero no es nulo
            if (remove(tr[i]) == 0) { //Si se puede borrar el fichero
                printf("Fichero %s borrado\n", tr[i]); //Imprime que el fichero se ha borrado
            } else {
                perror("remove");
            }
        }else{
            fprintf(stderr,"%s \n",cmd); //Imprime el error
        }
    }

}

void Cmd_delrec(char *tr[], char *cmd){ //Función para borrar directorios recursivamente
   for(int i = 1; tr[i] != NULL; i++){ //Recorre los directorios
    if(tr[i] == NULL){ //Si el directorio es nulo
        fprintf(stderr,"%s \n",cmd); //Imprime el error
        return; 
    }

    char *path = tr[i]; //Path
    struct stat path_stat; //Información del fichero
    if(stat(path, &path_stat) != 0){ //Si no se puede obtener la información del fichero
        perror("stat"); //Imprime el error
        return;
    } 
    if (S_ISDIR(path_stat.st_mode)) { //Si es un directorio
        DIR *dir = opendir(path); //Abre el directorio
        if (dir == NULL) { //Si el directorio es nulo
            perror("opendir"); //Imprime el error
            return;
        }

        struct dirent *entry; //Entrada
        while ((entry = readdir(dir)) != NULL) { //Mientras haya entradas
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) { //Si el nombre de la entrada es . o .. continua
                continue;
            }

            char full_path[1024]; //Path completo
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name); //Copia el path completo
            char *new_tr[] = {tr[0], full_path, NULL}; //Nuevo array de argumentos
            Cmd_delrec(new_tr, cmd); //Llama recursivamente a la función
        }

        closedir(dir); //Cierra el directorio

        if (rmdir(path) == 0) { //Si se puede borrar el directorio
            printf("Directorio %s borrado\n", path); //Imprime que el directorio se ha borrado
        } else {
            perror("rmdir");
        }
    } else {
        if (remove(path) == 0) { //Si se puede borrar el fichero
            printf("Fichero %s borrado\n", path); //Imprime que el fichero se ha borrado
        } else {
            perror("remove");
        }
    }
   }
}

void Cmd_authors(char *tr[], char *cmd){ //Función para mostrar los autores
    if (tr[1] == NULL){ //Si no hay argumentos
        printf("Ivan Afonso Cerdeira: ivan.afonso@udc.es, Minerva Antia Lago Lopez: minerva.lago.lopez@udc.es\n"); //Imprime los autores
    }else if (strcmp(tr[1], "-l") == 0){ //Si el argumento es -l
        printf("ivan.afonso@udc.es, minerva.lago.lopez\n"); //Imprime los correos
    }else if(strcmp(tr[1], "-n") == 0){ //Si el argumento es -n
        printf("Ivan Afonso Cerdeira, Minerva Antia \n"); //Imprime los nombres
    }else{
        fprintf(stderr,"%s \n",cmd); //Imprime el error
    }
}

void Cmd_pid(char *tr[], char *cmd){ //Función para mostrar el PID
    if (tr[1] == NULL){ //Si no hay argumentos
        printf("Process ID: %d\n", getpid()); //Imprime el PID
    }else{
        fprintf(stderr,"%s \n",cmd); //Imprime el error
    }
}

void Cmd_ppid(char *tr[], char *cmd){ //Función para mostrar el PPID
    if (tr[1] == NULL){ //Si no hay argumentos
        printf("Parent Process ID: %d\n", getppid()); //Imprime el PPID
    }else{
        fprintf(stderr,"%s \n",cmd); //Imprime el error
    }
}

void Cmd_cd(char *tr[], char *cmd){ //Función para cambiar de directorio
    char path[MAX]; //Path
    if(tr[1] == NULL){ //Si no hay argumentos
        if(getcwd(path,MAX)==NULL){ //Si no se puede obtener el directorio actual
            perror("getcwd"); //Imprime el error
        }else {
            printf("Directorio actual %s\n", path); //Imprime el directorio actual
        }
    }else if(tr[2] == NULL){ //Si hay un argumento
        if (chdir(tr[1])== -1){ //Si no se puede cambiar de directorio
            printf("No se ha podido cambiar de directorio\n"); //Imprime que no se ha podido cambiar de directorio
        }else{
             if (getcwd(path, MAX) == NULL) { //Si no se puede obtener el directorio actual
                perror("getcwd"); //Imprime el error
            } else {
                printf("Directorio cambiado a %s\n", path); //Imprime que el directorio se ha cambiado
            }
        }
    }else{
        fprintf(stderr,"%s \n",cmd); //Imprime el error

   }
}

void Cmd_date(char *tr[], char *cmd){ //Función para mostrar la fecha y la hora                                   
   if(tr[1] == NULL){ //Si no hay argumentos
        time_t t; //Tiempo
        struct tm *tm; //Estructura de tiempo
        char fechayhora[100]; //Fecha y hora
        t = time(NULL); //Tiempo actual
        tm = localtime(&t); //Tiempo local
        strftime(fechayhora, 100, "%d/%m/%Y %H:%M:%S", tm); //Formatea la fecha y la hora
        printf("Fecha y hora: %s\n", fechayhora); //Imprime la fecha y la hora
    }else if(strcmp(tr[1],"-d") == 0){ //Si el argumento es -d
        time_t t; //Tiempo
        struct tm *tm; //Estructura de tiempo
        char fechayhora[100]; //Fecha y hora
        t = time(NULL); //Tiempo actual
        tm = localtime(&t); //Tiempo local
        strftime(fechayhora, 100, "%d/%m/%Y", tm); //Formatea la fecha
        printf("Fecha: %s\n", fechayhora); //Imprime la fecha
    }else if(strcmp(tr[1],"-t") == 0){ //Si el argumento es -t
        time_t t; //Tiempo
        struct tm *tm; //Estructura de tiempo
        char fechayhora[100]; //Fecha y hora
        t = time(NULL); //Tiempo actual
        tm = localtime(&t); //Tiempo local
        strftime(fechayhora, 100, "%H:%M:%S", tm); //Formatea la hora
        printf("Hora: %s\n", fechayhora); //Imprime la hora
    }else{
        fprintf(stderr,"%s \n",cmd); //Imprime el error
    }
}

void ejecutarComandoHistorico(int index, bool *terminado, char *tr[], tListP *openFilesList) { //Función para ejecutar un comando del historial
    if (index < 0 || index >= totalCommands) { //Si el índice es menor que 0 o mayor que el número total de comandos
        printf("Error: Número de comando inválido\n"); //Imprime que el número de comando es inválido
        return;
    }

    ComandNode *current = historyList; //Nodo actual
    for (int i = totalCommands - 1; i > index; i--) { //Recorre los comandos
        current = current->next; //Siguiente nodo
    }

    printf("Ejecutando hist (%d): %s\n", index, current->name); //Imprime que se está ejecutando el comando del historial

    char cmd[MAX]; //Comando
    strcpy(cmd, current->name); //Copia el comando
    TrocearCadena(cmd, tr); //Trocea el comando
    procesarEntrada(current->name, terminado, tr, openFilesList); //Procesa la entrada
}



void Cmd_hist(char *tr[], char *cmd, bool *terminado, tListP *openFilesList){ //Función para mostrar el historial
    if (tr[1] == NULL) { //Si no hay argumentos
         printHistory(-1); //Imprime el historial
    }else if(strcmp(tr[1], "-?")==0){ //Si el argumento es -?
        printf("historic [-c|-N|N]	Muestra (o borra)el historico de comandos\n-N: muestra los N primeros\n-c: borra el historico\nN: repite el comando N\n"); //Imprime la ayuda
        return;
    }else if(strcmp(tr[1], "-c")==0){ //Si el argumento es -c
        clearHistory(); //Borra el historial
    } else if (tr[2] == NULL) { //Si hay un argumento
        int n = atoi(tr[1]); //Convierte el argumento a entero
        if(n>=0){
            ejecutarComandoHistorico(n, terminado, tr, openFilesList); //Ejecuta el comando del historial
        }else{
            printLastNCommands(-n); //Imprime los últimos N comandos 
        }
    }else {
        fprintf(stderr,"%s \n",cmd); //Imprime el error
    }    
}  

void Cmd_open (char * tr[], tListP *openFilesList){ //Función para abrir un fichero                                       
    int i,df, mode=0; //Variables       
        
    if (tr[1]==NULL) {  //No hay parametro                      
            ListFicherosAbiertos(0, openFilesList);  //Lista los ficheros abiertos
        return;                                                  
    }          
     	
    if (strcmp(tr[1], "-?") == 0) { //Si el argumento es -?
        printf("open fich m1 m2...    Abre el fichero fich\n");
        printf("    y lo anade a la lista de ficheros abiertos del shell\n");
        printf("    m1, m2..es el modo de apertura (or bit a bit de los siguientes)\n");
        printf("    cr: O_CREAT    ap: O_APPEND\n");
        printf("    ex: O_EXCL     ro: O_RDONLY\n");
        printf("    rw: O_RDWR     wo: O_WRONLY\n");
        printf("    tr: O_TRUNC\n"); //Imprime la ayuda
        return;
    }
    for (i=2; tr[i]!=NULL; i++) //Recorre los argumentos
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
        AnadirFicherosAbiertos(df,tr[1],mode); //Añade el fichero a la lista de ficheros abiertos                                                       
        printf ("Anadida entrada a la tabla de ficheros abiertos.\nDescriptor: %d, Name: %s, Mode: %s\n",df, tr[1], Mode(mode)); //Imprime que se ha añadido el fichero a la lista de ficheros abiertos                                               
    }
} 

void Cmd_close (char *tr[], tListP *openFilesList){ //Función para cerrar un fichero
    int df; //Descriptor de fichero

    if (tr[1]==NULL || (df=atoi(tr[1]))<0) { //No hay parametro
        ListFicherosAbiertos(0,openFilesList); //Lista los ficheros abiertos
        return;
    }

    if(strcmp(tr[1], "-?") == 0){ //Si el argumento es -?
        printf("close df	Cierra el descriptor df y elimina el correspondiente fichero de la lista de ficheros abiertos\n"); //Imprime la ayuda
        return;
    }

    if (close(df)==-1) { //Si no se puede cerrar el descriptor
        perror("Imposible cerrar descriptor"); //Imprime el error
    }else{
        EliminarDeFicherosAbiertos(df); //Elimina el fichero de la lista de ficheros abiertos
    }
}

void Cmd_dup (char * tr[], tListP *L){ //Función para duplicar un descriptor de fichero
    int df, duplicado, ormode; //Variables
    char aux[MAX],*orname;; //Auxiliar

    if (tr[1]==NULL || (df=atoi(tr[1]))<0) { //No hay parametros
        ListFicherosAbiertos(-1, L);// Lista los ficheros abiertos
        return;               
    }

    if(strcmp(tr[1], "-?")==0){ //Si el argumento es -?
        printf("dup df	Duplica el descriptor de fichero df y anade una nueva entrada a la lista ficheros abiertos\n"); //Imprime la ayuda
        return;
    }

    duplicado=dup(df); //Duplica el descriptor de fichero
    if(duplicado==-1){ //Si no se puede duplicar el descriptor
        perror("Imposible duplicar descriptor"); //Imprime el error
        return;
    }

    orname=NombreFicheroDescriptor(df); //Nombre del fichero del descriptor
    ormode=fcntl(df,F_GETFL); //Modo de apertura del descriptor
    if(ormode==-1){ //Si no se puede obtener el modo de apertura
        perror("Imposible obtener modo de apertura"); //Imprime el error
        return;
    }

    snprintf(aux, MAX, "%s (duplicated)", orname); //Copia el nombre del fichero duplicado
    AnadirFicherosAbiertos(duplicado,aux,ormode); //Añade el fichero duplicado a la lista de ficheros abiertos
}

void Cmd_infosys(char *tr[], char *cmd){ //Función para mostrar información del sistema
    struct utsname uts; //Estructura de información del sistema
    if (tr[1] == NULL){ //Si no hay argumentos
        uname(&uts); //Obtiene la información del sistema
        printf("Systemname: %s\n", uts.sysname); //Imprime el nombre del sistema
        printf("Nodename: %s\n", uts.nodename); //Imprime el nombre del nodo
        printf("Release: %s\n", uts.release); //Imprime el release
        printf("Version: %s\n", uts.version); //Imprime la versión
        printf("Machine: %s\n", uts.machine); //Imprime la máquina
    }else{
        fprintf(stderr,"%s \n",cmd);
    }
}

void Cmd_help(char *tr[], char *cmd){ //Función para mostrar ayuda sobre los comandos
    if (tr[1] == NULL || strcmp(tr[1], "-?") == 0) {
        printf("help [cmd|-lt|-T|-all]    Muestra ayuda sobre los comandos\n");
        printf("    -lt: lista topics de ayuda\n");
        printf("    -T topic: lista comandos sobre ese topic\n");
        printf("    cmd: info sobre el comando cmd\n");
        printf("    -all: lista todos los topics con sus comandos\n");
        return;
    }
    if (strcmp(tr[1], "-lt") == 0) {
        printf("Topics de ayuda disponibles:\n");
        printf("    archivos\n");
        printf("    directorios\n");
        printf("    sistema\n");
        return;
    }

     if (strcmp(tr[1], "-T") == 0 && tr[2] != NULL) {
        if (strcmp(tr[2], "archivos") == 0) {
            printf("Comandos sobre archivos:\n");
            printf("    open: Abre un archivo\n");
            printf("    close: Cierra un archivo\n");
            printf("    list: Lista archivos\n");
        } else if (strcmp(tr[2], "directorios") == 0) {
            printf("Comandos sobre directorios:\n");
            printf("    cd: Cambia de directorio\n");
            printf("    makedir: Crea un directorio\n");
            printf("    makefile: Crea un archivo\n");
        } else if (strcmp(tr[2], "sistema") == 0) {
            printf("Comandos sobre el sistema:\n");
            printf("    infosys: Muestra información del sistema\n");
            printf("    historic: Muestra el historial de comandos\n");
        } else {
            printf("Topic no encontrado: %s\n", tr[2]);
        }
        return;
    }
    if (strcmp(tr[1], "-all") == 0) {
        printf("Todos los topics con sus comandos:\n");
        printf("archivos:\n");
        printf("    open: Abre un archivo\n");
        printf("    close: Cierra un archivo\n");
        printf("    list: Lista archivos\n");
        printf("directorios:\n");
        printf("    cd: Cambia de directorio\n");
        printf("    makedir: Crea un directorio\n");
        printf("    makefile: Crea un archivo\n");
        printf("    reclist: Lista directorios recursivamente de fuera hacia adentro\n");
        printf("    revlist: Lista directorios recursivamente de adentro hacia fuera\n");
        printf("sistema:\n");
        printf("    infosys: Muestra información del sistema\n");
        printf("    historic: Muestra el historial de comandos\n");
        printf("    help: Muestra ayuda sobre los comandos\n");
        return;
    }

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
    }else if(strcmp(tr[1], "revlist")==0){
        printf("lists directories recursively (subdirectories before)\n");
    }else if(strcmp(tr[1], "reclist")==0){
        printf("lists directories recursively (subdirectories after)\n");
    }else if(strcmp(tr[1], "erase") == 0){
        printf("Deletes files and/or empty directories\n");
    }else if(strcmp(tr[1], "delrec") == 0){
        printf("Deletes files and/or non empty directories recursively\n");
    }else{
        fprintf(stderr,"%s \n",cmd);
        }

}

void imprimirPrompt(){ //Función para imprimir el prompt
    printf(":~>"); //Imprime el prompt
}

void leerEntrada(char *cmd, char *tr[], char *entrada){ //Función para leer la entrada
    fgets(entrada,MAX,stdin); //Lee la entrada
    strcpy(cmd,entrada); //Copia la entrada
    strtok(cmd,"\n"); //Trocea la entrada
    TrocearCadena(entrada,tr); //Trocea la entrada
}



void guardarLista(char *entrada, char *tr[]) { //Función para guardar la lista de comandos
    addCommand(tr); // Añadir el comando a la lista de comandos    
    if (lastCommand != NULL) { //Si el último comando no es nulo
        free(lastCommand); //Libera el último comando
    }
    lastCommand = strdup(entrada); //Duplica la entrada
}

void Cmd_quit(bool *terminado, char *tr[], tListP *openFilesList){ //Función para salir del shell
    if(tr[1] == NULL){ //Si no hay argumentos
        *terminado = true; //Terminado a true
        clearHistory(); //Borra el historial
        if(openFilesList!=NULL){ //Si la lista de ficheros abiertos no es nula
            deleteList(openFilesList); //Borra la lista de ficheros abiertos
        }    
        freeEntries(); //Libera las entradas   
        if (lastCommand != NULL){ //Si el último comando no es nulo
        free(lastCommand); //Libera el último comando
        lastCommand = NULL; //El último comando es nulo
        }
        
    }else{
        printf("Error: Invalid option\n");
    }
}




void procesarEntrada(char *cmd, bool *terminado, char *tr[], tListP *openFilesList){ //Función para procesar la entrada
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
            Cmd_hist(tr,cmd, terminado, openFilesList);
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
                Cmd_revlist(tr, cmd);
        }else if(strcmp(tr[0], "reclist") == 0){
                Cmd_reclist(tr, cmd);
        } else{ 
            fprintf(stderr,"%s \n",cmd); 
        }
  }
}

int main(){
    char entrada[MAX]; //Entrada
    char cmd[MAX]; //Comando
    char *tr[MAXTR]; //Trozo
    bool terminado = false; //Flag terminado
    tListP openFilesList; //Lista de ficheros abiertos    
    createEmptyList(&openFilesList); //Crea una lista vacía
    while (!terminado){ //Mientras no esté terminado
        imprimirPrompt(); //Imprime el prompt
        leerEntrada(cmd,tr,entrada); //Lee la entrada
        guardarLista(entrada, tr); //Guarda la lista de comandos
        procesarEntrada(cmd,&terminado, tr, &openFilesList); //Procesa la entrada
        }   
    clearHistory(); //Borra el historial
    deleteList(&openFilesList); //Borra la lista de ficheros abiertos
    freeEntries(); //Libera las entradas
    return 0;    
}

    
