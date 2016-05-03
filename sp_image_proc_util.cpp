#include "sp_image_proc_util.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/xfeatures2d.hpp"
using namespace cv;


int** spGetRGBHist(char* str, int nBins)
{
	int** return_array;
	Mat src = imread(str,1);
	if (!src.data){
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


	std::vector<Mat*> planes;
	planes.at(0) = &b_hist;
	planes.at(1) = &g_hist;
	planes.at(2) = &r_hist;

	//create the 2 dimensional array for the output
	return_array = (int**)malloc(3*nBins*sizeof(int));
	//TODO - refactor to function ?
	if (return_array!=NULL)
	{
		for (int i=0;i<3;i++)
		{
			for (int j=0;j<nBins;j++){
				return_array[i][j] = (planes.at(i))->at<int>(j);
			}
		}
	}
	return return_array;
}

double spRGBHistL2Distance(int** histA, int** histB, int nBins)
{
	double l2_squared = 0,current_vector_dist;
	int current_item;
	for (int i=0;i<3;i++)
	{
		current_vector_dist = 0;
		for (int j=0;j<nBins;j++){
			{
				current_item = histA[i][j] - histB[i][j];
				current_vector_dist += current_item*current_item;
			}
		}
		l2_squared += (0.33)*current_vector_dist;
	}
	return l2_squared;
}

double** spGetSiftDescriptors(char* str, int maxNFeautres, int *nFeatures)
{
	double** output_matrix;
	std::vector<KeyPoint> keypoints;
	Mat destination_matrix, src = imread(str,1);

	Ptr<xfeatures2d::SiftDescriptorExtractor> detect = xfeatures2d::SIFT::create(maxNFeautres);
	detect->detect(src,keypoints,Mat());
	detect->compute(src,keypoints,destination_matrix);

	//set the features count
	*nFeatures = destination_matrix.rows;

	//set the return matrix values

	output_matrix = (double**)malloc(128*(*nFeatures)*sizeof(double));
	if (output_matrix != NULL){
		for (int i = 0;i< *nFeatures;i++)
		{
			for (int j = 0 ;j < 128 ;j++)
			{
				output_matrix[i][j] = destination_matrix.at<double>(i,j);
			}
		}
	}
	return output_matrix;
}

double spL2SquaredDistance(double* featureA, double* featureB){
	double l2_squared_dist = 0;
	for (int i = 0 ; i < 128 ; i++)
	{
		double current = featureA[i]-featureB[i];
		l2_squared_dist += current*current;
	}
	return l2_squared_dist;
}

typedef struct distanceWithIndex {
	double distance;
	int index;
} distanceWithIndex;

int distanceWithIndexComparator(const void * a, const void * b) {
	distanceWithIndex* item1;
	distanceWithIndex* item2;
	double dist;
	item1 = (distanceWithIndex*)a;
	item2 = (distanceWithIndex*)b;
	dist = item1->distance - item2->distance;
	if (dist == 0)
		return item1->index - item2->index;
	return dist;
}

int* spBestSIFTL2SquaredDistance(int bestNFeatures, double* featureA,
		double*** databaseFeatures, int numberOfImages,
		int* nFeaturesPerImage){
	int totalNumberOfFeatures = 0;
	distanceWithIndex** distancesArray;
	int* outputArray;
	for (int i = 0; i < numberOfImages; i++) {
		totalNumberOfFeatures += nFeaturesPerImage[i];
	}
	distancesArray = (distanceWithIndex**)malloc(totalNumberOfFeatures*sizeof(distanceWithIndex*));

	if (distancesArray != NULL) {
		int k = 0;
		for (int i = 0; i < numberOfImages; i++) {
			for (int j = 0; j < nFeaturesPerImage[i]; j++) {
				distanceWithIndex curr;
				curr.distance =  spL2SquaredDistance(databaseFeatures[i][j], featureA);
				curr.index =  i;
				distancesArray[k] = &curr;
				k++;
			}
		}
		qsort(distancesArray, totalNumberOfFeatures, sizeof(distanceWithIndex), distanceWithIndexComparator);
	}

	// TODO - check if we can assume that bestNFeatures <= totalNumberOfFeatures
	outputArray = (int*)malloc(bestNFeatures*sizeof(int));
	if (outputArray != NULL) {
		for (int i = 0; i < bestNFeatures; i++) {
			outputArray[i] = distancesArray[i]->index;
		}
	}
	return outputArray;
}


