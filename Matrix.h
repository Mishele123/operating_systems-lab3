#include <iostream>
#include <vector>
#include <random>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "check.hpp"


class Matrix
{
private:
    size_t _size;
    std::vector<double> _data;

    struct ThreadData
    {
        const Matrix* a;
        const Matrix* b;
        Matrix* result;
        size_t startRow;
        size_t endRow;
    };
    static void* multiplyThread(void* arg);

public:
    Matrix(size_t n);
    Matrix(size_t n, double minValue, double maxValue);
    Matrix(const std::vector<double>& matrix);
    void fillRandom(double minVal, double maxVal);
    static Matrix readFromFile(const std::string& filename);
    void writeToFile(const std::string& filename) const;
    void print() const;
    size_t size() const;
    Matrix multiply(const Matrix& other) const;
    Matrix parallMultiply(const Matrix& other, size_t numThreads = 4) const;
    double operator()(size_t i, size_t j) const;
    double& operator()(size_t i, size_t j);


};
