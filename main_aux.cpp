#include "sp_image_proc_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//TODO - strings messages should be macros
//TODO - maybe we should make allSettings a global pointer

typedef struct settings{
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
} keyValue;

void getAsPositiveInt(char* message_type, int* value)
{
	printf("Enter number of %s:\n",message_type);
	fflush(NULL);
	scanf("%d", value);
	fflush(NULL);
	if (*value < 1) {
		printf("An error occurred - invalid number of %s\n",message_type);
		fflush(NULL);
		exit(0); //TODO - verify we should exit like this
	}
}

void generateFileNames(char* folderpath,char* image_prefix,char* image_suffix,settings* allSettings){
	allSettings->filePathsArray = (char**)malloc(sizeof(char *)*allSettings->numOfImages);
	char* file_path;
	for (int i =0 ; i< allSettings->numOfImages;i++){
		file_path = (char*)malloc(1024*sizeof(char));
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
imageData* calcHistAndSIFTDesc(settings* allSettings)
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


settings* testSetInput()
{
	settings* allSettings = (settings*)malloc(sizeof(settings));
	char* folderpath = "./images/"; char* image_prefix = "img"; char* image_suffix = ".png";
	allSettings->numOfImages = 17;
	allSettings->numOfBins = 16;
	allSettings->numOfFeatures = 100;

	//generate files names
	generateFileNames(folderpath, image_prefix, image_suffix, allSettings);

	return allSettings;
}


settings* setInput()
{
	settings* allSettings = (settings*)malloc(sizeof(settings));
	char folderpath[1024]; char image_prefix[1024]; char image_suffix[1024];

	//Handle input
	printf("Enter images directory path:\n");
	fflush(NULL);
	scanf("%s", folderpath);
	fflush(NULL);

	printf("Enter images prefix:\n");
	fflush(NULL);
	scanf("%s", image_prefix);
	fflush(NULL);

	getAsPositiveInt("images",&(allSettings->numOfImages));

	printf("Enter images suffix:\n");
	fflush(NULL);
	scanf("%s", image_suffix);
	fflush(NULL);

	getAsPositiveInt("bins",&(allSettings->numOfBins));
	getAsPositiveInt("features",&(allSettings->numOfFeatures));

	//generate files names
	generateFileNames(folderpath, image_prefix, image_suffix, allSettings);

	return allSettings;
}

void importSorted(int index, double distance, keyValue* items)
{
	int limit = index, j, k;
	keyValue current = {index, distance};
	keyValue temp1,temp2;

	//used to handle the case that we haven't inserted 5 items yet
	if (limit > 5)
		limit = 5;

	//find where (if needed) should we insert the current item
	for (j=0; j < limit && distance > items[j].value ;j++);

	if (limit < 5)
		limit++;

	if (j < 5) //push at j
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

void printItems(keyValue* items){
	printf("Array items : \n");
	fflush(NULL);
	for (int i=0;i<5;i++)
	{
		printf("[%d]: %f | ",items[i].key,items[i].value);
		fflush(NULL);
	}
	printf("\n");
	fflush(NULL);
}

void compareGlobalFeatures(settings* allSettings, imageData** imagesData,imageData* workingImageData )
{
	int i;
	//keyValue* topFiveItems = (keyValue*)malloc(5*sizeof(keyValue));
	//TODO - if this work, delete the prev line
	keyValue topFiveItems[5];
	for (i=0;i<allSettings->numOfImages;i++)
	{
		double value = spRGBHistL2Distance((*imagesData)[i].rgbHist,
				workingImageData->rgbHist,allSettings->numOfBins);
		printItems(topFiveItems);
		printf("Pushing item %d with value %f:\n",i,value);
		fflush(NULL);
		importSorted(i,value
				,topFiveItems);
		printItems(topFiveItems);
	}
	//print items
	printf("Nearest images using global descriptors:\n");
	fflush(NULL);
	//TODO - can we assume that we have 5 items???
	printf("%d, %d, %d, %d, %d\n",topFiveItems[0].key,topFiveItems[1].key,topFiveItems[2].key,topFiveItems[3].key,topFiveItems[4].key);
	fflush(NULL);
}

void compareLocalFeatures(settings* allSettings, imageData** imagesData,imageData* workingImageData)
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
		resultsArray = spBestSIFTL2SquaredDistance(5, workingImageData->siftDesc[i], databaseFeatures,
				allSettings->numOfImages, featuresPerImage);
		for (j=0; j<5 ; j++)
		{
			counterArray[resultsArray[j]]++;
		}
	}

	// step 3 - sort the index counter array and get the best 5 (print the fuckers)

	//TODO - verify that we can assume that we have at least 1 image

	int topItems[5];
	for (j=0;j<5;j++)
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
	printf("Nearest images using local descriptors:\n");
	fflush(NULL);
	printf("%d, %d, %d, %d, %d\n", topItems[0], topItems[1], topItems[2], topItems[3], topItems[4]);
	fflush(NULL);
}

void searchSimilarImages(settings* allSettings, imageData** imagesData,char* imagePath)
{
	imageData workingImageData = calcImageData(imagePath, allSettings->numOfBins, allSettings->numOfFeatures);
	compareGlobalFeatures(allSettings,imagesData,&workingImageData);
	compareLocalFeatures(allSettings,imagesData,&workingImageData);
}

void startInteraction(settings* allSettings, imageData** imagesData)
{
	char workingImagePath[1024];
	printf("Enter a query image or # to terminate:\n");
	fflush(NULL);
	scanf("%s",workingImagePath);
	fflush(NULL);
	//TODO - verify that we wont fail on the first run since workingImagePath is null
	while (strcmp(workingImagePath,"#"))
	{
		searchSimilarImages(allSettings,imagesData,workingImagePath);
		printf("Enter a query image or # to terminate:\n");
		fflush(NULL);
		scanf("%s",workingImagePath);
		fflush(NULL);
	}
	printf("Exiting...\n");
	fflush(NULL);
}

void start(){
	//settings* allSettings = setInput();
	settings* allSettings = testSetInput();
	imageData* imagesData = calcHistAndSIFTDesc(allSettings);
	//TODO - THINK CAREFULLY ABOUT CLEANUP !!!! BITCH
	startInteraction(allSettings, &imagesData);
	// call clean all
}
