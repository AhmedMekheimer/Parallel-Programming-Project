#include <iostream>
#include <math.h>
#include <stdlib.h>
#include<string.h>
#include<msclr\marshal_cppstd.h>
#include <ctime>// include this header 
#include <mpi.h>
#pragma once

#using <mscorlib.dll>
#using <System.dll>
#using <System.Drawing.dll>
#using <System.Windows.Forms.dll>
using namespace std;
using namespace msclr::interop;

int* inputImage(int* w, int* h, System::String^ imagePath) //put the size of image in w & h
{
	int* input;


	int OriginalImageWidth, OriginalImageHeight;

	//*********************************************************Read Image and save it to local arrayss*************************	
	//Read Image and save it to local arrayss

	System::Drawing::Bitmap BM(imagePath);

	OriginalImageWidth = BM.Width;
	OriginalImageHeight = BM.Height;
	*w = BM.Width;
	*h = BM.Height;
	int *Red = new int[BM.Height * BM.Width];
	int *Green = new int[BM.Height * BM.Width];
	int *Blue = new int[BM.Height * BM.Width];
	input = new int[BM.Height*BM.Width];
	for (int i = 0; i < BM.Height; i++)
	{
		for (int j = 0; j < BM.Width; j++)
		{
			System::Drawing::Color c = BM.GetPixel(j, i);

			Red[i * BM.Width + j] = c.R;
			Blue[i * BM.Width + j] = c.B;
			Green[i * BM.Width + j] = c.G;

			input[i*BM.Width + j] = ((c.R + c.B + c.G) / 3); //gray scale value equals the average of RGB values

		}

	}
	return input;
}


void createImage(int* image, int width, int height, int index)
{
	System::Drawing::Bitmap MyNewImage(width, height);


	for (int i = 0; i < MyNewImage.Height; i++)
	{
		for (int j = 0; j < MyNewImage.Width; j++)
		{
			//i * OriginalImageWidth + j
			if (image[i*width + j] < 0)
			{
				image[i*width + j] = 0;
			}
			if (image[i*width + j] > 255)
			{
				image[i*width + j] = 255;
			}
			System::Drawing::Color c = System::Drawing::Color::FromArgb(image[i*MyNewImage.Width + j], image[i*MyNewImage.Width + j], image[i*MyNewImage.Width + j]);
			MyNewImage.SetPixel(j, i, c);
		}
	}
	MyNewImage.Save("..//Data//Output//outputRes" + index + ".png");
	cout << "result Image Saved " << index << endl;
}


