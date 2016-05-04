#ifndef MAIN_AUX_H_
#define MAIN_AUX_H_

typedef struct settings{
	int numOfBins;
	int numOfFeatures;
	int numOfImages;
	char** filePathsArray;
} settings;

typedef struct imageData{
	int** rgbHist;
	double** siftDesc;
	int nFeatures;
} imageData;

typedef struct keyValue{
	int key;
	double value;
} keyValue;

void generateFileNames(char* folderpath,char* image_prefix,char* image_suffix);

void start();


#endif
