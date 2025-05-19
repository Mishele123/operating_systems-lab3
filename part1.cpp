#include <chrono>
#include "Matrix.h"



int main() 
{
    try
    {
        Matrix matrix1 = Matrix::readFromFile("../big_matrix1.bin");
        //matrix1.fillRandom(1, 5);
        Matrix matrix2 = Matrix::readFromFile("../big_matrix2.bin");
        //matrix2.fillRandom(1, 5);
        std::cout << "Matrix 1:" << std::endl;
        //matrix1.writeToFile("small_matrix1..bin");
        //matrix1.print();
        // Matrix m1 = Matrix::readFromFile("../matrix123");
        //matrix1.print();
        std::cout << "Matrix 2:" << std::endl;
        //matrix2.writeToFile("../matrix321.bin");
        //matrix2.print();
        
        auto start = std::chrono::high_resolution_clock::now();
        Matrix result1 = matrix1.multiply(matrix2);
        auto end = std::chrono::high_resolution_clock::now();
        auto time1 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "Matrix1 * Matrix2 (один поток) = " << time1.count() << " ms" << std::endl;

        // Многопоток
        auto start2 = std::chrono::high_resolution_clock::now();
        Matrix result2 = matrix1.parallMultiply(matrix2);
        auto end2 = std::chrono::high_resolution_clock::now();
        auto time2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);
        std::cout << "Matrix1 * Matrix2 (многопоточ) = " << time2.count() << " ms" << std::endl;

        result2.writeToFile("result2.bin");
        std::cout << "Result: " << std::endl;
        std::cout << "results written to file" << std::endl;

    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << std::endl;
        return -1;
    }
	return 0;
}