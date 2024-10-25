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
#include "list.h"
#define MAX 1024
#define MAXTR 100
#define MAXH 100

int TrocearCadena(char *cadena, char *tr[]);
void procesarEntrada(char *cmd, bool *terminado, char *tr[], tListP *openFilesList);

int max_depth=0;

char *history[MAXH];
int history_count = 0;

typedef struct File{
    int fd;
    char name[MAX];
    int mode;
}File;

File openFiles[MAX];

int openFilesCount = 0;

typedef struct ComandNode {
    char *name;       
    struct ComandNode *next;      
} ComandNode;

ComandNode *historyList = NULL;
int totalCommands = 0;

typedef struct {
    char path[MAX];
    int level;
    int is_dir; // Add this line
    off_t size; // Add this line to store the size of the file
} FileEntry;

FileEntry entries[MAX];
int entry_count = 0;

void freeEntries() {
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


void addCommand(char *tr[]) {


    char command[MAX] = "";
    for (int i = 0; tr[i] != NULL; i++) {
        strcat(command, tr[i]);
        if (tr[i + 1] != NULL) {
            strcat(command, " ");
        }
    }


    ComandNode *newNode = (ComandNode *)malloc(sizeof(ComandNode));
    newNode->name = strdup(command);
    newNode->next = historyList;
    historyList = newNode;
    totalCommands++;
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

void printHistory() {
    ComandNode *current = historyList;
    int index = totalCommands - 1;
    while (current != NULL) {
        printf("%d-> %s\n", index, current->name);
        current = current->next;
        index--;
    }
}

void printLastNCommands(int n) {
    if (n > totalCommands) {
        n = totalCommands;
    }
    ComandNode *current = historyList;
    int start = totalCommands - n;
    for (int i = totalCommands - 1; i >= start; i--) {
        printf("%d-> %s\n", i, current->name);
        current = current->next;
    }
}

void clearHistory(){
    ComandNode *current = historyList;
    ComandNode *next;
    while(current != NULL){
        next = current->next;
        free(current->name);
        free(current);
        current = next;
    }
    historyList = NULL;
    totalCommands = 0;
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

void Cmd_cwd(){
    char path[MAX];
    if(getcwd(path,MAX)==NULL){
        perror("getcwd");
    }else{
        printf("Directorio actual %s\n", path);
    }
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

void fileinfo(const char *path, struct stat *file_stat, int long_format, int acc_time, int link_info) {
    const char *filename = strrchr(path, '/') ? strrchr(path, '/') + 1 : path;

    if (long_format) {
        char timebuf[80];
        struct tm *tm_info = localtime(&file_stat->st_mtime);
        strftime(timebuf, sizeof(timebuf), "%Y/%m/%d-%H:%M", tm_info);

        char *mode_str = ConvierteModo2(file_stat->st_mode);

        printf("%s %3lu (%8ld) %8s %8s %12s %8ld %s\n",
               timebuf,
               file_stat->st_nlink,
               (long)file_stat->st_ino,
               getpwuid(file_stat->st_uid)->pw_name,
               getgrgid(file_stat->st_gid)->gr_name,
                mode_str,
               file_stat->st_size,
               filename);              
    } else if (acc_time) {
        char timebuf[80];
        struct tm *tm_info = localtime(&file_stat->st_atime);
        strftime(timebuf, sizeof(timebuf), "%Y/%m/%d-%H:%M", tm_info);

        printf("%8ld  %s %s\n", file_stat->st_size, timebuf, filename);
    } else if (link_info && S_ISLNK(file_stat->st_mode)) {
        char link_target[MAX];
        ssize_t len = readlink(path, link_target, sizeof(link_target) - 1);
        if (len != -1) {
            link_target[len] = '\0';
            printf("%s -> %s\n", filename, link_target);
        } else {
            perror("readlink");
        }
    } else {
        printf("%8ld  %s\n", file_stat->st_size, filename);
    }
    
}
//listfile, muestra: nombre y tamaño
// ls -l

//listfile -long, muestra: nº enlaces, inodo, user,group,permisos,tamaño,nombre
//ls -l -i 
void Cmd_listdir(char *tr[], char *cmd) {
    int long_format = 0;
    int acc_time = 0;
    int link_info = 0;
    int show_hidden = 0;
    int start_index = 1;

    // Parse flags
    for (int i = 1; tr[i] != NULL; i++) {
        if (strcmp(tr[i], "-long") == 0) {
            long_format = 1;
            start_index++;
        } else if (strcmp(tr[i], "-acc") == 0) {
            acc_time = 1;
            start_index++;
        } else if (strcmp(tr[i], "-link") == 0) {
            link_info = 1;
            start_index++;
        } else if (strcmp(tr[i], "-hid") == 0) {
            show_hidden = 1;
            start_index++;
        } else {
            break;
        }
    }

    // Check if no directories are provided
    if (tr[start_index] == NULL) {
        Cmd_cwd();
        return;
    }

    for (int i = start_index; tr[i] != NULL; i++) {
        DIR *dir = opendir(tr[i]);
        if (dir == NULL) {
            perror(tr[i]);
            continue;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (!show_hidden && entry->d_name[0] == '.') {
                continue;
            }

            char full_path[MAX];
            snprintf(full_path, sizeof(full_path), "%s/%s", tr[i], entry->d_name);

            struct stat file_stat;
            if (stat(full_path, &file_stat) == 0 || (link_info && lstat(full_path, &file_stat) == 0)) {
                fileinfo(full_path, &file_stat, long_format, acc_time, link_info);
            } else {
                perror(full_path);
            }
        }

        closedir(dir);
    }
}

void Cmd_listfile(char *tr[], char *cmd) {
    int long_format = 0;
    int acc_time = 0;
    int link_info = 0;
    int start_index = 1;

    // Parse flags
    for (int i = 1; tr[i] != NULL; i++) {
        if (strcmp(tr[i], "-long") == 0) {
            long_format = 1;
            start_index++;
        } else if (strcmp(tr[i], "-acc") == 0) {
            acc_time = 1;
            start_index++;
        } else if (strcmp(tr[i], "-link") == 0) {
            link_info = 1;
            start_index++;
        } else {
            break;
        }
    }

    // Check if no files are provided
    if (tr[start_index] == NULL) {
        Cmd_cwd();
        return;
    }

    struct stat file_stat;
    for (int i = start_index; tr[i] != NULL; i++) {
        if (stat(tr[i], &file_stat) == 0 || (link_info && lstat(tr[i], &file_stat) == 0)) {
            fileinfo(tr[i], &file_stat, long_format, acc_time, link_info);
        } else {
            perror(tr[i]);
        }
    }
}

//cwd muestra el directorio actual y todo lo que hay en el
void find_max_depth(const char *path, int level) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        char full_path[MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        struct stat path_stat;
        if (lstat(full_path, &path_stat) == 0) {
            if (S_ISDIR(path_stat.st_mode)) {
                if (level + 1 > max_depth) {
                    max_depth = level + 1;
                }
                find_max_depth(full_path, level + 1);
            }
        } else {
            perror("lstat");
        }
    }
    closedir(dir);
}

int get_max_depth(const char *path) {
    max_depth = 0;
    find_max_depth(path, 0);
    return max_depth;
}

void reclist_aux(const char *path, int level, int show_hidden, int long_format, int acc_time, int link_info) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }                                                     
    
    struct dirent *entry;                       
    struct stat path_stat;                               
    char full_path[MAX];

    printf("************%s\n", path);  // Imprime el directorio actual al iniciar                                                            
            
    // Primero, listar todos los directorios y archivos en este nivel
    while ((entry = readdir(dir)) != NULL) {
        // Si no se muestran los ocultos y el archivo es oculto, se omite
        if (!show_hidden && entry->d_name[0] == '.') {   
            continue;
        }   
    
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);                                                    
    
        if (lstat(full_path, &path_stat) == 0) {
            // Mostrar información del archivo/directorio
            fileinfo(full_path, &path_stat, long_format, acc_time, link_info);                                                       
        }
    }                                                         
            
    // Resetear el flujo del directorio para volver a leer    
    rewinddir(dir);
    while ((entry = readdir(dir)) != NULL) {                      
        // Omitir los directorios "." y ".." para la recursión
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) { 
            continue;                                        
        }   
    
        // Si no se muestran los ocultos y el archivo es oculto, se omite          
        if (!show_hidden && entry->d_name[0] == '.') {
            continue;
        }         
    
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);                                                              
    
        if (lstat(full_path, &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {     
            // Recursivamente listar los contenidos del directorio
            reclist_aux(full_path, level + 1, show_hidden, long_format, acc_time, link_info);                 
        }   
    }                                                        
            
    closedir(dir);                             
}  

