
/*
TODO:

Handle error correction code for command line arguments 
split code into .h and .cpp
Adjust probabilites to work on number ~ 10^-5

*/



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

            if (random < probability_int) {
                signalOriginal[counter] = 0;

                debug_count++;
            }

        }
        counter++;
    }
    
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
            
            // Convert our probaility to an int
            int probability_int = (int)(noise_probability * 100);

            if (random < probability_int) {
                signalOriginal[counter] = 1;
                debug_count++;
            }

        }
        counter++;
    }
    
    return signalOriginal;
}
    

std::vector<int> BinarytoASCII(std::vector<int> BinaryVector) {
    int counter = 0; // Internal Counter

    std::vector<int> BinaryOutput; // Output Vector
    for (auto element : BinaryVector){
        if (element == 0) {
            counter++;
        }
        else {
            BinaryOutput.push_back(counter);
            BinaryOutput.push_back(1);
            counter = 0;
        }
    }

    return BinaryOutput;
}

int main(int argc, char** argv)
{
    // Script expects 3 arguments: Name of noise file in 'Noise' folder, erasure probability, and noise probability 
    
    // If user does not input correct amount of commands
    // Handling each input
    for (int i = 0; i < argc; i++) {
        if (std::string(argv[i]) == "-h") {
            std::cout << "This code inserts noise and erasures into a ASCII file.";
            std::cout << "The input file must be placed in the 'Noise' Folder" << std::endl;
            std::cout << "Command Line arguments are: \n" << std::endl;
            std::cout << "[Name of Input] [Erasure Probability] [Noise Probability]" << std::endl;
            return 0;
        }
    }
    if (argc != 4) {
        std::cout << "You have entered an incorrect amount of arguments. Use -h for help." << std::endl;
        return 0;
    }


    // Input file 
    std::string input_file = argv[1];

    // Erasure Probability
    double erasure_prob = std::stod(argv[2]);

    // Noise probability
    double noise_prob = std::stod(argv[3]);

    // DEBUGGING: printing out command line arguments to see if they worked
    std::cout << input_file << ", " << erasure_prob << ", " << noise_prob << std::endl;

    // Opening file
    std::string fname = R"(Noise\test.txt)";
    std::vector<int> signal = openfile(fname);

    //Printing all the elements
    std::cout << signal << std::endl;


    std::vector<int> signalBinary = ASCIItoBinary(signal);
    std::cout << signalBinary << std::endl;

    std::vector<int> signalErasured = signalErasure(signalBinary, erasure_prob);
    std::cout << "\n" << signalErasured << std::endl;

    std::vector<int> signalNoised = signalNoise(signalErasured, noise_prob);
    std::cout << "\n" << signalNoised << std::endl;

    // Turning ASCII into Binary
    std::vector<int> outputBinary = BinarytoASCII(signalNoised);
    std::cout << "Binary Output\n" << outputBinary << std::endl;

    //Writes the vector to an output file
    std::ofstream outfile("output.txt");
    outfile << outputBinary << std::endl;
    outfile.close();


    return 0;
}

