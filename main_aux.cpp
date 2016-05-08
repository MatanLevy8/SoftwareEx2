#include "sp_image_proc_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main_aux.h"

//#define DEBUG //TODO - comment this line for production mode
#define flushNull

#ifdef DEBUG
#include "tests.h"
#undef flushNull
#define flushNull fflush(NULL);
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

/*----------global variables---------*/

/*
 * A settings structure that holds the input settings
 */
settings* allSettings = NULL;
/*
 * An array of pointers that holds the data processed from the images in out database
 */
imageData** imagesData = NULL;
/*
 * A pointer to a structure that holds the images data manipulated in way that is convenient to use with the "sp_image_proc_utils"
 */
imagesDatabase* allImagesDatabase = NULL;

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
 * The method frees the memory allocated to each image from the loaded images, and to the array itself
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
		free(imagesData);
	}
}

/*
 * The method frees the memory allocated for the images database structure,
 * as it hold a re-ordered version of the data from images data
 */
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

/*
 * The method is responsible for freeing the memory allocated to the images data.
 */
void freeImagesAndDatabase(){
	freeDatabase();
	freeImages();
}

/*
 * The method is responsible for freeing the memory allocated to the settings global variable.
 */
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

/*
 * The method reports the user for an error and terminates the program
 * the message clears the global variables allocated memory.
 * @message - the error message that is printed to the user
 */
void reportErrorAndExit(const char* message)
{
	printf("%s",message);
	flushNull
	/*if (allSettings != NULL){
		freeImagesAndDatabase();
		clearSettings();
	}*/
	exit(1); //TODO - check what error code should we return
}

/*
 * The method reports the user for an allocation error and terminates the program
 */
void reportAllocationErrorAndExit()
{
	reportErrorAndExit(ALLOCATION_ERROR_MESSAGE);
}

/*
 * A bridge for allocating memory, the method acts as 'calloc' does,
 * but prompt of failure and exists the program if it could not allocate the requested memory
 * @itemsCount - the number of items to be allocated
 * @sizeOfItem - the size of each item
 * @returns - a void pointer to the allocated memory
 */
void* safeCalloc(int itemsCount, int sizeOfItem)
{
	void* memoryPointer = calloc(itemsCount, sizeOfItem);
	if (memoryPointer == NULL) //allocation failed
		reportAllocationErrorAndExit();
	return memoryPointer;
}

/*
 * This method sets the images database structure using the imagedData array.
 * This is just a manipulation of how the imaged data is stored, for an convenient use lated on.
 */
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

/*
 * The method requests the user to input a positive integer,
 * and update its input to the "value" item given as a pointer.
 * @message_type - a message suffix presented to the user before the input
 * @value - a pointer to an integer, where the result would be saved
 */
void getAsPositiveInt(const char* message_type, int* value)
{
	printf(ENTER_NUMBER_OF, message_type);
	flushNull
	scanf("%d", value);
	flushNull
	if (*value < 1) {
		char errorMessage[1024];
		sprintf(errorMessage,ERROR_MESSAGE,message_type);
		flushNull
		reportErrorAndExit(errorMessage);
	}
}

/*
 * The method requests the user to input some string, and return its input as a string
 * @message - a message presented to the user before the input
 * @return - the string that the user inputed
 */
char* getAsString(const char* message)
{
	char* response = (char*)safeCalloc(1024,sizeof(char));
	printf("%s",message);
	flushNull
	scanf("%s", response);
	flushNull

	return response;
}

/*
 * The method generated the file names into the allSettings variable, using the given parameters
 *@folderpath - a relative folder path
 *@image_prefix - the suffix of the images
 *@image_prefix - the prefix of the images
 */
void generateFileNames(char* folderpath,char* image_prefix,char* image_suffix){
	char* file_path;

	allSettings->filePathsArray = (char**)safeCalloc(allSettings->numOfImages, sizeof(char *));

	for (int i =0 ; i< allSettings->numOfImages;i++){
		file_path = (char*)safeCalloc(MAX_IMAGE_PATH_LENGTH, sizeof(char));

		sprintf(file_path,"%s%s%d%s",folderpath,image_prefix,i,image_suffix);
		flushNull

		allSettings->filePathsArray[i] = file_path;
	}
}

/*
 * The method create an imageData structure for a given image and properties (histogram and sift
 * descriptors).
 * @filePath - the path of the image
 * @nBins - the number of bins for the RGB Histogram
 * @maxNumFeatures - the maximum number of sift descriptors for the image
 * @returns - a pointer to a new structure item with the relevant data
 */
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


/*
 * This method calculate the histogram and sift
 * descriptors data for each on of the images that were loaded to the settings structure
 * it also allocated the shared imageData variable to be an array of pointers to the data of each image
 */
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

/*
 * This method gets the data needed for constructing the database items and generated the file names
 */
void setInput()
{
	allSettings = (settings*)safeCalloc(1, sizeof(settings));
	char *folderpath, *image_prefix, *image_suffix;

	//Handle input
	folderpath = getAsString(ENTER_IMAGES_DIRECTORY_PATH);
	image_prefix = getAsString(ENTER_IMAGES_PREFIX);
	getAsPositiveInt(IMAGES, &(allSettings->numOfImages));
	image_suffix = getAsString(ENTER_IMAGES_SUFFIX);
	getAsPositiveInt(BINS, &(allSettings->numOfBins));
	getAsPositiveInt(FEATURES, &(allSettings->numOfFeatures));

	//generate files names
	generateFileNames(folderpath, image_prefix, image_suffix);

	//free allocated strings
	if (folderpath != NULL)
		free(folderpath);
	if (image_prefix != NULL)
		free(image_prefix);
	if (image_suffix != NULL)
		free(image_suffix);

}

