#include "sp_image_proc_util.h"
#include <stdio.h>
#include <stdlib.h>
//TODO - strings messages should be macros

typedef struct settings{
	int numOfBins;
	int numOfFeatures;
	int numOfImages;
	char** filePathsArray;
} settings;

void getAsPositiveInt(char* message_type, int* value)
{
	printf("Enter number of %s\n",message_type);
		scanf("%d", value);
		if (*value < 1) {
			printf("An error occurred - invalid number of %s\n",message_type);
			exit(0); //TODO - verify we should exit like this
		}
}

void generateFileNames(char* folderpath,char* image_prefix,char* image_suffix,settings* allSettings){
	allSettings->filePathsArray = (char**)malloc(sizeof(char *)*allSettings->numOfImages);
	char file_path[1024];
	for (int i =0 ; i< allSettings->numOfImages;i++){
		sprintf(file_path,"%s/%s%d%s",folderpath,image_prefix,i,image_suffix);
		allSettings->filePathsArray[i] = file_path;
	}
}

settings* setInput()
{
	settings* allSettings = (settings*)malloc(sizeof(settings));
	char folderpath[1024]; char image_prefix[1024]; char image_suffix[1024];

	//Handle input
	printf("Enter images directory path:\n");
	scanf("%s", folderpath);

	printf("Enter images prefix:\n");
	scanf("%s", image_prefix);

	getAsPositiveInt("images",&(allSettings->numOfImages));

	printf("Enter images suffix:\n");
	scanf("%s", image_suffix);

	getAsPositiveInt("bins",&(allSettings->numOfBins));
	getAsPositiveInt("features",&(allSettings->numOfFeatures));

	//generate files names
	generateFileNames(folderpath, image_prefix, image_suffix, allSettings);

	return allSettings;
}

void start(){
	settings* allSettings = setInput();

}