int main()
{
	int ImageWidth = 4, ImageHeight = 4;

	int start_s, stop_s, TotalTime = 0;

	System::String^ imagePath;
	std::string img;
	img = "..//Data//Input//test.png";


	imagePath = marshal_as<System::String^>(img);
	int* imageData = inputImage(&ImageWidth, &ImageHeight, imagePath);
	//Here the clock will include the time of MPI Initialization which adds time to all processors
	start_s = clock();

	MPI_Init(NULL, NULL);
	//Here the clock starts after MPI Initialization which will lead to a much fewer time
	//start_s = clock();
	int myrank, mysize;

	MPI_Comm_size(MPI_COMM_WORLD, &mysize);
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	// Local dynamic array from "imageData" for each processor (To be assigned in Scatter)
	int* local_imageData = new int[(ImageHeight * ImageWidth)/mysize];

	//We are going to scale to 0-255 because maximum value in "imageData" was 255
	//So, we will count number of values of "imageData" which are 0-255 in the below dynamic array
	//This array is local for each process
	int* local_no_pixels = new int[256];
	for (int i = 0; i < 256; i++) {
		local_no_pixels[i] = 0;
	}

	//This array will contain the sums of all "local_no_pixels" of processros 
	int all_no_pixels[256] = { 0 };

	//This variable is for each processor, it will take its value Scattered from "all_no_pixels"
	int* local_counted_pixels = new int[256];
	for (int i = 0; i < 256; i++) {
		local_counted_pixels[i] = 0;
	}

	//Probability dynamic array for each processor that has probability of its assigned numbers
	double* local_probability = new double[256 / mysize];

	//All probability values will be Gathered in this static array
	double all_probability[256] = { 0 };

	//Cumulative Probability for each value (0-255)
	double all_cumulative_probability[256] = { 0 };

	//Local Cumulative dynamic array for each processor from "all_cumulative_probability" 
	//Each processor will Scale its array then Floor it
	double* local_cumulative = new double[256];

	//Local floor values for "local_cumulative" for each processor
	int* local_floor_round = new int[256 / mysize];

	//This array has all the Floors to values (0-255)
	//The array will be used to map the Original Image values to Floor Round values to have the Final Output Image Values in an array 
	//This array Gathers all "local_floor" arrays of all processors
	int floor_round[256] = { 0 };

	//														STEP 1
	//Count number of pixels associated with each pixel intensity

	//Here we will Scatter the Original Image's array values on all processors, each has array "local_imageData"
	MPI_Scatter(imageData, (ImageHeight*ImageWidth)/mysize , MPI_INT, local_imageData, (ImageHeight * ImageWidth) / mysize, MPI_INT, 0, MPI_COMM_WORLD);


	//Here each processor will count the number of values (0-255) in its "local_no_pixels" array
	//So, this means that in Index 0 of "local_no_pixels" we have how many times the number "0" was in "local_imageData" and 
	//so on for other numbers until 255
	for (int i = 0; i < (ImageHeight * ImageWidth) / mysize; i++)
		local_no_pixels[local_imageData[i]]++;

	//Now we need to SUM all "local_no_pixels" arrays of processors and reduce them in one array
	MPI_Reduce(local_no_pixels, &all_no_pixels, 256, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

	//We will Scatter Number of Pixels array to local arrays for each processor
	//Remember they are all 256 element which is the number we Scale to
	MPI_Scatter(&all_no_pixels, 256 / mysize, MPI_INT, local_counted_pixels, 256 / mysize, MPI_INT, 0, MPI_COMM_WORLD);

	//														STEP 2
	//Calculate Probability of each pixel intensity in the image matrix

	//Each processor will calculate the probability of its "local_no_pixels" in a "local_probability" array
	//Probability=Number of pixels/Total Number of Pixels
	//Total Number of Pixels is the Image's number of pixels which is (width X height)
	for (int i = 0; i < 256 / mysize; i++)
		local_probability[i] = (double)((double)local_counted_pixels[i] / (double)(ImageWidth * ImageHeight));

	//We will gather all "local_probability" arrays of processor in one array
	MPI_Gather(local_probability, 256 / mysize, MPI_DOUBLE, &all_probability, 256 / mysize, MPI_DOUBLE, 0, MPI_COMM_WORLD);


	//														STEP 3
	//Calculate Cumulative Probability

	//Since we have all probability values in one array, we will calculate all their cumulative values in one array also
	//All done by Processor 0
	//Note: Why didn't we do this in parallel and used one processor?
	//Because its faster to calculate Cumulative once by one processor, rather than all processors calculate the same cumulative value
	if (myrank == 0)
	{
		all_cumulative_probability[0] = all_probability[0];
		for (int i = 1; i < 256; i++)
			all_cumulative_probability[i] = all_probability[i] + all_cumulative_probability[i - 1];
	}

	//Scatter values of cumulative on processor, each processor will have the same number of values in its local array
	MPI_Scatter(&all_cumulative_probability, 256 / mysize, MPI_DOUBLE, local_cumulative, 256 / mysize, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	//														STEP 4
	//Change Intensity range to 0-255

	//Each processor will Scale the cumulative values by multiplying by 256
	//Each processor will put the value in its "local_probability" array
	//Then each processor will Floor the probability values in its "local_floor" array
	//Now each processor has "local_floor" array which is the mapping of numbers (0-255)
	for (int i = 0; i < 256 / mysize; i++)
	{
		local_probability[i] = local_cumulative[i] * 256;
		local_floor_round[i] = floor(local_probability[i]);
	}

	//We will Gather all Floor values from processors to be in "floor_round" array which will be used to map values from Input Image to Output Image
	MPI_Gather(local_floor_round, 256 / mysize, MPI_INT, floor_round, 256 / mysize, MPI_INT, 0, MPI_COMM_WORLD);


	//														STEP 5
	//Mapping the Original Image values to Floor Round values to have the Final Output Image
	//Here we will prepare "imageData" values to be mapped by values we have from "floor_round"
	//So then we convert "imageData" values to the Final Output Image using the given "Create Image" function
	//This is done better sequentially using one processor
	if (myrank == 0)
	{
		for (int i = 0; i < ImageHeight * ImageWidth; i++)
			imageData[i] = floor_round[imageData[i]];
	}
	
	stop_s = clock();
	TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;

	//Since "image_Data" array values is only at Processor 0 so it is the only one that can create the image
	if (myrank == 0)
		createImage(imageData, ImageWidth, ImageHeight, 1);

	cout << "Time of Processor " << myrank << ":" << TotalTime << endl;

	free(imageData);
	MPI_Finalize();
	return 0;
}