/*
 * This method is used for passing it as a parameter to the printArraysTopItems,
 * it gets the value from an array of integers pointers
 * @array - an array of key values pointers
 * @index - the index we wish to return its key
 */
int getKeyFromIntArray(void* array, int index)
{
	return ((int*)array)[index];
}

/*
 * This method is used for passing it as a parameter to the printArraysTopItems,
 * it gets the key from an array of keyValue's pointers
 * @array - an array of key values pointers
 * @index - the index we wish to return its key
 */
int getKeyFromkeyValueArray(void* array, int index)
{
	return ((keyValue**)array)[index]->key;
}

/*
 * The method gets an array and print the "index value" of its top NUM_OF_BEST_DIST_IMGS items
 * the "index value" is retrieved using the function parameter funcGetKeyByIndex since the array is of general type.
 * @topItems - the array of items
 * @funcGetKeyByIndex - a function pointer that gets an array and an index and return the "index value"
 * @message - a message that is printed pre-printing the array items
 */
void printArraysTopItems(void* topItems, int (*funcGetKeyByIndex)( void*, int), const char* message) {
	int i;
	printf("%s",message);
	flushNull

	for (i = 0; i < NUM_OF_BEST_DIST_IMGS; i++) {

		printf("%d%s", funcGetKeyByIndex(topItems,i),
				i == (NUM_OF_BEST_DIST_IMGS - 1) ? "\n" : ", ");
		flushNull
	}
}

/*
 * The method gets an array of top NUM_OF_BEST_DIST_IMGS keyValues (as pointers)
 * and insert a new item if it has place among the best NUM_OF_BEST_DIST_IMGS while keeping the array sorted
 * @index - the index of the keyValue item
 * @distance - the value of the keyValue item
 * @items - the array of pointers sorted by distance ascending
 */
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

	//fix off by 1 problem when shifting the rest of the items
	if (limit < NUM_OF_BEST_DIST_IMGS)
		limit++;

	// we should insert the new item, at the j'th position
	if (j < NUM_OF_BEST_DIST_IMGS)
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

/**
 * Free the memory of the allocated items in the global comparison process.
 * @topItems - an array of pointers of keyValue type
 */
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

/**
 * The method gets an image to compare with,
 * it compares the image to the images database according to global features (RGB Histogram)
 * it prints out the top "NUM_OF_BEST_DIST_IMGS" (see macro) indexes of images found similar
 * @workingImageData - a pointer to the working image data
 */
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

/**
 * Free the memory of the allocated items in the local descriptors process.
 * @counterArray - integer array used in the local compare method
 * @topItems - integer array used in the local compare method
 * @resultsArray - integer array used in the local compare method
 */
void freeDataFromLocalDescProcess(int* counterArray, int* topItems) {
	if (topItems != NULL)
		free(topItems);

	if (counterArray != NULL)
		free(counterArray);
}

/**
 * The method gets an array of counters and returns an array of the best
 * (desc order) NUM_OF_BEST_DIST_IMGS of the indexes of the original array,
 * @counterArray - integer array of counters
 * @returns - an array of the top NUM_OF_BEST_DIST_IMGS indexes of the original array
 */
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
 * The method gets an image to compare with,
 * it compares the image to the images database according to local features (sift descriptors)
 * it prints out the top "NUM_OF_BEST_DIST_IMGS" (see macro) indexes of images found similar
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

		if (resultsArray != NULL)
			free(resultsArray);
	}

	//sort the index counter array and get the best 5
	topItems = getTopItems(counterArray);
	printArraysTopItems(topItems,&getKeyFromIntArray ,NEAREST_IMAGES_USING_LOCAL_DESCRIPTORS);

	//free data
	freeDataFromLocalDescProcess(counterArray, topItems);
}

/**
 * The method gets a path to an image,
 * it loads the image, compare it to the database images and ouptputs the comparison results.
 * @imagePath - the image path as string, pointer to a char array
 */
void searchSimilarImages(char* imagePath)
{
	//process the current new image
	imageData* workingImageData = calcImageData(imagePath, allSettings->numOfBins, allSettings->numOfFeatures);

	//verify database is built (logically we could build the database after setting the
	//							images paths but if the user will choose to exit at first run would than work for nothing)
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
 * The method interacts with the user and outputs comparision's data between the users input image path and the database.
 * the interaction ends when "#" input is entered
 */
void startInteraction()
{
	//first run must always happen
	char* workingImagePath = getAsString(ENTER_A_QUERY_IMAGE_OR_TO_TERMINATE);

	// iterating until the user inputs "#"
	while (strcmp(workingImagePath,"#"))
	{
		searchSimilarImages(workingImagePath);
		free(workingImagePath);
		workingImagePath = getAsString(ENTER_A_QUERY_IMAGE_OR_TO_TERMINATE);
	}

	//free the memory allocated for the path string
	free(workingImagePath);

	//announce the user for exiting
	printf("%s",EXITING);
	flushNull
}

void start(){
	//register the memory freeing methods at exit
	atexit(&clearSettings);
	atexit(&freeImagesAndDatabase);


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

	//free memory ("atexit") and exit
	exit(0);
}