void Cmd_reclist(char *tr[], char *cmd) {
    char path[MAX];
    int level = 0;
    int show_hidden = 0;
    int long_format = 0;
    int acc_time = 0;
    int link_info = 0;
    int start_index = 1;

    // Parse flags
    for (int i = start_index; tr[i] != NULL; i++) {
        if (strcmp(tr[i], "-?") == 0) {
            printf("reclist [-hid][-long][-link][-acc] n1 n2 ..\tlista recursivamente contenidos de directorios (subdirs antes)\n");
            printf("\t-hid: incluye los ficheros ocultos\n");
            printf("\t-long: listado largo\n");
            printf("\t-acc: acesstime\n");
            printf("\t-link: si es enlace simbolico, el path contenido\n");
            return;
        }if (strcmp(tr[i], "-hid") == 0) {
            show_hidden = 1;
            start_index++;
        } else if (strcmp(tr[i], "-long") == 0) {
            long_format = 1;
            start_index++;
        } else if (strcmp(tr[i], "-acc") == 0) {
            acc_time = 1;
            start_index++;
        } else if (strcmp(tr[i], "-link") == 0) {
            link_info = 1;
            start_index++;
        } else {
            break;
        }
    }

    if (tr[start_index] == NULL) {
        // No directory provided, use current directory
        if (getcwd(path, sizeof(path)) != NULL) {
            printf("Current directory: %s\n", path);
        } else {
            perror("getcwd");
        }
    } else {
        // Iterate over all provided directories
        for (int i = start_index; tr[i] != NULL; i++) {
            strncpy(path, tr[i], sizeof(path) - 1);
            path[sizeof(path) - 1] = '\0'; // Ensure null-termination
            printf("Listing directory: %s\n", path);
            reclist_aux(path, level, show_hidden, long_format, acc_time, link_info);
        }
    }

}    

