#include "sp_image_proc_util.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/xfeatures2d.hpp"

#define NUM_OF_COLS_IN_RGB 3
#define RANGE_FOR_RGB 256
#define NUM_OF_IMAGES_FOR_CALCHIST 1
#define NUM_OF_CHANELLS_FOR_CALCHIST 0
#define DIMS_FOR_CALCHIST 1
#define NUM_OF_ELEMS_IN_FEAT 128
#define ALLOCATION_ERROR_MESSAGE "An error occurred - allocation failure\n"
#define DEFAULT_UNSUCCESFUL_VALUE -1

#define DEBUG //TODO - comment this line for production mode
#define flushNull

#ifdef DEBUG
#undef flushNull
#define flushNull fflush(NULL);
#endif

using namespace cv;

/*
 * A structure for holding a distance and the index associated with it.
 */
typedef struct distanceWithIndex {
	double distance;
	int index;
} distanceWithIndex;

/*
 * The method reports an allocation error to the user and terminates the
 * program.
 */
void reportAllocationErrorAndExit_image_proc()
{
	printf("%s", ALLOCATION_ERROR_MESSAGE);
	flushNull
	exit(1);
}

/*
 * A bridge for allocating memory, the method acts as 'calloc' does,
 * but prompt of failure and exists the program if it could not allocate
 * the requested memory.
 * @itemsCount - the number of items to be allocated
 * @sizeOfItem - the size of each item
 * @return a void pointer to the allocated memory
 */
void* safeCalloc_image_proc(int itemsCount, int sizeOfItem)
{
	void* memoryPointer = calloc(itemsCount, sizeOfItem);
	if (memoryPointer == NULL) // allocation failed
		reportAllocationErrorAndExit_image_proc();
	return memoryPointer;
}

/*
 * allocates an integer matrix, fills it from the Mat* array received
 * and returns it.
 * @nBins - the number of bins of the histogram
 * @planes - an array of Mat*, each item in the array represents different
 * color: R, G, B
 * @return an integer matrix, representing the RGB histogram
 */
int** RGBPlanesAsIntMatrix(int nBins, Mat** planes)
{
	int col, bin;
	int** returnArray = (int**)safeCalloc_image_proc(NUM_OF_COLS_IN_RGB,
			sizeof(int*));

	if (returnArray != NULL)
	{
		for (col = 0; col < NUM_OF_COLS_IN_RGB; col++)
		{
			returnArray[col] = (int*)safeCalloc_image_proc(nBins, sizeof(int));
			if (returnArray[col] != NULL)
			{
				for (bin = 0; bin < nBins; bin++)
					returnArray[col][bin] =
							(int)(planes[col])->at<float>(bin);
			}
		}
	}

	return returnArray;
}

int** spGetRGBHist(char* str, int nBins)
{
	std::vector<Mat> rgbPlanes;
	Mat rHist, gHist, bHist;
	Mat* planes[NUM_OF_COLS_IN_RGB], src;
	float range[] = { 0, RANGE_FOR_RGB }; //set the range for RGB
	const float* histRange = {range};

	if (str == NULL || nBins <= 0)
		return NULL;

	src = imread(str, CV_LOAD_IMAGE_COLOR);

	if (!src.data)
		return NULL;

	//separate the image to rgb planes
	split(src, rgbPlanes);

	/// Compute the histograms
	calcHist(&rgbPlanes[0], NUM_OF_IMAGES_FOR_CALCHIST,
			NUM_OF_CHANELLS_FOR_CALCHIST, Mat(), bHist,
			DIMS_FOR_CALCHIST, &nBins, &histRange);
	calcHist(&rgbPlanes[1], NUM_OF_IMAGES_FOR_CALCHIST,
			NUM_OF_CHANELLS_FOR_CALCHIST, Mat(), gHist,
			DIMS_FOR_CALCHIST, &nBins, &histRange);
	calcHist(&rgbPlanes[2], NUM_OF_IMAGES_FOR_CALCHIST,
			NUM_OF_CHANELLS_FOR_CALCHIST, Mat(), rHist,
			DIMS_FOR_CALCHIST, &nBins, &histRange);

	// fill the Mat* array
	planes[0] = &rHist;
	planes[1] = &gHist;
	planes[2] = &bHist;

	//create the 2 dimensional array for the output and return it
	return RGBPlanesAsIntMatrix(nBins, planes);
}

