#include <iostream>
#include <omp.h>
#include<time.h>

using namespace std;
#define N 10000	//Number of array elements
#define MOD 1000 //Max value for elements

void selection_sort_OMP(int *unsorted, int *sorted, int size){
#pragma omp parallel for num_threads(500) 
//parallel for directive distributes iterations of OUTER LOOP on threads
	for (int i = 0; i < size; i++){
		//Local variable for each thread to identify Correct place of its assigned elements in sorted array
		int local_index = 0;
		//Loop so that a thread compares its assigned elements with all elements of unsorted array
		//As long as the element is smaller its index to be put in sorted array is incremented
		for (int j = 0; j < size; j++){
			if (unsorted[j] < unsorted[i])
				local_index++;
		}
		//Put the [i] element in sorted array with its local index
		sorted[local_index] = unsorted[i];

        // printf("\n");
        // for (int x = 0; x < size; x++){
        //     printf("%d ",sorted[x]);
        // }
	}
}

//Selection_sort_OMP did the job of sorting the array, but it ignores duplicate value and leave their indexes
//which means that old values in defined sorted array(-1) will remain

//So, fill_duplicates() method for each old value (-1) replaces it with the value before it in the array, explained more 
//in the report.
void fill_duplicates(int *sorted_arr, int size){
	int duplicate = -1;
	for (int i = 0; i < size; i++){
		if (sorted_arr[i] != -1){
			duplicate = sorted_arr[i];
		}
		else{
			sorted_arr[i] = duplicate;
		}
	}
}

int main(){
	int unsorted_arr[N];

	//Filling unsorted array with values from bigger 
	//to smaller which will make sort function sort 
	//all of the array
	for(int i=0 ; i<N; i++)
    {
		unsorted_arr[i]=N-i;
    }

	int *sorted_arr=new int[N];		//Dynamic allocation of sorted array with its values being (-1)
    for (int i = 0; i < N; i++)
		sorted_arr[i]=-1;
	

	selection_sort_OMP(unsorted_arr, sorted_arr, N);	//Sorting array and leaving duplicates

	printf(" \n SORTED ARR \n ");
	for (int i = 0; i < 50; i++)
		printf(" %d ",sorted_arr[i]);

	//printf("\n SIZE of UNSORTED %d \n Size of sorted %d",unsorted_arr,sizeof(sorted_arr));
	return 0;
}