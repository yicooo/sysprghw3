#define FUSE_USE_VERSION 26

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fuse.h>
#include <errno.h>
#include <fcntl.h>
#include "csvgetline2.c"

#define NAMES "NAMES"
#define CODES "CODES"
#define MAXLEN 100

#define MAYBE(x) x?x:"None"

typedef struct {
    char code[6];
    char neighborhood[20];
    char city[20];
	char district[20];
	char latitude[15];
	char longtitude[15];
} name_entry;

int which_map[3] = {2, 3, 1};

int renamePos;
name_entry renameLine;

int find(char ***buffer, char *city, char *district, char *neigh, int is_name, int which) {

	printf(">> find %s %s %s %d %d\n", MAYBE(city), MAYBE(district), MAYBE(neigh), is_name, which);

	int mxlen = MAXLEN, len = 0;
	*buffer = (char **)malloc(sizeof(char*) * mxlen);
	char *line;
	int n;
	FILE *csvfile = fopen("postal-codes.csv", "rb");
	
	while ((line = csvgetline(csvfile)) != NULL) {
		split();
		if (is_name &&
			( (!city || strcmp(csvfield(2), city) == 0) && 
			 (!district || strcmp(csvfield(3), district) == 0 ) &&
			 (!neigh || strcmp(csvfield(1), neigh) == 0)) ) {
				 
			renamePos = ftell(csvfile);
			strcpy(renameLine.code, csvfield(0));
			strcpy(renameLine.neighborhood, csvfield(1));
			strcpy(renameLine.city, csvfield(2));
			strcpy(renameLine.district, csvfield(3));
			strcpy(renameLine.latitude, csvfield(4));
			strcpy(renameLine.longtitude, csvfield(5));
			
		    if (len >= mxlen) {
				mxlen *= 2;
				*buffer = (char**) realloc(*buffer, mxlen * sizeof(char*));
			}

			if (which == 4) {
				printf("WHAT?\n");
				continue;
			}
			else if (city && district && neigh) {
				(*buffer)[len] = calloc(200, sizeof(char));
				int c = sprintf((*buffer)[len++], 
						"code: %s\nneightborhood: %s\ncity: %s\ndistrict: %s\nlatitude: %s\nlongtitude: %s",
						csvfield(0), neigh, city, district, csvfield(4), csvfield(5));
				printf("read: %d\n", c);
				printf("%s\n",(*buffer)[len-1]);
				return len;
			}
			else {
				if (len && strcmp((*buffer)[len-1], csvfield(which_map[which])) == 0)
					continue;
				(*buffer)[len] = malloc(sizeof(char) * 20);
				strcpy((*buffer)[len++], csvfield(which_map[which]));
			}
		}
		else if (!is_name && 
				 (!city || strncmp(city, csvfield(0), 2) == 0)) {
			
			renamePos = ftell(csvfile);
			strcpy(renameLine.code, csvfield(0));
			strcpy(renameLine.neighborhood, csvfield(1));
			strcpy(renameLine.city, csvfield(2));
			strcpy(renameLine.district, csvfield(3));
			strcpy(renameLine.latitude, csvfield(4));
			strcpy(renameLine.longtitude, csvfield(5));
			
			if (len >= mxlen) {
				mxlen *= 2;
				*buffer = (char**) realloc(*buffer, mxlen * sizeof(char*));
			}
			n = 10;
			if (which == 0)
				n = 2;

			if (which == 2) {

				(*buffer)[len] = calloc(200, sizeof(char));
				sprintf((*buffer)[len++], 
						"code: %s\nneightborhood: %s\ncity: %s\ndistrict: %s\nlatitude: %s\nlongtitude: %s\n",
						csvfield(0), csvfield(3), csvfield(1), csvfield(2), csvfield(4), csvfield(5));
				return len;
			}
			if (len && strncmp((*buffer)[len-1], csvfield(0), n) == 0)
				continue;
			(*buffer)[len] = calloc(20, sizeof(char));
			strncpy((*buffer)[len++], csvfield(0), n);
		}
	}

	reset();
	fclose(csvfile);
	return len;
}


