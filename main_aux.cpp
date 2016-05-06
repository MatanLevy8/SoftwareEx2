#include "sp_image_proc_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG //TODO - comment this line for production mode

#ifdef DEBUG
#include "tests.h"
#endif

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
#define IMAGE_PROC_ERROR_MESSAGE "An error occurred - failed to process image\n"
#define IMAGE_COMPARE_ERROR_MESSAGE "An error occurred - failed to compare images\n"

//TODO - move structures definitions to this file

//shared variables
settings* allSettings = NULL;
imagesDatabase* allImagesDatabase = NULL;
imageData** imagesData = NULL;

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
	#ifdef DEBUG
		fflush(NULL);
	#endif
	if (allSettings != NULL)
		clearSettings();
	exit(-1); //TODO - check what error code should we return
}

void reportAllocationErrorAndExit()
{
	reportErrorAndExit(ALLOCATION_ERROR_MESSAGE);
}

void* safeCalloc(int itemsCount, int sizeOfItem)
{
	void* memoryPointer = calloc(itemsCount, sizeOfItem);
	if (memoryPointer == NULL) //allocation failed
		reportAllocationErrorAndExit();
	return memoryPointer;
}

void setImagesDatabase()
{
	int i;
	allImagesDatabase = (imagesDatabase*)safeCalloc(1, sizeof(imagesDatabase));

	//step 1 - create the database and the number of features array
	allImagesDatabase->databaseFeatures = (double***)safeCalloc(allSettings->numOfImages, sizeof(double**));

	allImagesDatabase->featuresPerImage=(int*)safeCalloc(allSettings->numOfImages, sizeof(int));

	//allocate the database features and number of features
	for (i=0;i<allSettings->numOfImages;i++)
	{
		allImagesDatabase->databaseFeatures[i] = imagesData[i]->siftDesc;
		allImagesDatabase->featuresPerImage[i] = imagesData[i]->nFeatures;
	}
}

void getAsPositiveInt(const char* message_type, int* value)
{
	printf(ENTER_NUMBER_OF, message_type);
	#ifdef DEBUG
		fflush(NULL);
	#endif
	scanf("%d", value);
	#ifdef DEBUG
		fflush(NULL);
	#endif
	if (*value < 1) {
		char errorMessage[1024];
		sprintf(errorMessage,ERROR_MESSAGE,message_type);
		reportErrorAndExit(errorMessage);
	}
}

void generateFileNames(char* folderpath,char* image_prefix,char* image_suffix){
	char* file_path;

	allSettings->filePathsArray = (char**)safeCalloc(allSettings->numOfImages, sizeof(char *));

	for (int i =0 ; i< allSettings->numOfImages;i++){
		file_path = (char*)safeCalloc(MAX_IMAGE_PATH_LENGTH, sizeof(char));

		sprintf(file_path,"%s%s%d%s",folderpath,image_prefix,i,image_suffix);

		#ifdef DEBUG
			fflush(NULL);
		#endif

		allSettings->filePathsArray[i] = file_path;
	}
}

imageData* calcImageData(char* filePath, int nBins, int maxNumFeatures)
{
	imageData* data = (imageData*)safeCalloc(1, sizeof(imageData));

	data->rgbHist = spGetRGBHist(filePath, nBins);
	if (data->rgbHist == NULL)
		reportErrorAndExit(IMAGE_PROC_ERROR_MESSAGE);

	data->siftDesc = spGetSiftDescriptors(filePath,maxNumFeatures,&(data->nFeatures));
	if (data->siftDesc == NULL)
			reportErrorAndExit(IMAGE_PROC_ERROR_MESSAGE);

	return data;
}

//This method calculates the histograms and the sift descriptors
void calcHistAndSIFTDesc()
{
	int i;
	imagesData = (imageData**)safeCalloc(allSettings->numOfImages, sizeof(imageData*));

	for (i=0 ; i<allSettings->numOfImages ; i++)
	{
		imagesData[i] = calcImageData(allSettings->filePathsArray[i],
								allSettings->numOfBins,
								allSettings->numOfFeatures);
	}
}

void setInput()
{
	allSettings = (settings*)safeCalloc(1, sizeof(settings));

	char folderpath[MAX_IMAGE_PATH_LENGTH];
	char image_prefix[MAX_IMAGE_PATH_LENGTH];
	char image_suffix[MAX_IMAGE_PATH_LENGTH];

	//Handle input
	printf(ENTER_IMAGES_DIRECTORY_PATH);
	#ifdef DEBUG
		fflush(NULL);
	#endif
	scanf("%s", folderpath);
	#ifdef DEBUG
		fflush(NULL);
	#endif

	printf(ENTER_IMAGES_PREFIX);
	#ifdef DEBUG
		fflush(NULL);
	#endif
	scanf("%s", image_prefix);
	#ifdef DEBUG
		fflush(NULL);
	#endif

	getAsPositiveInt(IMAGES, &(allSettings->numOfImages));

	printf(ENTER_IMAGES_SUFFIX);
	#ifdef DEBUG
		fflush(NULL);
	#endif
	scanf("%s", image_suffix);
	#ifdef DEBUG
		fflush(NULL);
	#endif

	getAsPositiveInt(BINS, &(allSettings->numOfBins));
	getAsPositiveInt(FEATURES, &(allSettings->numOfFeatures));

	//generate files names
	generateFileNames(folderpath, image_prefix, image_suffix);
}

