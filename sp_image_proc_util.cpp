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


#define DEBUG //TODO - comment this line for production mode
#define flushNull

#ifdef DEBUG
#undef flushNull
#define flushNull fflush(NULL);
#endif

using namespace cv;

typedef struct distanceWithIndex {
	double distance;
	int index;
} distanceWithIndex;


void reportAllocationErrorAndExit_image_proc()
{
	printf(ALLOCATION_ERROR_MESSAGE);
	flushNull
	exit(1);
}


/*
 * A bridge for allocating memory, the method acts as 'calloc' does,
 * but prompt of failure and exists the program if it could not allocate the requested memory
 * @itemsCount - the number of items to be allocated
 * @sizeOfItem - the size of each item
 * @returns - a void pointer to the allocated memory
 */
void* safeCalloc_image_proc(int itemsCount, int sizeOfItem)
{
	void* memoryPointer = calloc(itemsCount, sizeOfItem);
	if (memoryPointer == NULL) //allocation failed
		reportAllocationErrorAndExit_image_proc();
	return memoryPointer;
}




int** fromMatArrayToIntMatrix(int nBins, Mat** planes)
{
	int col, bin;
	int** return_array = (int**)calloc(NUM_OF_COLS_IN_RGB, sizeof(int*));

	if (return_array != NULL)
	{
		for (col = 0; col < NUM_OF_COLS_IN_RGB; col++)
		{
			return_array[col] = (int*)calloc(nBins, sizeof(int));
			if (return_array[col] != NULL)
				for (bin = 0; bin < nBins; bin++)
					return_array[col][bin] =
							(int)(planes[col])->at<float>(bin);
		}
	}
	return return_array;
}

int** spGetRGBHist(char* str, int nBins)
{
	Mat src = imread(str, CV_LOAD_IMAGE_COLOR);
	if (!src.data)
		return NULL; //TODO - check what should we return in this case

	//separate the image to rgb planes
	std::vector<Mat> rgb_planes;
	split(src, rgb_planes);

	//set the range for RGB
	float range[] = { 0, RANGE_FOR_RGB };
	const float* histRange = {range};

	Mat r_hist, g_hist, b_hist;

	/// Compute the histograms
	calcHist(&rgb_planes[0], NUM_OF_IMAGES_FOR_CALCHIST,
			NUM_OF_CHANELLS_FOR_CALCHIST, Mat(), b_hist,
			DIMS_FOR_CALCHIST, &nBins, &histRange);
	calcHist(&rgb_planes[1], NUM_OF_IMAGES_FOR_CALCHIST,
			NUM_OF_CHANELLS_FOR_CALCHIST, Mat(), g_hist,
			DIMS_FOR_CALCHIST, &nBins, &histRange);
	calcHist(&rgb_planes[2], NUM_OF_IMAGES_FOR_CALCHIST,
			NUM_OF_CHANELLS_FOR_CALCHIST, Mat(), r_hist,
			DIMS_FOR_CALCHIST, &nBins, &histRange);

	Mat* planes[2];
	planes[0] = &b_hist;
	planes[1] = &g_hist;
	planes[2] = &r_hist;

	//create the 2 dimensional array for the output
	return fromMatArrayToIntMatrix(nBins, planes);
}

double spRGBHistL2Distance(int** histA, int** histB, int nBins)
{
	int col, bin;
	double current_vector_dist;
	double current_item;
	double l2_squared = 0;

	for (col = 0; col < NUM_OF_COLS_IN_RGB; col++)
	{
		current_vector_dist = 0;

		for (bin = 0; bin < nBins; bin++){
			{
				current_item = histA[col][bin] - histB[col][bin];
				current_vector_dist += 0.33 * current_item * current_item;
			}
		}

		l2_squared += current_vector_dist;
	}

	return l2_squared;
}