double spRGBHistL2Distance(int** histA, int** histB, int nBins)
{
	int col, bin;
	double currentVectorDist;
	double currentItem;
	double l2Squared = 0;

	if (nBins <= 0 || histA == NULL || histB == NULL)
		return DEFAULT_UNSUCCESFUL_VALUE;

	for (col = 0; col < NUM_OF_COLS_IN_RGB; col++)
	{
		currentVectorDist = 0;

		for (bin = 0; bin < nBins; bin++)
		{
				currentItem = histA[col][bin] - histB[col][bin];
				currentVectorDist += 0.33 * currentItem * currentItem;
		}

		l2Squared += currentVectorDist;
	}

	return l2Squared;
}

/*
 * allocates a double matrix, fills it from the Mat received
 * and returns it.
 * @nFeatures - a pointer the number of features in the image
 * @destination_matrix - a Mat, representing the matrix returned from OpenCV.
 * @return a double matrix, representing the Sift Descriptors
 */
double** SiftDescriptorsAsDoubleMatrix(int *nFeatures, Mat destination_matrix)
{
	int feature, elem;
	double** outputMatrix = (double**)safeCalloc_image_proc((*nFeatures),
			sizeof(double*));

	if (outputMatrix != NULL)
	{
		for (feature = 0; feature < *nFeatures; feature++)
		{
			outputMatrix[feature] = (double*)safeCalloc_image_proc(
					NUM_OF_ELEMS_IN_FEAT, sizeof(double));
			if (outputMatrix[feature] != NULL)
			{
				for (elem = 0; elem < NUM_OF_ELEMS_IN_FEAT; elem++)
					outputMatrix[feature][elem] =
						(double)(destination_matrix.at<float>(feature, elem));
			}
		}
	}

	return outputMatrix;
}

double** spGetSiftDescriptors(char* str, int maxNFeautres, int *nFeatures)
{
	std::vector<KeyPoint> keypoints;
	Mat destinationMatrix;
	Mat src;

	if (str == NULL || maxNFeautres <= 0 || nFeatures == NULL)
		return NULL;

	src =  imread(str, CV_LOAD_IMAGE_GRAYSCALE);

	if (!src.data)
		return NULL;

	// detect and compute features
	Ptr<xfeatures2d::SiftDescriptorExtractor> detect =
			xfeatures2d::SIFT::create(maxNFeautres);
	detect->detect(src, keypoints, Mat());
	detect->compute(src, keypoints, destinationMatrix);

	// set the features count
	*nFeatures = keypoints.size();

	// create the return matrix and fill it
	return SiftDescriptorsAsDoubleMatrix(nFeatures, destinationMatrix);
}

double spL2SquaredDistance(double* featureA, double* featureB)
{
	double dist = 0.0;
	double current;
	int elem;

	if (featureA == NULL || featureB == NULL)
		return DEFAULT_UNSUCCESFUL_VALUE;

	for (elem = 0; elem < NUM_OF_ELEMS_IN_FEAT; elem++)
	{
		current = featureA[elem] - featureB[elem];
		dist += current * current;
	}

	return dist;
}

/*
 * a comparator for items of type "distanceWithItems"
 * @firstItem - a pointer to the first item to be compared
 * @secondItem - a pointer to the second item to be compared
 * @return -1 if distance of firstItem is lower, 1 if distance of secondItem is
 * lower, and index of the first item minus index of the secondItem in case
 * their distance is the same.
 */
int distanceWithIndexComparator(const void * firstItem, const void * secondItem)
{
	distanceWithIndex** item1;
	distanceWithIndex** item2;
	double dist;
	item1 = (distanceWithIndex**)firstItem;
	item2 = (distanceWithIndex**)secondItem;

	dist = (*item1)->distance - (*item2)->distance;

	if (dist < 0.0)
		return -1;

	if (dist > 0.0)
		return 1;

	return (*item1)->index - (*item2)->index;
}

/**
 * calculates the total number features in all of the images and returns it.
 * @param numberOfImages - The number of images in the database.
 * @param nFeaturesPerImage - The number of features for each image.
 * @return the total number features in all of the images.
 */