int split_path(char **arr, const char *path) {
	printf(">> split_path %s\n", path);
	arr[0] = NULL;
	arr[1] = NULL;
	arr[2] = NULL;
	int pi = 0, i = 0, ix = 0;
	if (path[0] == '/') pi++; 
	for(ix = 0; ix < 4; ix++) {
		if (!path[pi])
			break;
		if (ix == 3)
			return 4;
		arr[ix] = malloc(sizeof(char) * 20);
		i = 0;
		while (path[pi] && path[pi] != '/' && path[pi] != '.'){
			arr[ix][i++] = path[pi++];
		}
		arr[ix][i] = '\0';
		if (path[pi] != '/'){
			ix++;
			break;
		}
		pi++;
	}
	return ix;
}

static int postal_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi){
	printf(">> readdir -%s-\n", path);
	
	if (strcmp(path, "/") == 0) {
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);
		filler(buf, NAMES, NULL, 0);
		filler(buf, CODES, NULL, 0);
	}
	else {
		
		filler(buf, ".", NULL, 0);
		filler(buf, "..", NULL, 0);

		int is_name = (strncmp(path+1, NAMES, 5) == 0);
		int is_code = (strncmp(path+1, CODES, 5) == 0);

		if (!is_name && !is_code)
			return -ENOENT;
		
		char *arr[3]; 
		int which = split_path(arr, path+6);
		
		printf("which: %d\n", which);
		
		char **result;
		int c = find(&result, arr[0], arr[1], arr[2], is_name, which);
		
		printf("count: %d\n", c);
		
		int i;
		for(i = 0; i < c; i++) {
			if ((is_name && which == 2) || (is_code && which == 1)) {
				char filename[20];
				int ix = -1;
				
				while(result[i][++ix])
					filename[ix] = result[i][ix];
				
				strcpy(filename + ix, ".txt");
				printf("file: %s\n", filename);
				filler(buf, filename, NULL, 0);
			}
			else {
				filler(buf, result[i], NULL, 0);
			}
			free(result[i]);
		}
		free(result);
	}
	return 0;
}

static int postal_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi){
	printf(">> getattr %s\n", path);
	int retval = 0;
	
	memset(stbuf, 0, sizeof(struct stat));

	if (strcmp(path, "/") == 0 || strcmp(path, "/NAMES") == 0 || strcmp(path, "/CODES") == 0) {
		stbuf->st_mode = S_IFDIR | 0775;
		stbuf->st_nlink = 2;
		stbuf->st_size = 4096;
	}
	else if (strncmp(path+1, NAMES, 5) == 0) {
		char *arr[3]; 
		int c = split_path(arr, path+7);
		free(*arr);
		free(arr[1]);
		free(arr[2]);
		printf("split_path: %d\n", c);
		if (c == 3)
			stbuf->st_mode = S_IFREG | 0644;
		else if (c < 3)
			stbuf->st_mode = S_IFDIR | 0755;
		else
			return -ENOENT;
		stbuf->st_nlink = 1;
		stbuf->st_size = 1000;
	}
	else if (strncmp(path+1, CODES, 5) == 0) {
		char *arr[3]; 
		int c = split_path(arr, path+7);
		free(*arr);
		free(arr[1]);
		free(arr[2]);
		printf("splitted to %d\n", c);
		if (c == 2)
			stbuf->st_mode = S_IFREG | 0644;
		else if (c < 2)
			stbuf->st_mode = S_IFDIR | 0755;
		else
			return -ENOENT;
		stbuf->st_nlink = 1;
		stbuf->st_size = 1000;
	}
	else {
		return -ENOENT;
	}
	return 0;
}

static int postal_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
	printf(">> read\n");
	
	char *arr[3];
	int which = split_path(arr, path+7);
	int is_name = (strncmp(path+1, NAMES, 5) == 0);
	int is_code = (strncmp(path+1, CODES, 5) == 0);

	printf("which: %d\n", which);

	if ( which == 3 && is_name ) {
		char **result;
		int c = find(&result, arr[0], arr[1], arr[2], is_name, which);
		
		printf("find returned: %d\n", c);

		strcpy(buf, *result);

		printf("res: %s\n", buf);
		
		return strlen(*result);
	}
	else if( which == 2 && is_code ) {
		printf("code\n\n");
		char **result;
		int c = find(&result, arr[0], arr[1], arr[2], is_name, which);
		
		printf("find returned: %d\n", c);

		strcpy(buf, *result);

		printf("res: %s\n", buf);
		
		return strlen(*result);
	}

	return 0;
}