//TODO - decide how to unite with int** creator
double** fromMatTypeToDoubleMat(int *nFeatures, Mat destination_matrix)
{
	int feature, elem;
	double** output_matrix = (double**)calloc((*nFeatures), sizeof(double*));

	if (output_matrix != NULL)
	{
		for (feature = 0; feature < *nFeatures; feature++)
		{
			output_matrix[feature] = (double*)calloc(NUM_OF_ELEMS_IN_FEAT,
					sizeof(double));
			if (output_matrix[feature] != NULL)
				for (elem = 0; elem < NUM_OF_ELEMS_IN_FEAT; elem++)
					output_matrix[feature][elem] =
						(double)(destination_matrix.at<float>(feature, elem));
		}
	}

	return output_matrix;
}

double** spGetSiftDescriptors(char* str, int maxNFeautres, int *nFeatures)
{
	std::vector<KeyPoint> keypoints;
	Mat destination_matrix;
	Mat src = imread(str, CV_LOAD_IMAGE_GRAYSCALE);

	if (!src.data)
		return NULL; //TODO - check what should we return in this case

	Ptr<xfeatures2d::SiftDescriptorExtractor> detect =
			xfeatures2d::SIFT::create(maxNFeautres);
	detect->detect(src, keypoints, Mat());
	detect->compute(src, keypoints, destination_matrix);

	//set the features count
	*nFeatures = keypoints.size();

	//set the return matrix values
	return fromMatTypeToDoubleMat(nFeatures, destination_matrix);
}

double spL2SquaredDistance(double* featureA, double* featureB)
{
	double dist = 0.0;
	double current;
	int elem;

	for (elem = 0; elem < NUM_OF_ELEMS_IN_FEAT; elem++)
	{
		current = featureA[elem] - featureB[elem];
		dist += current * current;
	}

	return dist;
}

int distanceWithIndexComparator(const void * a, const void * b)
{
	distanceWithIndex** item1;
	distanceWithIndex** item2;
	double dist;
	item1 = (distanceWithIndex**)a;
	item2 = (distanceWithIndex**)b;

	dist = (*item1)->distance - (*item2)->distance;

	if (dist < 0.0)
		return -1;

	if (dist > 0.0)
		return 1;

	return (*item1)->index - (*item2)->index;
}

int* spBestSIFTL2SquaredDistance(int bestNFeatures, double* featureA,
		double*** databaseFeatures, int numberOfImages,
		int* nFeaturesPerImage)
{
	int totalNumberOfFeatures = 0;
	int image, imageFeature, feature;
	distanceWithIndex* curr;
	distanceWithIndex** distancesArray;
	int* outputArray;
	for (image = 0; image < numberOfImages; image++)
		totalNumberOfFeatures += nFeaturesPerImage[image];

	distancesArray = (distanceWithIndex**)calloc(totalNumberOfFeatures,
			sizeof(distanceWithIndex*));

	if (distancesArray != NULL) {
		feature = 0;
		for (image = 0; image < numberOfImages; image++)
		{
			for (imageFeature = 0; imageFeature < nFeaturesPerImage[image];
					imageFeature++)
			{
				curr = (distanceWithIndex*)malloc(sizeof(distanceWithIndex));
				curr->distance = spL2SquaredDistance(
						databaseFeatures[image][imageFeature], featureA);
				curr->index = image;
				distancesArray[feature] = curr;
				feature++;
			}
		}

		qsort(distancesArray, totalNumberOfFeatures,
				sizeof(distanceWithIndex*), distanceWithIndexComparator);
	}

	// TODO - check if we can assume that bestNFeatures <= totalNumberOfFeatures
	outputArray = (int*)calloc(bestNFeatures, sizeof(int));
	if (outputArray != NULL)
		for (feature = 0; feature < bestNFeatures; feature++)
			outputArray[feature] = distancesArray[feature]->index;

	if (distancesArray != NULL)
	{
		for (feature = 0; feature < totalNumberOfFeatures; feature++)
			if (distancesArray[feature] != NULL)
				free(distancesArray[feature]);

		free(distancesArray);
	}

	return outputArray;
}

