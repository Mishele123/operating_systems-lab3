#include <iostream>
#include <vector>
#include <random>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "check.hpp"
#include <fstream>


struct ParallelSearchData 
{
    const std::vector<int>* data;
    int value;
    ssize_t result;
    size_t startIndex;
    size_t endIndex;
    pthread_spinlock_t* lock;
};


std::vector<int> readArrayFromFile(const std::string& filename)
{
    int file = check(open(filename.c_str(), O_RDONLY));
    if (file == -1)
        throw std::runtime_error("cannot open file");

    struct stat st;
    if (fstat(file, &st) != 0)
    {
        close(file);
        throw std::runtime_error("fail to get file size");
    }

    size_t fileSize = st.st_size;
    if (fileSize % sizeof(int) != 0) {
        close(file);
        throw std::runtime_error("file size is not a multiple of int size");
    }

    size_t elements = fileSize / sizeof(int);
    std::vector<int> buffer(elements);
    ssize_t bytesRead = check(read(file, buffer.data(), fileSize));
    if (bytesRead != static_cast<ssize_t>(fileSize))
    {
        close(file);
        throw std::runtime_error("Failed to read vector from file");
    }
    
    close(file);
    return buffer;
}


void fillFileWithRandomNumbers(const std::string& filename, size_t count)
{
    int file = check(open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644));
    if (file == -1)
        throw std::runtime_error("Cannot open file for writing");


    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, INT32_MAX);

    for (size_t i = 0; i < count; i++)
    {
        int number = dist(gen);
        ssize_t bytesWritten = check(write(file, &number, sizeof(int)));
        if (bytesWritten != sizeof(int))
        {
            close(file);
            throw std::runtime_error("Failed to write number to file");
        }
    }

    close(file);
}


int sequentialSearch(const std::vector<int>& data, int value) 
{
    for (size_t i = 0; i < data.size(); ++i) 
    {
        if (data[i] == value)
            return i;
    }
    return -1;
}


void* parallelSearchThread(void* arg) 
{
    ParallelSearchData* psd = static_cast<ParallelSearchData*>(arg);
    for (size_t i = psd->startIndex; i < psd->endIndex; i++) 
    {
        if ((*psd->data)[i] == psd->value)
        {
            pthread_spin_lock(psd->lock);
            if (psd->result == -1 || psd->result > i)
                psd->result = i;
            pthread_spin_unlock(psd->lock);
            break;
        }
    }
    return nullptr;
}


ssize_t parallelSearch(const std::vector<int>& data, int value, size_t numThreads = 4) 
{
    if (data.empty())
        return -1;
    
    pthread_spinlock_t spinLock;
    pthread_spin_init(&spinLock, 0);
    
    numThreads = std::min(numThreads, data.size());
    std::vector<pthread_t> threads(numThreads);
    std::vector<ParallelSearchData> threadData(numThreads);

    size_t chunkSize = data.size() / numThreads;
    for (size_t i = 0; i < numThreads; i++) 
    {
        threadData[i] = {
            &data,
            value,
            -1,
            i * chunkSize,
            (i == numThreads - 1) ? data.size() : (i + 1) * chunkSize,
            &spinLock
        };
        
        if (pthread_create(&threads[i], nullptr, parallelSearchThread, &threadData[i]) != 0) 
        {
            pthread_spin_destroy(&spinLock);
            throw std::runtime_error("Failed to create thread");
        }
    }

    for (auto& thread : threads)
        pthread_join(thread, nullptr);

    pthread_spin_destroy(&spinLock);
    int result = -1;
    for (const auto& td : threadData) 
        if (td.result != -1 && (result == -1 || td.result < result))
            result = td.result;

    return result;
}



int main()
{
    fillFileWithRandomNumbers("numbers.bin", 1'000'000);
    auto v = readArrayFromFile("numbers.bin");
    auto start = std::chrono::high_resolution_clock::now();
    ssize_t index = sequentialSearch(v, 149); //149
    auto end = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "(один поток) = " << time.count() << " us" << std::endl;

    auto start1 = std::chrono::high_resolution_clock::now();
    ssize_t index1 = parallelSearch(v, 149); //149
    auto end1 = std::chrono::high_resolution_clock::now();
    auto time1 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1);
    std::cout << "(многопоток) = " << time1.count() << " us" << std::endl;

    return 0;
}