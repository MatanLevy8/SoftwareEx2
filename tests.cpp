#include "tests.h"


void testSetInput(settings* allSettings)
{
	char*  folderpath = "./images/", *image_prefix = "img", *image_suffix = ".png";
	allSettings->numOfImages = 17;
	allSettings->numOfBins = 16;
	allSettings->numOfFeatures = 100;

	//generate files names
	generateFileNames(folderpath, image_prefix, image_suffix);
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


void printMatrix(double ** matrix, int maxNFeautres)
{
	printf("Printing matrix...\n");
	fflush(NULL);
	for (int i =0;i<maxNFeautres;i++)
	{
		for (int j=0;j<128;j++)
		{
			if (j%8 == 0)
			{
				printf("\n");
				fflush(NULL);
			}
			printf("%f | ",matrix[i][j]);
			fflush(NULL);
		}
		printf("\n----------------------------row-------------------------------------------\n");
		fflush(NULL);
	}
}

void printFeature(char* string, double* array)
{
	printf("%s : ",string);
	fflush(NULL);
	int i=0;
	while (i < 128)
	{
		printf("%f | ",*array);
		fflush(NULL);
		if (i%6 == 1)
		{
			printf("\n");
			fflush(NULL);
		}
		array++;
		i++;
	}
	printf("\n");
	fflush(NULL);
}
