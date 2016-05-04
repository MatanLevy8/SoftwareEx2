#include "sp_image_proc_util.h"
#include "tests.h" //TODO - DONT FORGET TO DELETE THIS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXITING "Exiting...\n"
#define ENTER_A_QUERY_IMAGE_OR_TO_TERMINATE "Enter a query image or # to terminate:\n"
#define NEAREST_IMAGES_USING_LOCAL_DESCRIPTORS "Nearest images using local descriptors:\n"
#define NEAREST_IMAGES_USING_GLOBAL_DESCRIPTORS "Nearest images using global descriptors:\n"
#define FEATURES "features"
#define BINS "bins"
#define IMAGES "images"
#define ENTER_IMAGES_PREFIX "Enter images prefix:\n"
#define ENTER_IMAGES_DIRECTORY_PATH "Enter images directory path:\n"
#define ERROR_MESSAGE "An error occurred - invalid number of %s\n"
#define ENTER_NUMBER_OF "Enter number of %s:\n"
#define ENTER_IMAGES_SUFFIX "Enter images suffix:\n"
#define MAX_IMAGE_PATH_LENGTH  1024
#define NUM_OF_BEST_DIST_IMGS 5

settings* allSettings = NULL;

//TODO - strings messages should be macros
//TODO - maybe we should make allSettings a global pointer

/*typedef struct settings{
	int numOfBins;
	int numOfFeatures;
	int numOfImages;
	char** filePathsArray;
} settings;

typedef struct imageData{
	int** rgbHist;
	double** siftDesc;
	int nFeatures; //TODO - check maybe we should change to just int
} imageData;

typedef struct keyValue{
	int key;
	double value;
} keyValue;*/

void getAsPositiveInt(const char* message_type, int* value)
{
	printf(ENTER_NUMBER_OF, message_type);
	fflush(NULL);
	scanf("%d", value);
	fflush(NULL);
	if (*value < 1) {
		printf(ERROR_MESSAGE, message_type);
		fflush(NULL);
		exit(0); //TODO - verify we should exit like this
	}
}

void generateFileNames(char* folderpath,char* image_prefix,char* image_suffix){
	allSettings->filePathsArray = (char**)malloc(sizeof(char *)*allSettings->numOfImages);
	char* file_path;
	for (int i =0 ; i< allSettings->numOfImages;i++){
		file_path = (char*) malloc(MAX_IMAGE_PATH_LENGTH * sizeof(char));
		sprintf(file_path,"%s%s%d%s",folderpath,image_prefix,i,image_suffix);
		fflush(NULL);
		allSettings->filePathsArray[i] = file_path;
	}
}

imageData calcImageData(char* filePath, int nBins, int maxNumFeatures)
{
	imageData data;
	data.rgbHist = spGetRGBHist(filePath, nBins);
	data.siftDesc = spGetSiftDescriptors(filePath,maxNumFeatures,&data.nFeatures);
	return data;
}

//This method calculates the histograms and the sift descriptors
imageData* calcHistAndSIFTDesc()
{
	int i;
	imageData* data = (imageData*)calloc(allSettings->numOfImages, sizeof(imageData));
	for (i=0 ; i<allSettings->numOfImages ; i++)
	{
		data[i] = calcImageData(allSettings->filePathsArray[i],
								allSettings->numOfBins,
								allSettings->numOfFeatures);
	}
	return data;
}

void setInput()
{
	allSettings = (settings*)malloc(sizeof(settings));
	char folderpath[MAX_IMAGE_PATH_LENGTH];
	char image_prefix[MAX_IMAGE_PATH_LENGTH];
	char image_suffix[MAX_IMAGE_PATH_LENGTH];

	//Handle input
	printf(ENTER_IMAGES_DIRECTORY_PATH);
	fflush(NULL);
	scanf("%s", folderpath);
	fflush(NULL);

	printf(ENTER_IMAGES_PREFIX);
	fflush(NULL);
	scanf("%s", image_prefix);
	fflush(NULL);

	getAsPositiveInt(IMAGES, &(allSettings->numOfImages));

	printf(ENTER_IMAGES_SUFFIX);
	fflush(NULL);
	scanf("%s", image_suffix);
	fflush(NULL);

	getAsPositiveInt(BINS, &(allSettings->numOfBins));
	getAsPositiveInt(FEATURES, &(allSettings->numOfFeatures));

	//generate files names
	generateFileNames(folderpath, image_prefix, image_suffix);
}

void importSorted(int index, double distance, keyValue* items)
{
	int limit = index, j, k;
	keyValue current = {index, distance};
	keyValue temp1,temp2;

	//used to handle the case that we haven't inserted 5 items yet
	if (limit > NUM_OF_BEST_DIST_IMGS)
		limit = NUM_OF_BEST_DIST_IMGS;

	//find where (if needed) should we insert the current item
	for (j=0; j < limit && distance > items[j].value ;j++);

	if (limit < NUM_OF_BEST_DIST_IMGS)
		limit++;

	if (j < NUM_OF_BEST_DIST_IMGS) //push at j
	{
		//push new item
		temp1 = items[j];
		items[j] = current;

		//shift rest of the items
		for (k = j+1; k < limit ;k++)
		{
			temp2 = items[k];
			items[k] = temp1;
			temp1 = temp2;
		}
	}
}