int getKeyFromIntArray(void* array, int index)
{
	return ((int*)array)[index];
}

int getKeyFromkeyValueArray(void* array, int index)
{
	return ((keyValue**)array)[index]->key;
}

void printArraysTopItems(void* topItems, int (*funcGetKeyByIndex)( void*, int), const char* message) {
	int i;
	printf(message);

	#ifdef DEBUG
		fflush(NULL);
	#endif

	for (i = 0; i < NUM_OF_BEST_DIST_IMGS; i++) {

		printf("%d%s", funcGetKeyByIndex(topItems,i),
				i == (NUM_OF_BEST_DIST_IMGS - 1) ? "\n" : ", ");
		#ifdef DEBUG
			fflush(NULL);
		#endif
	}
}

void importSorted(int index, double distance, keyValue** items)
{
	int limit = index, j, k;
	keyValue* current = (keyValue*)safeCalloc(1, sizeof(keyValue));

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
		if (limit == NUM_OF_BEST_DIST_IMGS)
			free(temp1);
	}
	else
	{
		//clear memory of the unused item
		free(current);
	}
}

void freeDataForGlobalFeatures(keyValue** topItems)
{
	int i;
	if (topItems != NULL)
	{
		for (i=0;i<NUM_OF_BEST_DIST_IMGS;i++)
		{
			if (topItems[i] != NULL)
				free(topItems[i]);
		}
		free(topItems);
	}
}

void compareGlobalFeatures(imageData* workingImageData )
{
	int i;
	keyValue** topItems = (keyValue**)safeCalloc(NUM_OF_BEST_DIST_IMGS,sizeof(keyValue*));

	//fill items array sorted by the value, keep only top NUM_OF_BEST_DIST_IMGS
	for (i=0;i<allSettings->numOfImages;i++)
	{
		importSorted(i,spRGBHistL2Distance(imagesData[i]->rgbHist,
				workingImageData->rgbHist,allSettings->numOfBins) ,topItems);
	}

	//print items
	printArraysTopItems(topItems,&getKeyFromkeyValueArray ,NEAREST_IMAGES_USING_GLOBAL_DESCRIPTORS);

	//free data
	freeDataForGlobalFeatures(topItems);
}

void freeDataFromLocalDescProcess(int* counterArray, int* topItems, int*resultsArray) {
	if (resultsArray != NULL)
		free(resultsArray);

	if (topItems != NULL)
		free(topItems);

	if (counterArray != NULL)
		free(counterArray);
}

// sort the index counter array and get the best NUM_OF_BEST_DIST_IMGS
int* getTopItems(int* counterArray) {
	int i,j, tempMaxIndex;
	int *topItems = (int*)safeCalloc(NUM_OF_BEST_DIST_IMGS, sizeof(int));

	for (j = 0; j < NUM_OF_BEST_DIST_IMGS; j++) {
		tempMaxIndex = 0;
		for (i = 0; i < allSettings->numOfImages; i++) {
			if (counterArray[i] > counterArray[tempMaxIndex]) {
				tempMaxIndex = i;
			}
		}
		topItems[j] = tempMaxIndex;
		counterArray[tempMaxIndex] = -1;
	}
	return topItems;
}

/**
 * The method gets the database of images and an image to compare with,
 * it compares the image to database according to local features (sift descriptors)
 * it prints out the top "NUM_OF_BEST_DIST_IMGS" (see macro) indexes of images found similar
 * @param imagesData - the database as an array of pointers to an image data
 * @workingImageData - a pointer to the working image data
 */
