#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <linux/limits.h>
#include <limits.h>
#include <unistd.h>

using namespace std;

bool u = false;
bool g = false;
bool o = false;
bool plusbit = false;
bool minusbit = false;
bool r = false;
bool w = false;
bool x = false;

bool absolute = false;

DIR* d;

char* filename;

struct stat finfo;

mode_t permission = 0;

void parsePermissions(string permissions){
	size_t plusminus;
	
	//set plus/minus
	if((plusminus=permissions.find_first_of("+-"))==string::npos){
		perror("Permissions string must have +/-");
		exit(-4);
	}
	if(plusminus==0){
		perror("Permissions string must start with [ugo]+");
		exit(-4);
	}
	if(plusminus==permissions.length()-1){
		perror("Permissions string must end with [rwx]+");
		exit(-4);
	}
	if(permissions[plusminus]=='+'){
		plusbit = true;
	}
	else{
		minusbit = true;
	}

	//set ugo
	for(unsigned int i = 0; i<plusminus;i++){
		if(permissions[i] == 'u'){
			u=true;
		}
		else if(permissions[i] == 'g'){
			g=true;
		}
		else if(permissions[i] == 'o'){
			o=true;
		}
		else{
			perror("Permissions string must start with [ugo]+");
			exit(-4);
		}
	}
	
	//set rwx
	for(unsigned int i = plusminus+1; i<permissions.length(); i++){
		if(permissions[i] == 'r'){
			r=true;
		}
		else if(permissions[i] == 'w'){
			w=true;
		}
		else if(permissions[i] == 'x'){
			x=true;
		}
		else{
			perror("Permissions string must have [rwx]+ after the +/-");
			exit(-4);
		}
	}
}

void setPermissions(char* file){
	if(stat(file, &finfo)==0){
		if(u){
			if(r){
				permission = (permission | S_IRUSR);
			}
			if(w){
				permission = (permission | S_IWUSR);
			}
			if(x){
				permission = (permission | S_IXUSR);
			}
		}
		if(g){
                	if(r){
				permission = (permission | S_IRGRP);
                        }
                        if(w){
				permission = (permission | S_IWGRP);
                        }
                        if(x){
				permission = (permission | S_IXGRP);
                        }
		}
		if(o){
                        if(r){
				permission = (permission | S_IROTH);
                        }
                        if(w){
				permission = (permission | S_IWOTH);
                        }
                        if(x){
				permission = (permission | S_IXOTH);
                        }			
		}
		if(plusbit){
			permission = (permission | finfo.st_mode);
		}
		else if(minusbit){
			permission = ~permission;
			permission = (permission & finfo.st_mode);
		}
	}
	else{
		perror("Couldnt get current mode of file");
		exit(-1);
	}
}

void recursively(char* path, mode_t mode){
	struct stat finfo;
	
	DIR *d;
	
	struct dirent *de;
	
	d=opendir(path);

	if(d == NULL) { perror("Couldn't open directory"); exit(1); }

	if(chmod(path, mode)!=0){
		perror("Could not change mode");
		exit(-1);
	}

	for(de = readdir(d); de != NULL; de = readdir(d)){
		if(string(de->d_name)!="." && string(de->d_name)!=".."){	
			string s = string(path) + "/" + string(de->d_name);
		
			if(lstat(s.c_str(), &finfo) == 0){
				if(S_ISDIR(finfo.st_mode)){
					recursively((char *) s.c_str(), mode);
				}
				else{
					if(chmod((char *) s.c_str(), mode)!=0){
                                                perror("Could not change mode");
                                                exit(-3);
                                        }
				}
			}
		}
	}
}

bool isAbsoluteMode(string s){
	//Check to see if we are dealing with a mode
        if(isdigit(s[0])){
        	//Make sure there's exactly 3 bits
                	if(s.length()!=3){
                        	perror("mode must be exactly 3 numbers");
                                exit(-2);
                        }
                        if(!(isdigit(s[1]) && isdigit(s[2]))){
                                perror("mode must be 3 digits");
                                exit(-2);
                        }
                        //Make sure bits are between 0 and 7
                        if((s[0]-'0') > 7 || (s[1]-'0') > 7 || (s[2]-'0') > 7){
                                perror("mode bits must be 0-7");
                                exit(-2);
                        }
                        else if((s[0]-'0') < 0 || (s[1]-'0') < 0 || (s[2]-'0') < 0){
                                perror("mode bits must be 0-7");
                                exit(-2);
                        }

			return true;
        }
	else{
		return false;
	}

}

int main(int argc, char *argv[]){
	if(argc==4){
		if(string(argv[1])=="-R"){
			if(stat(argv[3], &finfo)!=0){
				perror("Could not open file/directory");
				exit(-1);
			}
			string s = string(argv[2]);
			filename=argv[3];

			//Check if it's a directory
			if(S_ISDIR(finfo.st_mode)){
                                if(isAbsoluteMode(s)){
                                        mode_t mode = strtol(s.c_str(),NULL,8);

                                        recursively(argv[3], mode);
                                }
				else{
					parsePermissions(s);
					setPermissions(argv[3]);
					
					string path = string(argv[3]);

					//If path ends in /, drop the / (for formatting)
        				if(path[path.size()-1]=='/'){
                				path = path.substr(0, path.size()-1);
        				}	
			
					recursively((char*)path.c_str(), permission);
				}
				//printf("It's a directory with r\n");
			}
			//Check if it's a file
			else{
				if(isAbsoluteMode(s)){
					mode_t mode = strtol(s.c_str(),NULL,8);

					if(chmod(filename, mode)!=0){
						perror("could not set mode");
						exit(-3);
					}
				}
				//if it's not absolute, must be relative
				else{
					parsePermissions(s);
					setPermissions(argv[3]);
					if(chmod(argv[3], permission)!=0){
						perror("could not set mode");
						exit(-3);
					}
				}
			}
				
		}
		else{
                	perror("USAGE: chmod [-R] PERMISSIONS FILE");
               		exit(-1);
		}	
	}
	else if(argc==3){
		//Check if it exists
		if(stat(argv[2], &finfo)==0){
			filename=argv[2];
			string s = string(argv[1]);
			//check if the mode is absolute
			if(isAbsoluteMode(s)){
				mode_t mode = strtol(s.c_str(),NULL,8);
				
				if(chmod(filename, mode)!=0){
					perror("could not set mode");
					exit(-3);
				}
			}
			//If it's not absolute, parse for relative
			else{
				parsePermissions(s);
				setPermissions(argv[2]);
                                if(chmod(argv[2], permission)!=0){
                                        perror("could not set mode");
                                        exit(-3);
                                }

			}
		}
		else{
			perror("Could not open file/directory");
			exit(-1);
		}
	}
	else{
		perror("USAGE: chmod [-R] PERMISSIONS FILE");
		exit(-1);
	}
}