void guardar_entries_revlist(const char *path, int level, int show_hidden) {                                           
    DIR *dir = opendir(path);                         
    if (dir == NULL) {                          
        perror("opendir");                           
        return;                                           
    }   
                                                                 
    struct dirent *entry;                       
                                                            
    // Guarda el propio directorio en entries
    struct stat dir_stat;                                      
    if (lstat(path, &dir_stat) == 0 && entry_count < MAX) {
        strncpy(entries[entry_count].path, path, MAX); 
        entries[entry_count].level = level;                       
        entries[entry_count].is_dir = 1; // Marca como directorio
        entries[entry_count].size = dir_stat.st_size;                                                         
        entry_count++;
    }     
    // Lee el contenido del directorio         
    while ((entry = readdir(dir)) != NULL) {                   
        // Omite . y ..                                   
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {                                                         
            continue;
        }                                                   
            
        // Si show_hidden es 0 y el archivo es oculto, lo omite
        if (!show_hidden && entry->d_name[0] == '.') {
            continue;                                  
        }                                                         
                        
        // Construye el path completo para la entrada             
        char full_path[MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);  
            // Procesa la entrada
            struct stat path_stat;                                 
            if (lstat(full_path, &path_stat) == 0) {          
                if (S_ISDIR(path_stat.st_mode)) {
                    // Si es un directorio, llama recursivamente       
                    guardar_entries_revlist(full_path, level + 1, show_hidden);                                                         
                } else {                            
                    // Si es un archivo, revisa si es duplicado
                    int is_duplicate = 0;               
                    for (int i = 0; i < entry_count; i++) {
                        if (strcmp(entries[i].path, full_path) == 0) {
                            is_duplicate = 1;
                            break;                                    
                        }
                    }
                    if (!is_duplicate && entry_count < MAX) {
                    // Almacena el archivo en entries
                    strncpy(entries[entry_count].path, full_path, MAX);                                                                
                    entries[entry_count].level = level;   
                    entries[entry_count].is_dir = 0; // Marca como archivo
                    entries[entry_count].size = path_stat.st_size; 
                    entry_count++;              
                }                                           
            }                    
        } else {                                         
            perror("lstat");                        
        }
    }
                        
    closedir(dir);                
} 