static int postal_unlink(const char *path){
	printf(">> unlink\n");
	
	char *arr[3];
	int which = split_path(arr, path+7);
	int is_name = (strncmp(path+1, NAMES, 5) == 0);
	int is_code = (strncmp(path+1, CODES, 5) == 0);
	
	if ((which == 3 && is_name) ||  (which == 2 && is_code)) {
		char **result;
		int c = find(&result, arr[0], arr[1], arr[2], is_name, which);
				
		free(arr[0]);
		free(arr[1]);
		free(arr[2]);
		
		FILE *csvfile = fopen("postal-codes.csv", "rb+");
		FILE *tmp = fopen("postal-codes-tmp.csv", "wb");	//geçiçi dosya

		//int lineLen = strlen(renameLine.code) + strlen(renameLine.neighborhood) + strlen(renameLine.city) + strlen(renameLine.district) + strlen(renameLine.latitude) + strlen(renameLine.longtitude) + 6;
		
		while(csvgetline(csvfile) != NULL){
			if(renamePos != ftell(csvfile)){	//silinmek istenen satırın başına gelince o satırı yazma atla
				fprintf(tmp, "%s\n", line);
			}
		}
		
		renamePos = -1;
		
		fclose(csvfile);
		fclose(tmp);
		
		remove("postal-codes.csv");
		rename("postal-codes-tmp.csv", "postal-codes.csv");

		return 0;
	}
	
	return -1;
}

static int postal_rename(const char *from, const char *to){
	printf(">> rename\n");
	
	char *arr[3];
	int which = split_path(arr, from+7);
	int is_name = (strncmp(from+1, NAMES, 5) == 0);
	int is_code = (strncmp(from+1, CODES, 5) == 0);

	char *arrTo[3];
	int whichTo = split_path(arrTo, to+7);
	
	if ((which == 3 && is_name) ||  (which == 2 && is_code)) {
		char **result;
		int c = find(&result, arr[0], arr[1], arr[2], is_name, which);
		
		free(arr[0]);
		free(arr[1]);
		free(arr[2]);
		
		printf("RENAME: find returned: %d\n", c);
		printf("RENAME: pos: %d\n", renamePos);
		
		FILE *csvfile = fopen("postal-codes.csv", "rb+");
		FILE *tmp = fopen("postal-codes-tmp.csv", "wb");

		//int lineLen = strlen(renameLine.code) + strlen(renameLine.neighborhood) + strlen(renameLine.city) + strlen(renameLine.district) + strlen(renameLine.latitude) + strlen(renameLine.longtitude) + 6;
		
		while(csvgetline(csvfile) != NULL){
			if(renamePos == ftell(csvfile)){	//değiştirilmek istenen satıra gelince yeni satırı yaz
				if(which == 3 && is_name){
					fprintf(tmp, "%s\t%s\t%s\t%s\t%s\t%s\n", renameLine.code, arrTo[2], renameLine.city, renameLine.district, renameLine.latitude, renameLine.longtitude);
				}
				else if( which == 2 && is_code ){
					char *flag = "0";
					while(strcmp(arr[1], csvfield(0)) != 0 && flag != NULL){
						flag = csvgetline(csvfile);
						if(flag == NULL){
							break;
						}
						fprintf(tmp, "%s\n", line);
					}
					fprintf(tmp, "%s\t%s\t%s\t%s\t%s\t%s\n", arrTo[1], renameLine.neighborhood, renameLine.city, renameLine.district, renameLine.latitude, renameLine.longtitude);
				}
			}
			else{
				fprintf(tmp, "%s\n", line);
			}
		}
		
		renamePos = -1;
		
		fclose(csvfile);
		fclose(tmp);
		
		remove("postal-codes.csv");
		rename("postal-codes-tmp.csv", "postal-codes.csv");

		return 0;
	}
	
	return -1;
}

static struct fuse_operations postal_oper = {
	.readdir = postal_readdir, /* directory listing - ls - filler komutuyla buffer doldurma*/
	.getattr = postal_getattr, /* reading file attributes - size, link, mode - struct stat*/
	.read = postal_read, /* reading a file - dosya içeriğinin buffera kopyalandığı yer */
	.unlink = postal_unlink, /* deleting a file */
	.rename = postal_rename, /* renaming a file */
};

int main(int argc, char *argv[]) {

	return fuse_main(argc, argv, &postal_oper, NULL);
}
