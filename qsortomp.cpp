#include<stdio.h>
#include<omp.h>
#include<time.h>
#include<stdlib.h>
 
int Partition(int* data, int start, int end)
{
    int temp = data[start];
    while(start < end){
        while(start < end && data[end] >= temp) end--;
        data[start] = data[end];
        while(start < end && data[start] <= temp) start++;
        data[end] = data[start];
    }
    data[start] = temp;
    return start;
}
 
void quickSort(int* data, int start, int end) // 并行快排
{
    if (start < end) {
        int pos = Partition(data, start, end);
        #pragma omp parallel sections // 设置并行区域
        {
            #pragma omp section
            quickSort(data, start, pos - 1);
            #pragma omp section
            quickSort(data, pos + 1, end);
        }
    }
}
 
int main(int argc, char* argv[])
{
    int n = atoi(argv[2]), i; // 线程数
    int size = atoi(argv[1]); // 数据大小
    int* num = (int*)malloc(sizeof(int) * size);
 
    double starttime = omp_get_wtime();
    srand(time(NULL) + rand());
    for (i = 0; i < size; i++)
        num[i] = rand();
    omp_set_num_threads(n); // 设置线程数
    quickSort(num, 0, size - 1);
    double endtime = omp_get_wtime();
 
    // for (i = 0; i < 10 && i<size; i++)
    //     printf("%d ", num[i]);
    printf("并行时间: %lfs\n", endtime - starttime);
    return 0;
}