#include <bits/stdc++.h>
#include <sys/time.h>
#include <algorithm>
#include "mpi.h"

using namespace std;

void partialSort(int *arr, int *eachGroup, int groupSize, int *pivots, int numProc){
    MPI_Scatter(arr, groupSize, MPI_INT, eachGroup, groupSize, MPI_INT, 0, MPI_COMM_WORLD);
    sort(eachGroup, eachGroup + groupSize); // 每个进程局部排序
    for(int i = 0; i < numProc; ++i){ // 每个进程选numProc个主元
        pivots[i] = eachGroup[i*groupSize/numProc];
    }
}

void pivotPatition(int *pivots, int *eachGroup, int groupSize, int *partSize, int numProc, int myRank){
    int *gatherPivots = (int *)malloc(sizeof(int) * numProc * numProc);
    int *newPivots = (int *)malloc(sizeof(pivots[0]) * (numProc - 1));
    // 每个进程选出的主元发送给0号进程
    MPI_Gather(pivots, numProc, MPI_INT, gatherPivots, numProc, MPI_INT, 0, MPI_COMM_WORLD);
    if(myRank == 0){
        sort(gatherPivots, gatherPivots+numProc*numProc); // numProc*numProc个主元排序
        for(int i = 0; i < numProc-1; ++i){ // 选出numProc-1个主元
            newPivots[i] = gatherPivots[(i+1)*numProc-1];
        }
    }
    // 0号进程广播numProc-1个主元
    MPI_Bcast(newPivots, numProc-1, MPI_INT, 0, MPI_COMM_WORLD);
    // 根据主元，每个进程被划分成numProc段，记录每一段的长度
    int index = 0;
    for(int i = 0; i < groupSize; ++i){
        while(index < numProc-1 && eachGroup[i] > newPivots[index]) index++;
        if(index == numProc-1){
            partSize[index] = groupSize - i;
            break;
        }
        else{
            partSize[index]++;
        }
    }
    free(gatherPivots);
    free(newPivots);
}

void globalExchange(int *eachGroup, int groupSize, int *partSize, int *newPartSize, int **recvArr, int numProc){
    MPI_Alltoall(partSize, 1, MPI_INT, newPartSize, 1, MPI_INT, MPI_COMM_WORLD); // 进行全局的通信，在一个处理器中获得每一个处理器分给他处理的元素个数

    int totalSize = 0;
    for(int i = 0; i < numProc; ++i){ // 计算每个进程接收到的数据总量
        totalSize += newPartSize[i];
    }
    *recvArr = (int *)malloc(sizeof(int) * totalSize);
    int *sendPos = (int *)malloc(sizeof(int) * numProc); // 记录每个进程发送的数据的起始位置
    int *recvPos = (int *)malloc(sizeof(int) * numProc); // 记录每个进程接收的数据的起始位置
    sendPos[0] = 0;
    recvPos[0] = 0;
    for(int i = 1; i < numProc; ++i){
        sendPos[i] = sendPos[i-1] + partSize[i-1];
        recvPos[i] = recvPos[i-1] + newPartSize[i-1];
    }
    MPI_Alltoallv(&(eachGroup[0]), partSize, sendPos, MPI_INT, *recvArr, newPartSize, recvPos, MPI_INT, MPI_COMM_WORLD);
    free(sendPos);
    free(recvPos);
}

