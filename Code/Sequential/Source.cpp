#include <iostream>
#include <math.h>
#include <stdlib.h>
#include<string.h>
#include<msclr\marshal_cppstd.h>
#include <ctime>// include this header 
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

    start_s = clock();

    //Array to count Number of Pixels' Intensities
    int number_of_pixels[256];
    for (int i = 0; i < 256; i++) {
        number_of_pixels[i] = 0;
    }

    //Array for probability of each pixel intensity
    double probability[256];

    //Array for Cumulative Values of Probabilities
    double cumulative_probability[256];

    //Array to Scale "cumulative_probability" to 0-255
    double scaled_cumulative_probability[256];

    //Array to Floor "scaled_cumulative_probability" values
    int floored_round[256];

    //                                                              STEP 1
    //Count number of pixels associated with each pixel intensity 
    for (int i = 0; i < (ImageWidth * ImageHeight); i++)
        number_of_pixels[imageData[i]]++;






    //                                                              STEP 2
    //Calculate Probability of each pixel intensity in the image matrix
    for (int i = 0; i < 256; i++)
        probability[i] = (double)((double)number_of_pixels[i] / (double)(ImageWidth * ImageHeight));

    //                                                              STEP 3
    //Calculate Cumulative Probability
    cumulative_probability[0] = probability[0];
    for (int i = 1; i < 256; i++)
        cumulative_probability[i] = probability[i] + cumulative_probability[i - 1];

    //                                                              STEP 4
    //Change Intensity range to 0-255
    //Scaling to 0-255 & Flooring
    for (int i = 0; i < 256; i++)
    {
        scaled_cumulative_probability[i] = cumulative_probability[i] * 256;
        floored_round[i] = floor(scaled_cumulative_probability[i]);
    }

    //                                                              STEP 5
    //Mapping the Original Image values to Floor Round values to have the Final Output Image
    //Here we will prepare "imageData" values to be mapped by values we have from "floored_round"   
    for (int i = 0; i < ImageWidth * ImageHeight; i++)
        imageData[i] = floored_round[imageData[i]];

    stop_s = clock();
    TotalTime += (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000;

    //Converting "imageData" values to the Final Output Image using the given "Create Image" function
    createImage(imageData, ImageWidth, ImageHeight, 1);
    cout << "Time Taken By Sequential Code: " << TotalTime << endl;

    free(imageData);
    system("pause");
    return 0;
}