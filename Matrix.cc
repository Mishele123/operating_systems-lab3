#include "Matrix.h"



Matrix::Matrix(size_t n) : _size(n), _data(n * n, 0.0) {}


Matrix::Matrix(size_t n, double minValue, double maxValue) : _size(n), _data(n * n)
{
    fillRandom(minValue, maxValue);
}


Matrix::Matrix(const std::vector<double>& matrix) : _size(static_cast<size_t>(std::sqrt(matrix.size()))),  
_data(matrix) {}


void Matrix::fillRandom(double minVal, double maxVal)
{
    if (minVal > maxVal)
    {
        throw std::runtime_error("Error: minVal > maxVal");
    }
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dist(minVal, maxVal);
    for (auto& it : _data)
        it = dist(gen);
}

size_t Matrix::size() const
{
    return _size;
}

void Matrix::print() const 
{
    std::cout << "Matrix " << _size << "x" << _size << ":\n";
    for (size_t i = 0; i < _size; ++i) 
    {
        for (size_t j = 0; j < _size; ++j) 
        {
            std::cout << _data[i * _size + j] << "\t";
        }
        std::cout << "\n";
    }
}


void Matrix::writeToFile(const std::string& filename) const
{
    int file = check(open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644));
    if (file == - 1)
        throw std::runtime_error("Cannot open file for write");
    
    ssize_t bytesWritten = check(write(file, _data.data(), _size * _size * sizeof(double)));
    if (bytesWritten == -1 || static_cast<size_t>(bytesWritten) != _size * _size * sizeof(double))
    {
        close(file);
        throw std::runtime_error("cannot write data in file");
    }
    close(file);
}


Matrix Matrix::readFromFile(const std::string& filename)
{
    int file = check(open(filename.c_str(), O_RDONLY));
    if (file == -1)
        throw std::runtime_error("Cannot open file for read");
    
    struct stat fileStat;
    if (fstat(file, &fileStat))
    {
        close(file);
        throw std::runtime_error("fail to get file size");
    }
    off_t fileSize = fileStat.st_size;
    size_t matrixSize = static_cast<size_t>(std::sqrt(fileSize / sizeof(double)));

    if (matrixSize * matrixSize * sizeof(double) != static_cast<size_t>(fileSize)) 
    {
        close(file);
        throw std::runtime_error("Invalid file size for square matrix");
    }

    Matrix result(matrixSize);
    ssize_t bytesRead = check(read(file, result._data.data(), fileSize));
    if (bytesRead == -1 || static_cast<size_t>(bytesRead) != fileSize) 
    {
        close(file);
        throw std::runtime_error("Failed to read matrix from file");
    }

    close(file);
    return result;
}


double Matrix::operator()(size_t i, size_t j) const 
{
    if (i >= _size || j >= _size)
        throw std::out_of_range("matrix indices out of range");
    return _data[i * _size + j];
}


double& Matrix::operator()(size_t i, size_t j) 
{
    if (i >= _size || j >= _size) 
        throw std::out_of_range("matrix indices out of range");
    return _data[i * _size + j];
}


Matrix Matrix::multiply(const Matrix& other) const
{
    if (_size != other._size)
        throw std::runtime_error("size matrix1 != size matrix2 (Cannot multiplied)");
    Matrix result(_size);
    for (size_t i = 0; i < _size; ++i) 
    {
        for (size_t j = 0; j < _size; ++j) 
        {
            double sum = 0.0;
            for (size_t k = 0; k < _size; ++k)
                sum += (*this)(i, k) * other(k, j);
            result(i, j) = sum;
        }
    }
    return result;
}


Matrix Matrix::parallMultiply(const Matrix& other, size_t numThreads) const
{
    if (_size != other._size)
        throw std::invalid_argument("Matrix dimensions must agree for multiplication");

    if (numThreads == 0)
        throw std::invalid_argument("Number of threads must be positive");

    Matrix result(_size);
    if (_size == 0)
        return result;

    numThreads = std::min(numThreads, _size);
    std::vector<pthread_t> threads(numThreads);
    std::vector<ThreadData> threadData(numThreads);

    size_t rowsPerThread = _size / numThreads;
    size_t remainder = _size % numThreads;
    size_t start_row = 0;

    for (size_t i = 0; i < numThreads; i++)
    {
        size_t endRow = start_row + rowsPerThread + (i < remainder ? 1 : 0);

        threadData[i] = {this, &other, &result, start_row, endRow};

        if (pthread_create(&threads[i], nullptr, multiplyThread, &threadData[i]) != 0)
        {
            for (size_t j = 0; j < i; j++)
            {
                pthread_join(threads[j], nullptr);
            }
            throw std::runtime_error("Failed to create thread");
        }

        start_row = endRow;
    }

    for (size_t i = 0; i < numThreads; i++)
    {
        if (pthread_join(threads[i], nullptr) != 0)
        {
            throw std::runtime_error("Failed to join thread");
        }
    }

    return result;
}


void* Matrix::multiplyThread(void* arg) {
    ThreadData* data = static_cast<ThreadData*>(arg);
    
    for (size_t i = data->startRow; i < data->endRow; ++i) {
        for (size_t k = 0; k < data->a->size(); ++k) {
            double tmp = (*data->a)(i, k);
            for (size_t j = 0; j < data->a->size(); ++j) {
                (*data->result)(i, j) += tmp * (*data->b)(k, j);
            }
        }
    }
    return nullptr;
}