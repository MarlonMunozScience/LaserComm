// LaserCommNoise.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream> //For reading files
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>

// Overloading << Operator to print everything the vector
template <typename S>
std::ostream& operator<<(std::ostream& os,
    const std::vector<S>& vector)
{
    // Printing all the elements
    // using <<
    for (auto element : vector) {
        os << element << " ";
    }
    return os;
}

// Opens the provided file with error handling
// Returns vector of signal photons
std::vector<int> openfile(std::string filename) {
    std::ifstream inputFile(filename);
    
    if (!inputFile.is_open()) {
        std::cerr << "Unable to open file :C";
    }
    
    std::vector<int> signalPhotons;

    int element;

    //Reads elements from the file and appends them to the vector
    while (inputFile >> element) {
        signalPhotons.push_back(element);
    }
    

    return signalPhotons;
}

//Takes in ASCII vector, converts it to binary
std::vector<int> ASCIItoBinary(std::vector<int> signalPhotons) {
    // ASCII vectors come in form of <number of zeros> <number of signal photons>
    std::vector<int> signalBinary;

    int count = 0; // Counter to help us keep track if we are counting zeros or ones
    
    for (auto element : signalPhotons) {
        // for zeros
        if (count == 0)
        {
            for (int i = 0; i < element; i++) {
                signalBinary.push_back(0);
            }
            count = 1; //switching for next case
        }
        else // for signal photons
        {
            for (int i = 0; i < element; i++) {
                signalBinary.push_back(1);
            }
            count = 0; // switching for the next case
        }
    }

    return signalBinary;

}

// Introduces erasures by turning ones into zeros
std::vector<int> signalErasure(std::vector<int> signalOriginal, double erasure_probability) {
    // Index through each element of the original signal, roll a random number and if it is higher, erase the value
   // To safe time, we only have to do this with values that are 1.
    
    // Providing a seed value
    srand((unsigned)time(NULL));

    int counter = 0; //used to index elements later
    int debug_count = 0;
    //Looping through the original signal
    for (auto element : signalOriginal) {
        if (element == 1) { // only worried about slots with signal photons

            // Generate a random number between 0 and 100
            int random = rand() % 100;

            // Convert our probaility to an int
            int probability_int = (int)(erasure_probability * 100);

            if (random >= probability_int) {
                signalOriginal[counter] = 0;

                debug_count++;
            }

        }
        counter++;
    }
    std::cout << debug_count << std::endl;
    return signalOriginal;
}

// Introduces noise by turning zeros into ones
std::vector<int> signalNoise(std::vector<int> signalOriginal, double noise_probability) {
    // Index through each element of the original signal, roll a random number and if it is higher, add noise
   // To safe time, we only have to do this with values that are 0.

    // Providing a seed value
    srand((unsigned)time(NULL)); 

    int counter = 0; //used to index elements later
    int debug_count = 0;
    //Looping through the original signal
    for (auto element : signalOriginal) {
        if (element == 0) { // only worried about slots with signal photons

            // Generate a random number between 0 and 100
            int random = rand() % 100;
            std::cout << random << " ";
            // Convert our probaility to an int
            int probability_int = (int)(noise_probability * 100);

            if (random >= probability_int) {
                signalOriginal[counter] = 1;
                debug_count++;
            }

        }
        counter++;
    }
    std::cout << debug_count << std::endl;
    return signalOriginal;
}
    

int main()
{
    double erasure_prob = 0.5;
    double noise_prob = 0.5;
    std::cout << "Hello World!\n";
    std::string fname = R"(C:\Users\marlo\Desktop\Code\LaserCommNoise\test.txt)";
    
    std::vector<int> signal = openfile(fname);

    //Printing all the elements
    std::cout << signal << std::endl;

    std::cout << "File open complete" << std::endl;

    std::vector<int> signalBinary = ASCIItoBinary(signal);

    std::cout << signalBinary << std::endl;

    std::vector<int> signalErasured = signalErasure(signalBinary, erasure_prob);

    std::cout << "\n" << signalErasured << std::endl;

    std::vector<int> signalNoised = signalNoise(signalErasured, noise_prob);

    std::cout << "\n" << signalNoised << std::endl;

    return 0;
}

