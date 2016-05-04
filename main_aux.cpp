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
#define ALLOCATION_ERROR_MESSAGE "An error occurred - allocation failure\n"

settings* allSettings = NULL;

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
void clearSettings()
{
	int i;
	if (allSettings != NULL)
	{
		if (allSettings->filePathsArray != NULL)
		{
			for (i=0;i<allSettings->numOfImages;i++)
			{
				free(allSettings->filePathsArray[i]);
			}
			free(allSettings->filePathsArray);
		}
		free(allSettings);
	}
}

void reportErrorAndExit(const char* message)
{
	printf(message);
	fflush(NULL);
	if (allSettings != NULL)
		clearSettings();
	exit(-1); //TODO - check what error code should we return
}

void reportAllocationErrorAndExit()
{
	reportErrorAndExit(ALLOCATION_ERROR_MESSAGE);
}

void getAsPositiveInt(const char* message_type, int* value)
{
	printf(ENTER_NUMBER_OF, message_type);
	fflush(NULL);
	scanf("%d", value);
	fflush(NULL);
	if (*value < 1) {
		reportErrorAndExit(ERROR_MESSAGE);
	}
}

void generateFileNames(char* folderpath,char* image_prefix,char* image_suffix){
	allSettings->filePathsArray = (char**)calloc(allSettings->numOfImages, sizeof(char *));
	char* file_path;

	if (allSettings->filePathsArray == NULL)
		reportAllocationErrorAndExit();

	for (int i =0 ; i< allSettings->numOfImages;i++){
		file_path = (char*)calloc(MAX_IMAGE_PATH_LENGTH, sizeof(char));
		if (file_path == NULL)
				reportAllocationErrorAndExit();
		sprintf(file_path,"%s%s%d%s",folderpath,image_prefix,i,image_suffix);
		fflush(NULL);
		allSettings->filePathsArray[i] = file_path;
	}
}

imageData* calcImageData(char* filePath, int nBins, int maxNumFeatures)
{
	imageData* data = (imageData*)malloc(sizeof(imageData));
	if (data == NULL)
			reportAllocationErrorAndExit();
	data->rgbHist = spGetRGBHist(filePath, nBins);
	data->siftDesc = spGetSiftDescriptors(filePath,maxNumFeatures,&(data->nFeatures));
	return data;
}

//This method calculates the histograms and the sift descriptors
imageData** calcHistAndSIFTDesc()
{
	int i;
	imageData** data = (imageData**)calloc(allSettings->numOfImages, sizeof(imageData*));
	if (data == NULL)
			reportAllocationErrorAndExit();
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
	if (allSettings == NULL)
			reportAllocationErrorAndExit();
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

void importSorted(int index, double distance, keyValue** items)
{
	int limit = index, j, k;
	keyValue* current = (keyValue*)malloc(sizeof(keyValue));
	if (current == NULL)
		reportAllocationErrorAndExit();
	current->key = index;
	current->value = distance;
	keyValue *temp1, *temp2;

	//used to handle the case that we haven't inserted 5 items yet
	if (limit > NUM_OF_BEST_DIST_IMGS)
		limit = NUM_OF_BEST_DIST_IMGS;

	//find where (if needed) should we insert the current item
	for (j=0; j < limit && distance > items[j]->value ;j++);

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
		//clear memory to the last item
		free(temp1);
	}
	else
	{
		//clear memory of the unused item
		free(current);
	}
}

void compareGlobalFeatures(imageData** imagesData,imageData* workingImageData )
{
	int i;
	keyValue* topItems[NUM_OF_BEST_DIST_IMGS];
	for (i=0;i<allSettings->numOfImages;i++)
	{
		importSorted(i,spRGBHistL2Distance(imagesData[i]->rgbHist,
				workingImageData->rgbHist,allSettings->numOfBins) ,topItems);
	}
	//print items
	printf(NEAREST_IMAGES_USING_GLOBAL_DESCRIPTORS);
	fflush(NULL);

	for (int i=0; i<NUM_OF_BEST_DIST_IMGS ;i++)
	{
		printf("%d%s",topItems[i]->key,i == (NUM_OF_BEST_DIST_IMGS-1) ? "\n" : ", ");
		fflush(NULL);
		free(topItems[i]);
	}
}

void compareLocalFeatures(imageData** imagesData,imageData* workingImageData)
{
	//step 0 - create an index-counter array for the images
	int *counterArray = (int*)calloc(allSettings->numOfImages, sizeof(int));
	if (counterArray == NULL)
		reportAllocationErrorAndExit();
	//step 1 - create the database object and the num of features array
	double*** databaseFeatures = (double***)calloc(allSettings->numOfImages, sizeof(double**));
	if (databaseFeatures == NULL)
		reportAllocationErrorAndExit();
	int *featuresPerImage=(int*)calloc(allSettings->numOfImages, sizeof(int)), *resultsArray;
	if (featuresPerImage == NULL)
		reportAllocationErrorAndExit();

	int i,j, tempMaxIndex;

	for (i=0;i<allSettings->numOfImages;i++)
	{
		counterArray[i] = 0;
		databaseFeatures[i] = imagesData[i]->siftDesc;
		featuresPerImage[i] = imagesData[i]->nFeatures;
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

	// step 3 - sort the index counter array and get the best 5

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

	for (i=0; i<NUM_OF_BEST_DIST_IMGS ;i++)
	{
		printf("%d%s",topItems[i],i == (NUM_OF_BEST_DIST_IMGS-1) ? "\n" : ", ");
		fflush(NULL);
	}

	//free data


	if (featuresPerImage != NULL)
		free(featuresPerImage);
	if (databaseFeatures != NULL)
	{
		for (i=0;i<allSettings->numOfImages;i++)
		{
			//TODO - this is not good check if realy nesseccery
			free(databaseFeatures[i]);
		}
		free(databaseFeatures);
	}
	if (counterArray != NULL)
		free(counterArray);

}

void freeImageData(imageData* image)
{
	int i;
	if (image != NULL)
	{
		if (image->rgbHist != NULL)
		{
			for (i=0;i<3;i++)
			{
				free(image->rgbHist[i]);
			}
			free(image->rgbHist);
		}
		if (image->siftDesc != NULL)
		{
			for (i=0;i<image->nFeatures;i++)
			{
				free(image->siftDesc[i]);
			}
			free(image->siftDesc);
		}
		free(image);
	}
}

void freeImages(imageData** images)
{
	int i;
	if (images != NULL)
	{
		for (i=0;i<allSettings->numOfImages;i++)
		{
			freeImageData(images[i]);
		}
	}
	free(images);
}
void searchSimilarImages(imageData** imagesData,char* imagePath)
{
	imageData* workingImageData = calcImageData(imagePath, allSettings->numOfBins, allSettings->numOfFeatures);
	compareGlobalFeatures(imagesData, workingImageData);
	compareLocalFeatures(imagesData, workingImageData);

	freeImageData(workingImageData);

}

void startInteraction(imageData** imagesData)
{
	char workingImagePath[MAX_IMAGE_PATH_LENGTH];
	printf(ENTER_A_QUERY_IMAGE_OR_TO_TERMINATE);
	fflush(NULL);
	scanf("%s",workingImagePath);
	fflush(NULL);
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
	imageData** imagesData = calcHistAndSIFTDesc();
	startInteraction(imagesData);

	freeImages(imagesData);
	clearSettings();
}