void print_entries_revlist(int long_format, int acc_time, int link_info, int show_hidden) {               
    int printed[MAX]; // Array para marcar entradas ya impresas
    memset(printed, 0, sizeof(printed)); // Inicializa a 0
                 
    // Itera en orden inverso para imprimir directorios antes de los archivos           
    for (int i = entry_count - 1; i >= 0; i--) {
        if (entries[i].is_dir) {                            
            struct stat dir_stat;
            if (lstat(entries[i].path, &dir_stat) == 0) {
                printf("************%s\n", entries[i].path);

                // Imprime archivos y subdirectorios dentro del directorio actual
                for (int j = entry_count - 1; j >= 0; j--) {
                    if (strncmp(entries[j].path, entries[i].path, strlen(entries[i].path)) == 0) {
                        const char *relative_path = entries[j].path + strlen(entries[i].path) + 1;

                        // Verifica que esté en el mismo nivel del directorio actual y no esté impreso
                        if (strchr(relative_path, '/') == NULL && strcmp(entries[j].path, entries[i].path) != 0) {
                            if (!printed[j]) {
                                struct stat file_stat;
                                if (lstat(entries[j].path, &file_stat) == 0) {
                                    fileinfo(entries[j].path, &file_stat, long_format, acc_time, link_info);
                                    printed[j] = 1; // Marca como impreso
                                }
                            }
                        }
                    }
                }

                // Imprime los directorios . y .. si show_hidden está habilitado
                if (show_hidden) {
                    char dot_path[MAX + 3]; // Increase buffer size to accommodate additional characters
                    char dotdot_path[MAX + 4]; // Increase buffer size to accommodate additional characters
                    snprintf(dot_path, sizeof(dot_path), "%s/.", entries[i].path);
                    snprintf(dotdot_path, sizeof(dotdot_path), "%s/..", entries[i].path);

                    struct stat dot_stat;
                    struct stat dotdot_stat;

                    if (lstat(dot_path, &dot_stat) == 0) {
                        fileinfo(dot_path, &dot_stat, long_format, acc_time, link_info);
                    }
                    if (lstat(dotdot_path, &dotdot_stat) == 0) {
                        fileinfo(dotdot_path, &dotdot_stat, long_format, acc_time, link_info);
                    }
                }
            }
        }
    }
}


void revlist_aux(const char *path, int level, int show_hidden, int long_format, int acc_time, int link_info) {
    entry_count = 0; // Reinicia el contador de entradas
    guardar_entries_revlist(path, level, show_hidden); // Guarda las entradas del directorio y sus subdirectorios
    print_entries_revlist(long_format, acc_time, link_info, show_hidden); // Imprime las entradas guardadas
}


void Cmd_revlist(char *tr[], char *cmd){
    char path[MAX];
    int show_hidden = 0;
    int long_format = 0;
    int acc_time = 0;
    int link_info = 0;
    int start_index = 1;

    // Parse flags
    for (int i = start_index; tr[i] != NULL; i++) {
        if (strcmp(tr[i], "-?") == 0) {
            printf("revlist [-hid][-long][-link][-acc] n1 n2 ..\tlista recursivamente contenidos de directorios (subdirs antes)\n");
            printf("%s", "\t-hid: incluye los ficheros ocultos\n");
            printf("%s", "\t-long: listado largo\n");
            printf("%s", "\t-acc: acesstime\n");
            printf("%s", "\t-link: si es enlace simbolico, el path contenido\n");
            return;
        }if (strcmp(tr[i], "-hid") == 0) {
            show_hidden = 1;
            start_index++;
        } else if (strcmp(tr[i], "-long") == 0) {
            long_format = 1;
            start_index++;
        } else if (strcmp(tr[i], "-acc") == 0) {
            acc_time = 1;
            start_index++;
        } else if (strcmp(tr[i], "-link") == 0) {
            link_info = 1;
            start_index++;
        } else {
            break;
        }
    }

    if (tr[start_index] == NULL) {
        // No directory provided, use current directory
        if (getcwd(path, sizeof(path)) != NULL) {
            printf("Current directory: %s\n", path);
        } else {
            perror("getcwd");
        }
    } else {
        // Iterate over all provided directories
        for (int i = start_index; tr[i] != NULL; i++) {
            strncpy(path, tr[i], sizeof(path) - 1);
            path[sizeof(path) - 1] = '\0'; // Ensure null-termination
            revlist_aux(path, 0, show_hidden, long_format, acc_time, link_info);
        }
    }

    
}



//erase borra directorio si es un fichero o si es un directorio vacio
void Cmd_erase(char *tr[], char *cmd){
   for(int i = 1; tr[i] != NULL; i++){
        if(tr[i] != NULL){
            if (remove(tr[i]) == 0) {
                printf("Fichero %s borrado\n", tr[i]);
            } else {
                perror("remove");
            }
        }else{
            fprintf(stderr,"%s \n",cmd);
        }
    }

}