void compareLocalFeatures(imageData* workingImageData)
{
	int i,j;
	int *topItems, *counterArray, *resultsArray;

	//create an index-counter array for the images
	counterArray = (int*)safeCalloc(allSettings->numOfImages, sizeof(int));

	//set up the counter array
	for (i=0;i<allSettings->numOfImages;i++)
	{
		counterArray[i] = 0;
	}

	//for each feature in the working image send a request to compare the best NUM_OF_BEST_DIST_IMGS
	// for each NUM_OF_BEST_DIST_IMGS returned increase a counter at the relevant index
	for (i=0; i<workingImageData->nFeatures;i++)
	{
		resultsArray = spBestSIFTL2SquaredDistance(NUM_OF_BEST_DIST_IMGS,
				workingImageData->siftDesc[i], allImagesDatabase->databaseFeatures,
				allSettings->numOfImages, allImagesDatabase->featuresPerImage);

		if (resultsArray == NULL)
		{
			reportErrorAndExit(IMAGE_COMPARE_ERROR_MESSAGE);
		}

		for (j=0; j<NUM_OF_BEST_DIST_IMGS ; j++)
		{
			counterArray[resultsArray[j]]++;
		}
	}

	//sort the index counter array and get the best 5
	topItems = getTopItems(counterArray);
	printArraysTopItems(topItems,&getKeyFromIntArray ,NEAREST_IMAGES_USING_LOCAL_DESCRIPTORS);

	//free data
	freeDataFromLocalDescProcess(counterArray, topItems, resultsArray);
}

/**
 * The method gets an image and frees the memory allocated to each one of its parts, and than free the memory for his pointer.
 * @image - a pointer to the image data
 */
void freeImageData(imageData* image)
{
	int i;
	if (image != NULL)
	{
		//free the rgbHist matrix
		if (image->rgbHist != NULL)
		{
			for (i=0;i<3;i++)
			{
				free(image->rgbHist[i]);
			}
			free(image->rgbHist);
		}
		//free the sift descriptors matrix
		if (image->siftDesc != NULL)
		{
			for (i=0;i<image->nFeatures;i++)
			{
				free(image->siftDesc[i]);
			}
			free(image->siftDesc);
		}
		//free the pointer to the image
		free(image);
	}
}

/**
 * The method gets the database of images and frees the memory allocated to each image, and to the array itself
 * @imagePath - the image path as string, pointer to a char array
 */
void freeImages()
{
	int i;
	if (imagesData != NULL)
	{
		for (i=0;i<allSettings->numOfImages;i++)
		{
			freeImageData(imagesData[i]);
		}
	}
	free(imagesData);
}

void freeDatabase()
{
	if (allImagesDatabase != NULL)
	{
		if (allImagesDatabase->databaseFeatures != NULL)
		{
			free(allImagesDatabase->databaseFeatures);
		}
		if (allImagesDatabase->featuresPerImage != NULL)
		{
			free(allImagesDatabase->featuresPerImage);
		}
		free(allImagesDatabase);
	}
}


void freeImagesAndDatabase(){
	freeDatabase();
	freeImages();
}

/**
 * The method gets the database of images and a path to an image,
 * it loads the image, compare it to the database images and ouptuts the comparison results.
 * @param imagesData - the database as an array of pointers to an image data
 * @imagePath - the image path as string, pointer to a char array
 */
void searchSimilarImages(char* imagePath)
{
	//process the current new image
	imageData* workingImageData = calcImageData(imagePath, allSettings->numOfBins, allSettings->numOfFeatures);

	//verify database is built
	if (allImagesDatabase == NULL)
		setImagesDatabase();

	//compare the image to the database regarding global features and print the results
	compareGlobalFeatures(workingImageData);

	//compare the image to the database regarding local features and print the results
	compareLocalFeatures(workingImageData);

	//free the current image data
	freeImageData(workingImageData);
}

/**
 * The method gets a pointer to a string, requests the user for an image path and update the string accordingly
 * @param workingImagePath - the pointer to the path
 */
void setPathFromUser(char* workingImagePath) {
	printf(ENTER_A_QUERY_IMAGE_OR_TO_TERMINATE);
	#ifdef DEBUG
		fflush(NULL);
	#endif
	scanf("%s", workingImagePath);
	#ifdef DEBUG
		fflush(NULL);
	#endif
}

/**
 * The method interacts with the user and outputs comparision's data between the users input image path and the database.
 * the interaction ends when "#" input is entered
 */
void startInteraction()
{
	//first run must always happen
	char* workingImagePath = (char*)safeCalloc(MAX_IMAGE_PATH_LENGTH,sizeof(char));

	setPathFromUser(workingImagePath);

	// iterating until the user inputs "#"
	while (strcmp(workingImagePath,"#"))
	{
		searchSimilarImages(workingImagePath);
		setPathFromUser(workingImagePath);
	}

	//free the memory allocated for the path string
	free(workingImagePath);

	//announce the user for exiting
	printf(EXITING);
	#ifdef DEBUG
		fflush(NULL);
	#endif
}

void start(){
	//sets the input data
#ifdef DEBUG
	//tests - debug mode
	allSettings = (settings*)safeCalloc(1, sizeof(settings));
	testSetInput(allSettings);
#else
	//production
	setInput();
#endif

	//create the database from the images
	calcHistAndSIFTDesc();

	//starts the user interaction
	startInteraction();

	//free memory and exit
	freeImagesAndDatabase();
	clearSettings();
}
