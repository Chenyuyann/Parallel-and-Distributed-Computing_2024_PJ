#include<stdio.h>
#include<omp.h>
#include<time.h>
#include<stdlib.h>
#include<algorithm>
#include<chrono>

using namespace std;
 
int Partition(int* data, int start, int end){
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
 
void quickSortPara(int* data, int start, int end){ // 并行快排
    if(start < end){
        int pos = Partition(data, start, end);
        #pragma omp task shared(data) firstprivate(start, pos)
        {
            quickSortPara(data, start, pos - 1);
        }
        #pragma omp task shared(data) firstprivate(end, pos)
        {
            quickSortPara(data, pos + 1, end);
        }
    }
}

void quickSort(int* data, int start, int end){ // 串行快排
    if(start < end){
        int pos = Partition(data, start, end);
        quickSort(data, start, pos - 1);
        quickSort(data, pos + 1, end);
    }
}
 
int main(int argc, char* argv[]){
    int n = atoi(argv[2]), i; // 线程数
    int size = atoi(argv[1]); // 数据大小
    int* num = (int*)malloc(sizeof(int) * size);
    int* check = (int*)malloc(sizeof(int) * size);
    
    srand(time(NULL) + rand());
    for(i = 0; i < size; i++){
        num[i] = rand();
        check[i] = num[i];
    }

    double start = omp_get_wtime();
    quickSort(check, 0, size - 1);
    double end = omp_get_wtime();
    printf("串行时间: %lfs\n", end - start);

    omp_set_num_threads(n); // 设置线程数
    double start_para = omp_get_wtime();
    #pragma omp parallel
    {
        #pragma omp single nowait
        {
            quickSortPara(num, 0, size - 1);
        }
    }
    double end_para = omp_get_wtime();

    // 检查排序结果是否正确
    bool flag = true;
    for(i = 0; i < size; i++){
        if(num[i] != check[i]){
            flag = false;
            printf("排序错误\n");
            return 0;
        }
    }
    if(flag) printf("排序正确\n");
 
    printf("并行时间: %lfs\n", end_para - start_para);
    return 0;
}