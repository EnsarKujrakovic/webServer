#include <sys/dir.h>  
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <string.h>
#include <linux/limits.h>
#include <unistd.h>
#include <stdlib.h>

#define IS_IRUSR(m) ((m & S_IRUSR) ? 'r' : '-')
#define IS_IWUSR(m) ((m & S_IWUSR) ? 'w' : '-')  
#define IS_IXUSR(m) ((m & S_IXUSR) ? 'x' : '-')
#define IS_IRGRP(m) ((m & S_IRGRP) ? 'r' : '-')
#define IS_IWGRP(m) ((m & S_IWGRP) ? 'w' : '-')
#define IS_IXGRP(m) ((m & S_IXGRP) ? 'x' : '-')
#define IS_IROTH(m) ((m & S_IROTH) ? 'r' : '-')
#define IS_IWOTH(m) ((m & S_IWOTH) ? 'w' : '-')
#define IS_IXOTH(m) ((m & S_IXOTH) ? 'x' : '-')
#define IS_ISUID(m) ((m & S_ISUID) ? 'U' : '-')
#define IS_ISGID(m) ((m & S_ISGID) ? 'G' : '-')
#define IS_ISVTX(m) ((m & S_ISVTX) ? 'S' : '-')


char permissions[20] = {0};
char date[30] = {0};
void getPermissions(mode_t);
char getType(mode_t);
void Print(struct stat);

int main(int argc, char** argv){
    struct dirent **fileList;
    struct stat entrystat;
    lstat(argv[1], &entrystat);
    if(S_ISLNK(entrystat.st_mode) || S_ISREG(entrystat.st_mode) || S_ISCHR(entrystat.st_mode) || //ispis za fajl koji nije direktorij
	   S_ISBLK(entrystat.st_mode) || S_ISFIFO(entrystat.st_mode) || S_ISSOCK(entrystat.st_mode)){
		
		getPermissions(entrystat.st_mode);
		strftime(date, 30, "%Y-%m-%d %H:%M", localtime(&entrystat.st_mtime));
		printf("    %9.9s %7lu %c %3lu %4d %4d %10lu %s %s",
				permissions,
				entrystat.st_ino,
				getType(entrystat.st_mode),
				entrystat.st_nlink,
				entrystat.st_uid,
				entrystat.st_gid,
				entrystat.st_size,
				date,
				argv[1]);
			if(S_ISLNK(entrystat.st_mode)){ //ispis putanje za simbolicki link
		    	char lpath[PATH_MAX] = {0};
		    	readlink(argv[1], lpath, PATH_MAX);
		    	printf(" -> %s", lpath);
		    }
		    printf("\n");
	}else if(S_ISDIR(entrystat.st_mode)|| argv[1] == NULL){//ispis za direktorij ili bez argumenta
		char path[PATH_MAX];char tmp [PATH_MAX];	
		if(argv[1]== NULL) {strcpy(path, ".");}//ako je poziv bez argumenata otvara se direktorij . i putanja se postavlja na "" jer je . trenutni direktorij
		else {strcpy(path, argv[1]); strcat(path, "/");}//otvara se direktorij iz proslijedjenog argumenta i putanja se postavlja na vrijednost argumenta
		int n = scandir(path, &fileList, NULL, alphasort);
		int i = 0;
		while(i < n){//ispis sadrzaja direktorija
		    strcpy(tmp, path);
		    strcat(tmp, fileList[i]->d_name);
		    lstat(tmp, &entrystat);
		    
		    getPermissions(entrystat.st_mode);
		    strftime(date, 30, "%Y-%m-%d %H:%M", localtime(&entrystat.st_mtime));
		    printf("    %9.9s %7lu %c %3lu %4d %4d %10lu %s %s",
				permissions,
				fileList[i]->d_ino,
				getType(entrystat.st_mode),
				entrystat.st_nlink,
				entrystat.st_uid,
				entrystat.st_gid,
				entrystat.st_size,
				date,
				fileList[i]->d_name);
			if(S_ISLNK(DTTOIF(fileList[i]->d_type))){ //ispis putanje za simbolicki link
		    	char lpath[PATH_MAX] = {0};
		    	readlink(tmp, lpath, PATH_MAX);
		    	printf(" -> %s", lpath);
		    }
		    if(S_ISDIR(DTTOIF(fileList[i]->d_type)))
		    	printf("/");
			printf("\n");
			free(fileList[i]);
			++i;
		}
		free(fileList);
		
	}else{//ako argument nije validan
	
		printf("Cannot access '%s': No such file or directory\n", argv[1]);
	
	}
}

char getType(mode_t m ){//vraca tip fajla
	char type;
	if (S_ISREG(m)) type = '-';
	if (S_ISLNK(m)) type = 'l';
	if (S_ISDIR(m)) type = 'd';
	if (S_ISCHR(m)) type = 'c';
	if (S_ISBLK(m)) type = 'b';
	if (S_ISFIFO(m)) type = 'p';
	if (S_ISSOCK(m)) type = 's';
	return type;
}
void getPermissions(mode_t m){//string permissions puni sa permisijama 
	sprintf(permissions, "%c%c%c%c%c%c%c%c%c",  
	 	IS_IRUSR(m),
	 	IS_IWUSR(m),
	 	IS_IXUSR(m),
	 	IS_IRGRP(m),
	 	IS_IWGRP(m),
		IS_IXGRP(m),
		IS_IROTH(m),
		IS_IWOTH(m),
		IS_IXOTH(m));
}