void compareGlobalFeatures(imageData** imagesData,imageData* workingImageData )
{
	int i;
	//keyValue* topFiveItems = (keyValue*)malloc(5*sizeof(keyValue));
	//TODO - if this work, delete the prev line
	keyValue topItems[NUM_OF_BEST_DIST_IMGS];
	for (i=0;i<allSettings->numOfImages;i++)
	{
		importSorted(i,spRGBHistL2Distance((*imagesData)[i].rgbHist,
				workingImageData->rgbHist,allSettings->numOfBins) ,topItems);
	}
	//print items
	printf(NEAREST_IMAGES_USING_GLOBAL_DESCRIPTORS);
	fflush(NULL);
	//TODO - can we assume that we have 5 items???

	for (int i=0; i<NUM_OF_BEST_DIST_IMGS ;i++)
	{
		printf("%d%s",topItems[i].key,i == (NUM_OF_BEST_DIST_IMGS-1) ? "\n" : ", ");
		fflush(NULL);
	}
}

void compareLocalFeatures(imageData** imagesData,imageData* workingImageData)
{
	//step 0 - create an index-counter array for the images
	int *counterArray = (int*)malloc(sizeof(int)*(allSettings->numOfImages));
	//step 1 - create the database object and the num of features array
	double*** databaseFeatures = (double***)malloc(sizeof(double**)*(allSettings->numOfImages));
	int *featuresPerImage=(int*)malloc(sizeof(int)*(allSettings->numOfImages)), *resultsArray;
	int i,j, tempMaxIndex;

	for (i=0;i<allSettings->numOfImages;i++)
	{
		counterArray[i] = 0;
		databaseFeatures[i] = (*imagesData)[i].siftDesc;
		featuresPerImage[i] = (*imagesData)[i].nFeatures;
	}

	//step 2 - for each feature in the working image send a request to compare the best 5
	// for each 5 returned increase a counter at the relevant index
	for (i=0; i<workingImageData->nFeatures;i++)
	{
		resultsArray = spBestSIFTL2SquaredDistance(NUM_OF_BEST_DIST_IMGS, workingImageData->siftDesc[i], databaseFeatures,
				allSettings->numOfImages, featuresPerImage);
		for (j=0; j<NUM_OF_BEST_DIST_IMGS ; j++)
		{
			counterArray[resultsArray[j]]++;
		}
	}

	// step 3 - sort the index counter array and get the best 5 (print the fuckers)

	//TODO - verify that we can assume that we have at least 1 image

	int topItems[NUM_OF_BEST_DIST_IMGS];
	for (j=0;j<NUM_OF_BEST_DIST_IMGS;j++)
	{
		tempMaxIndex = 0;
		for (i=0;i<allSettings->numOfImages;i++)
		{
			if (counterArray[i] > counterArray[tempMaxIndex])
			{
				tempMaxIndex = i;
			}
		}
		topItems[j] = tempMaxIndex;
		counterArray[tempMaxIndex] = -1;
	}
	printf(NEAREST_IMAGES_USING_LOCAL_DESCRIPTORS);
	fflush(NULL);

	for (int i=0; i<NUM_OF_BEST_DIST_IMGS ;i++)
	{
		printf("%d%s",topItems[i],i == (NUM_OF_BEST_DIST_IMGS-1) ? "\n" : ", ");
		fflush(NULL);
	}
}

void searchSimilarImages(imageData** imagesData,char* imagePath)
{
	imageData workingImageData = calcImageData(imagePath, allSettings->numOfBins, allSettings->numOfFeatures);
	compareGlobalFeatures(imagesData,&workingImageData);
	compareLocalFeatures(imagesData,&workingImageData);
}

void startInteraction(imageData** imagesData)
{
	char workingImagePath[MAX_IMAGE_PATH_LENGTH];
	printf(ENTER_A_QUERY_IMAGE_OR_TO_TERMINATE);
	fflush(NULL);
	scanf("%s",workingImagePath);
	fflush(NULL);
	//TODO - verify that we wont fail on the first run since workingImagePath is null
	while (strcmp(workingImagePath,"#"))
	{
		searchSimilarImages(imagesData,workingImagePath);
		printf(ENTER_A_QUERY_IMAGE_OR_TO_TERMINATE);
		fflush(NULL);
		scanf("%s",workingImagePath);
		fflush(NULL);
	}
	printf(EXITING);
	fflush(NULL);
}

void start(){
	setInput();
	//testSetInput(allSettings);
	imageData* imagesData = calcHistAndSIFTDesc();
	//TODO - THINK CAREFULLY ABOUT CLEANUP !!!! BITCH
	startInteraction(&imagesData);
	// call clean all
}