//delrec borra directorio si es un fichero o si es un directorio no vacio de forma recursiva
void Cmd_delrec(char *tr[], char *cmd){
   for(int i = 1; tr[i] != NULL; i++){
    if(tr[i] == NULL){
        fprintf(stderr,"%s \n",cmd);
        return;
    }

    char *path = tr[i];
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
             if (getcwd(path, MAX) == NULL) {
                perror("getcwd");
            } else {
                printf("Directorio cambiado a %s\n", path);
            }
        }
    }else{
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

void ejecutarComandoHistorico(int index, bool *terminado, char *tr[], tListP *openFilesList) {
    if (index < 0 || index >= totalCommands) {
        printf("Error: Número de comando inválido\n");
        return;
    }

    ComandNode *current = historyList;
    for (int i = totalCommands - 1; i > index; i--) {
        current = current->next;
    }

    printf("Ejecutando hist (%d): %s\n", index, current->name);

    char cmd[MAX];
    strcpy(cmd, current->name);
    TrocearCadena(cmd, tr);
    procesarEntrada(current->name, terminado, tr, openFilesList);
}



void Cmd_hist(char *tr[], char *cmd, bool *terminado, tListP *openFilesList){
    if (tr[1] == NULL) {  // Sin argumentos, imprime todo el historial
         printHistory(-1);
    }else if(strcmp(tr[1], "-?")==0){
        printf("historic [-c|-N|N]	Muestra (o borra)el historico de comandos\n-N: muestra los N primeros\n-c: borra el historico\nN: repite el comando N\n");
        return;
    }else if(strcmp(tr[1], "-c")==0){
        clearHistory();

    } else if (tr[2] == NULL) {  // Un argumento, imprime el comando en esa posición
        int n = atoi(tr[1]);
        if(n>=0){
            ejecutarComandoHistorico(n, terminado, tr, openFilesList);
        }else{
            printLastNCommands(-n); 
        }
    }else {
        fprintf(stderr,"%s \n",cmd);
    }    
}  

void Cmd_open (char * tr[], tListP *openFilesList){                                         
    int i,df, mode=0;      
        
    if (tr[1]==NULL) {                       
            ListFicherosAbiertos(0, openFilesList); 
        return;                                                  
    }          
     	
    if (strcmp(tr[1], "-?") == 0) {
        printf("open fich m1 m2...    Abre el fichero fich\n");
        printf("    y lo anade a la lista de ficheros abiertos del shell\n");
        printf("    m1, m2..es el modo de apertura (or bit a bit de los siguientes)\n");
        printf("    cr: O_CREAT    ap: O_APPEND\n");
        printf("    ex: O_EXCL     ro: O_RDONLY\n");
        printf("    rw: O_RDWR     wo: O_WRONLY\n");
        printf("    tr: O_TRUNC\n");
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

    if(strcmp(tr[1], "-?") == 0){
        printf("close df	Cierra el descriptor df y elimina el correspondiente fichero de la lista de ficheros abiertos\n");
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

    if(strcmp(tr[1], "-?")==0){
        printf("dup df	Duplica el descriptor de fichero df y anade una nueva entrada a la lista ficheros abiertos\n");
        return;
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

void Cmd_quit(bool *terminado, char *tr[], tListP *openFilesList){
    if(tr[1] == NULL){
        *terminado = true;
        clearHistory();
        if(openFilesList!=NULL){
            deleteList(openFilesList);
        }    
        freeEntries();    
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



void guardarLista(char *entrada, char *tr[]) {
    // Guardar el comando en el historial
    addCommand(tr);

    // Actualizar el último comando guardado
    static char *lastCommand = NULL;
    if (lastCommand != NULL) {
        free(lastCommand);
    }
    lastCommand = strdup(entrada);
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
    char entrada[MAX] = {0};
    char cmd[MAX] = {0};
    char *tr[MAXTR] = {0};
    bool terminado = false;
    tListP openFilesList;    
    createEmptyList(&openFilesList);
    while (!terminado){
        imprimirPrompt();
        leerEntrada(cmd,tr,entrada);
        guardarLista(entrada, tr);
        procesarEntrada(cmd,&terminado, tr, &openFilesList);
        }   
    clearHistory();   
    return 0;    
}

    
