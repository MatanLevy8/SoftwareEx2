#include "sp_image_proc_util.h"
//#include "tests.h" //TODO - DONT FORGET TO DELETE THIS
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/xfeatures2d.hpp"
#include <assert.h>

#define NUM_OF_COLS_IN_RGB 3

using namespace cv;

typedef struct distanceWithIndex {
	double distance;
	int index;
} distanceWithIndex;

int** spGetRGBHist(char* str, int nBins)
{
	assert(str != NULL && nBins >0);
	int** return_array;
	Mat src = imread(str,CV_LOAD_IMAGE_COLOR);
	if (!src.data){
		//assert(str == NULL);
		return NULL; //TODO - check what should we return in this case
	}
	//separate the image to rgb planes
	std::vector<Mat> rgb_planes;
	split(src,rgb_planes);

	//set the range for RGB
	float range[] = {0,256};
	const float* histRange = {range};

	Mat r_hist, g_hist, b_hist;

	/// Compute the histograms:
	calcHist(&rgb_planes[0],1,0,Mat(),b_hist,1,&nBins,&histRange);
	calcHist(&rgb_planes[1],1,0,Mat(),g_hist,1,&nBins,&histRange);
	calcHist(&rgb_planes[2],1,0,Mat(),r_hist,1,&nBins,&histRange);

	Mat* planes[2];
	planes[0] = &b_hist;
	planes[1] = &g_hist;
	planes[2] = &r_hist;

	//create the 2 dimensional array for the output
	return_array = (int**)calloc(NUM_OF_COLS_IN_RGB, sizeof(int*));
	//TODO - refactor to function ?
	if (return_array!=NULL)
	{
		for (int i = 0; i < NUM_OF_COLS_IN_RGB; i++)
		{
			return_array[i] = (int*)calloc(nBins, sizeof(int));
			for (int j=0;j<nBins;j++){
				return_array[i][j] = (int)(planes[i])->at<float>(0,j);
			}
		}
	}

	assert(return_array != NULL);
	assert(return_array[0] != NULL);
	assert(return_array[1] != NULL);
	assert(return_array[2] != NULL);
	return return_array;
}

double spRGBHistL2Distance(int** histA, int** histB, int nBins)
{
	assert(histA != NULL && histB!= NULL && nBins>0);
	double l2_squared = 0,current_vector_dist;
	double current_item;
	for (int i = 0; i < NUM_OF_COLS_IN_RGB; i++)
	{
		current_vector_dist = 0;
		for (int j=0;j<nBins;j++){
			{
				current_item = histA[i][j] - histB[i][j];
				current_vector_dist += (0.33)*current_item*current_item;
				assert(current_vector_dist>=0);
			}
		}
		l2_squared += current_vector_dist;
	}
	assert(l2_squared>=0);
	return l2_squared;
}

//TODO - Check if we need to change the name features?
double** spGetSiftDescriptors(char* str, int maxNFeautres, int *nFeatures)
{
	assert(str != NULL && maxNFeautres > 0 && nFeatures != NULL);
	double** output_matrix;
	std::vector<KeyPoint> keypoints;
	Mat destination_matrix, src = imread(str,CV_LOAD_IMAGE_GRAYSCALE);
	assert(src.data != NULL);

	Ptr<xfeatures2d::SiftDescriptorExtractor> detect = xfeatures2d::SIFT::create(maxNFeautres);
	detect->detect(src,keypoints,Mat());
	detect->compute(src,keypoints,destination_matrix);

	//TODO - check wtf to do with this shit
	//set the features count

	//*nFeatures = (int)keypoints.size() < maxNFeautres ? keypoints.size():maxNFeautres;//destination_matrix.rows;
	*nFeatures = keypoints.size();


	//set the return matrix values
	output_matrix = (double**)calloc((*nFeatures), sizeof(double*));
	if (output_matrix != NULL){
		for (int i = 0;i< *nFeatures;i++)
		{
			output_matrix[i] = (double*)calloc(128, sizeof(double));
			for (int j = 0 ;j < 128 ;j++)
			{
				output_matrix[i][j] = (double)(destination_matrix.at<float>(i,j));
			}
		}
	}

	return output_matrix;
}

double spL2SquaredDistance(double* featureA, double* featureB){
	double dist = 0.0, current;
	int i;
	for (i=0 ; i<128 ; i++)
	{
		current = featureA[i]-featureB[i];
		dist += current*current;
		assert(dist>=0.0);
	}
	return dist;
}

int distanceWithIndexComparator(const void * a, const void * b) {
	distanceWithIndex** item1;
	distanceWithIndex** item2;
	double dist;
	item1 = (distanceWithIndex**)a;
	item2 = (distanceWithIndex**)b;

	dist = (*item1)->distance - (*item2)->distance;

	if (dist < 0.0)
	{
		return -1;
	}
	if (dist > 0.0){
		return 1;
	}

	return (*item1)->index - (*item2)->index;
}

int* spBestSIFTL2SquaredDistance(int bestNFeatures, double* featureA,
		double*** databaseFeatures, int numberOfImages,
		int* nFeaturesPerImage){
	int totalNumberOfFeatures = 0, i, j, k;
	distanceWithIndex* curr;
	distanceWithIndex** distancesArray;
	int* outputArray;
	for (int i = 0; i < numberOfImages; i++) {
		totalNumberOfFeatures += nFeaturesPerImage[i];
	}
	distancesArray = (distanceWithIndex**)calloc(totalNumberOfFeatures, sizeof(distanceWithIndex*));

	if (distancesArray != NULL) {
		k = 0;
		for (i = 0; i < numberOfImages; i++) {
			for (j = 0; j < nFeaturesPerImage[i]; j++) {
				curr = (distanceWithIndex*)malloc(sizeof(distanceWithIndex));
				curr->distance = spL2SquaredDistance(databaseFeatures[i][j], featureA);
				curr->index =  i;
				distancesArray[k] = curr;
				k++;
			}
		}
		qsort(distancesArray, totalNumberOfFeatures, sizeof(distanceWithIndex*), distanceWithIndexComparator);
	}

	// TODO - check if we can assume that bestNFeatures <= totalNumberOfFeatures
	outputArray = (int*)calloc(bestNFeatures, sizeof(int));
	if (outputArray != NULL) {
		for (i = 0; i < bestNFeatures; i++) {
			outputArray[i] = distancesArray[i]->index;
		}
	}
	for (k = 0; k < totalNumberOfFeatures; k++) {
		free(distancesArray[k]);
	}
	free(distancesArray);

	return outputArray;
}