void mergeSort(int *recvArr,int *newPartSize,int numProc,int myRank,int *newArr){
    int *sortedSubList,*recv,*index,*partEnd,*subListSize;
    int totalSize=0;
    index = (int *)malloc(numProc * sizeof(int));
    partEnd = (int *)malloc(numProc * sizeof(int));
    
    subListSize = (int *)malloc(numProc * sizeof(int));
    recv = (int *)malloc(numProc * sizeof(int));

    index[0] = 0;
    totalSize = newPartSize[0];
    for(int i = 1; i < numProc; i++)
    {
        totalSize += newPartSize[i];
        index[i] = index[i-1] + newPartSize[i-1]; // 计算各处理器中的数据的numProc个子列表的起始索引
        partEnd[i-1] = index[i]; // 记录各个分区结束位置
    }
    sortedSubList = (int *)malloc(totalSize * sizeof(int));
    partEnd[numProc-1] = totalSize;
    // 各个处理器归并排序
    for(int i = 0; i < totalSize; i++)
    {
        int min = INT_MAX;
        int mark;
        for(int j = 0; j < numProc; j++)
        {
            if(index[j] < partEnd[j] && min > recvArr[index[j]])
            {
                min = recvArr[index[j]];
                mark = j;
            }
        }
        sortedSubList[i] = min;
        index[mark]++;
    }
    MPI_Gather(&totalSize, 1, MPI_INT, subListSize, 1, MPI_INT, 0, MPI_COMM_WORLD); // 把每个处理器的数据数量收集到subListSize数组
    if(myRank == 0)
    {
        recv[0] = 0;
        for(int i = 1; i < numProc; i++)
        {
            recv[i] = recv[i-1] + subListSize[i-1]; // 计算每段接收的偏移量
        }
    }
    MPI_Gatherv(sortedSubList, totalSize, MPI_INT, newArr, subListSize, recv, MPI_INT, 0, MPI_COMM_WORLD);//收集到newArr中
    free(partEnd);
    free(sortedSubList);
    free(index);
    free(subListSize);
    free(recv);
}

int main(int argc, char *argv[]){
    int myRank, numProc, addnum, groupSize;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    MPI_Comm_size(MPI_COMM_WORLD, &numProc);
    int size = atoi(argv[1]);
    int *arr = (int *)malloc(sizeof(int) * size);
    int *realArr = (int *)malloc(sizeof(int) * size);
    int *pivots = (int *)malloc(sizeof(int) * numProc);
    int *partSize = (int *)malloc(sizeof(int) * numProc);
    int *newPartSize = (int *)malloc(sizeof(int) * numProc);
    int *recvArr;
    srand(time(NULL));
    // 主进程初始化数组并串行排序进行对照
    if(myRank == 0){
        for(int i = 0; i < size; ++i){
            arr[i] = rand();
            realArr[i] = arr[i];
        }
        double starttime = MPI_Wtime();
        sort(realArr, realArr+size);
        double endtime = MPI_Wtime();
        printf("串行时间: %lfs\n", endtime - starttime);
        // 更新size的值，保证可以被numProc整除
        int i;
        for(i = size; i%numProc; ++i) arr[i]=INT_MIN;
        addnum = i-size;
        size = i;
    }

    MPI_Barrier(MPI_COMM_WORLD); // 同步所有的MPI进程
    double starttime = MPI_Wtime();
    for(int i = 0; i < numProc; i++)
    {
        partSize[i] = 0;
    }
    groupSize = size / numProc;
    int *eachGroup = (int *)malloc(sizeof(int) * groupSize);
    int *newArr = (int *)malloc(sizeof(int) * size);
    MPI_Barrier(MPI_COMM_WORLD);

    partialSort(arr, eachGroup, groupSize, pivots, numProc);
    if(numProc > 1){
        pivotPatition(pivots, eachGroup, groupSize, partSize, numProc, myRank);
        globalExchange(eachGroup, groupSize, partSize, newPartSize, &recvArr, numProc);
        mergeSort(recvArr, newPartSize, numProc, myRank, newArr);
    }
    if(myRank == 0){
        for(int i = addnum; i < size; ++i) newArr[i-addnum] = newArr[i];
        double endtime = MPI_Wtime();
        printf("并行时间: %lfs\n", endtime - starttime);
        // 检查结果是否正确
        bool flag = true;
        for(int i = 0; i < size-addnum; ++i){
            if(newArr[i] != realArr[i]){
                flag = false;
                printf("Error! %d is wrong!\n", i);
                break;
            }
        }
        if(flag) printf("Correct!\n");
    }
    free(arr);
    free(realArr);
    free(eachGroup);
    free(pivots);
    free(partSize);
    free(newPartSize);
    free(recvArr);
    free(newArr);
    MPI_Finalize();
    return 0;
}