int calcTotalNumberOfFeatures(int numberOfImages, int *nFeaturesPerImage)
{
	int totalNumberOfFeatures = 0;
	int image;
	for (image = 0; image < numberOfImages; image++)
		totalNumberOfFeatures += nFeaturesPerImage[image];
	return totalNumberOfFeatures;
}

/**
 * allocate an array of distanceWithIndex*, fills it according to all the
 * parameters given, sorts it and returns it.
 * @param totalNumberOfFeatures - the number of features in all of the images.
 * @param featureA - A sift descriptor which will be compared with the other
 * descriptors
 * @param databaseFeatures - An array of two dimensional array, in which the
 * descriptors of the images are stored.
 * @param numberOfImages - The number of images in the database.
 * @param nFeaturesPerImage - The number of features for each image.
 * @return a sorted array containing distanceWithIndex* according to the given
 * parameters.
 */
distanceWithIndex** createAndSortDistancesArray(int totalNumberOfFeatures,
		double* featureA, double*** databaseFeatures, int numberOfImages,
		int* nFeaturesPerImage)
{
	int imageIndex, imageFeature, feature;
	distanceWithIndex* curr;
	distanceWithIndex** distancesArray =
			(distanceWithIndex**)safeCalloc_image_proc(totalNumberOfFeatures,
					sizeof(distanceWithIndex*));

	if (distancesArray != NULL)
	{
		feature = 0;
		for (imageIndex = 0; imageIndex < numberOfImages; imageIndex++)
		{
			for (imageFeature = 0; imageFeature < nFeaturesPerImage[imageIndex];
					imageFeature++)
			{
				curr = (distanceWithIndex*)safeCalloc_image_proc(1,
						sizeof(distanceWithIndex));
				curr->distance = spL2SquaredDistance(
						databaseFeatures[imageIndex][imageFeature], featureA);
				curr->index = imageIndex;
				distancesArray[feature] = curr;
				feature++;
			}
		}

		// sort distance array first according distance and afterwards according
		// to index
		qsort(distancesArray, totalNumberOfFeatures,
				sizeof(distanceWithIndex*), distanceWithIndexComparator);
	}

	return distancesArray;
}

/*
 * allocate output array, fills it with the first bestNFeatures in
 * distancesArray (assuming distancesArray is sorted)
 * @bestNFeatures - number of features to fill the output array with
 * @distancesArray - the array from which we output the best features
 * @return an integer array contains the first bestNFeatures in distancesArray
 * (assuming distancesArray is sorted)
 */
int* createOutputArray(int bestNFeatures, distanceWithIndex** distancesArray)
{
	int feature;
	int *outputArray = (int*)safeCalloc_image_proc(bestNFeatures, sizeof(int));
	if (outputArray != NULL)
	{
		for (feature = 0; feature < bestNFeatures; feature++)
			outputArray[feature] = distancesArray[feature]->index;
	}
	return outputArray;
}

/*
 * free each item in the given distancesArray and then the array itself.
 * @distancesArray - the distancesArray to free
 * @totalNumberOfFeatures - number of items in the array
 */
void distancesArrayCleanup(distanceWithIndex** distancesArray,
		int totalNumberOfFeatures)
{
	int feature;
	if (distancesArray != NULL)
	{
		for (feature = 0; feature < totalNumberOfFeatures; feature++)
		{
			if (distancesArray[feature] != NULL)
				free(distancesArray[feature]);
		}

		free(distancesArray);
	}
}

int* spBestSIFTL2SquaredDistance(int bestNFeatures, double* featureA,
		double*** databaseFeatures, int numberOfImages,
		int* nFeaturesPerImage)
{
	distanceWithIndex** distancesArray;
	int* outputArray;
	int totalNumberOfFeatures;

	if (featureA == NULL || databaseFeatures == NULL ||
			numberOfImages <= 1 || nFeaturesPerImage == NULL)
		return NULL;

	totalNumberOfFeatures = calcTotalNumberOfFeatures(numberOfImages,
				nFeaturesPerImage); // calculate total number of features


	// create distances array, fill it and sort it
	distancesArray = createAndSortDistancesArray(totalNumberOfFeatures,
			featureA, databaseFeatures, numberOfImages, nFeaturesPerImage);

	// allocate output array and fill it
	outputArray = createOutputArray(bestNFeatures, distancesArray);

	// free the memory which is no longer needed
	distancesArrayCleanup(distancesArray, totalNumberOfFeatures);

	return outputArray;
}

