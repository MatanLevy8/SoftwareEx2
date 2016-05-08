#ifndef MAIN_AUX_H_
#define MAIN_AUX_H_

/*
 * A structure used to save all the relevant session settings
 * @numOfBins - the number of bins to split the RGB Histogram,
 * 				received from the user as input
 * @numOfFeatures - the maximum number of features to use for the local descriptors,
 * 					received from the user as input
 * @numOfImages - the number of images to load - received from the user as input
 * @filePathsArray - an array of strings representing the (relative) path to the images set
 */
typedef struct settings{
	int numOfBins;
	int numOfFeatures;
	int numOfImages;
	char** filePathsArray;
} settings;

/*
 * A structure used to save the data analyzed from a specific image
 * @rgbHist - a matrix of integers representing the RGB histogram data of the image
 * @siftDesc - a matrix of doubles used to represent the local sift descriptors data of the image
 * @nFeatures - an integer representing the number  of features that were analyzed on the image,
 * 				that is the number of rows at "siftDesc" matrix
 */
typedef struct imageData{
	int** rgbHist;
	double** siftDesc;
	int nFeatures;
} imageData;

/*
 * A structure used in order to save a re-arranged format of the images database,
 * that can be passed to the "sp_image_proc_util" methods conveniently
 * @databaseFeatures - an array of 2-dimensional matrix's such that the i'th
 * 					   item in the array represent the sift descriptors of the
 * 					   i'th image in the working set of images
 * @featuresPerImage - an array of integers, the i'th item represent the number
 * 					   of features that were collected for the i'th image in the
 * 					   working set of images
 */
typedef struct imagesDatabase{
	double*** databaseFeatures;
	int* featuresPerImage;
} imagesDatabase;

/*
 * A structure used to save a "key value" type for sorting purposes
 * @key - a key as an integer
 * @value - a value as a double
 */
typedef struct keyValue{
	int key;
	double value;
} keyValue;

//TODO - this is not necessary for production, just for tests
void generateFileNames(char* folderpath,char* image_prefix,char* image_suffix);

/**
 * The method used in order to start the process.
 */
void start();


#endif
