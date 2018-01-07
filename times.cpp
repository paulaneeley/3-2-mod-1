#include <fstream>
#include <iostream>
#include <vector>
#include <exception>
#include <stdlib.h>
#include <math.h>
#include <chrono>
#include <ctime>

//////////////////////////////////////////////////////////////////////////////////////////////////
// This program is an arbitrary precision package specially tailored to handle the (3/2)^n      //
// sequence by implementing a technique called variable length quantity. In our package, we     //
// concatenate 63 bits of every 64 bit unsigned long integer to form the current value of       //
// (3/2)^n. The 64th bit is used to determine when we might "overflow" a 64 bit unsigned long   //
// integer.                                                                                     //
//                                                                                              //
// Usage: times <number-of-iterations> <bins-file> < state-file>                                //
//                                                                                              //
// The bins-file represents the number of elements in each bin for the current set of           //
// iterations. Therefore, if running multiple instances of the program over different ranges    //
// to parallelize, the bins-files will need to be saved during each instance to a different     //
// file and then merged at the end.                                                             //
//////////////////////////////////////////////////////////////////////////////////////////////////


// Global variable declarations

// Number of most significant fractional digits to be binned for each (3/2)^n
int noOfDigits = 10;
// Number of bins
int noOfBins = pow(2, noOfDigits);
// Vector to hold bins 
std::vector<int> bins;
// Vector to hold the current value of (3/2)^n
std::vector<unsigned long> value;
// Mask representing the 64th bit of an unsigned long integer
unsigned long topValue = 0x8000000000000000;


// To calculate running time for the program
std::chrono::high_resolution_clock::time_point startTime;
using namespace std::chrono;

//////////////////////////////////////////////////////////////////////////////////////////////////
// These functions save and restore the state-file and bins-file of the program. The function   //
// saveState prints the vector "value" to a file every 100,000 iterations. The function         //
// saveBins prints the number of elements in each bin to a file every 100,000 iterations. The   //
// function restoreState reads in the state-file that saveState writes.                         //
//////////////////////////////////////////////////////////////////////////////////////////////////


char* outBins = NULL;
char* outState = NULL;

void saveBins() {
    std::ofstream results;
    results.open(outBins);
    for (int index = 0; index < noOfBins; index++)
        results << bins[index] << std::endl;
    results.close();
}

void saveState(int rp) {
    std::ofstream state;
    state.open(outState);
    state << rp << std::endl;
    for (int index = 0; index < value.size(); index++)
        state << value[index] << std::endl;
    state.close();
    }

int restoreState() {
    std::ifstream state;
    state.open(outState);
    std::string line;

    int rp = 0;
    int lineNo = 0;
    while (getline(state, line)) {
        switch (lineNo) {
            case 0: rp = stoi(line); break;
            default: value.push_back(stoul(line)); break;
        }
        lineNo++;
    }
    state.close();
    return rp;
    }


//////////////////////////////////////////////////////////////////////////////////////////////////
// This function returns the bin number that the current data point fits in by selecting the    //
// most significant bits (having length "noOfDigits") for frac(3/2)^n. Bit 63 of each vector    //
// element "wordNum" is not included in this calculation as it serves as a special marker bit   //
// and does not contain a value for the calculation.                                            //
//////////////////////////////////////////////////////////////////////////////////////////////////

int bin(int rp) {
    static unsigned long mask = pow(2, noOfDigits) - 1;
    // Vector element containing all bits or the most significant bits of frac(3/2)^n
    int wordNum = rp / 63;
    // Most significant bit in the vector wordNum
    int bitNum  = rp % 63;
    
    int shiftBits = bitNum - (noOfDigits - 1);
    unsigned long shiftMask = 0;
    if (shiftBits >= 0)
        shiftMask = mask << shiftBits;
    else
        shiftMask = mask >> abs(shiftBits);
    
    unsigned long result = 0;
    if (shiftBits >= 0)
        result = (value[wordNum] & shiftMask) >> shiftBits;
    else
        result = (value[wordNum] & shiftMask) << abs(shiftBits);
    
    // Some of the low order bits of the fraction that identifies the bin number 
    // may be in the previous vector element, as indicated when shiftBits < 0. 
    // In this case, we must extract it from the most significant bits of the 
    // previous vector element.
    if (shiftBits < 0)
        try {
            unsigned long lowerWordShiftBits = shiftBits + 63;
            shiftMask = mask << lowerWordShiftBits;
            result |= (value[wordNum - 1] & shiftMask) >> (63 + shiftBits);
        } catch (std::exception ex) {
        }
    
    return (int) result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// This function generates (3/2)^n for each iteration. Working its way up from the least        //
// significant word of the vector "value" to the most significant word, this function performs  //
// 2 arithmetic operations on each element in the value vector: a shift operation and an add    //
// operation. The arithmetic shift operation sets the shifted bit when overflow of the 63 bit   //
// vector element occurs, and the arithmetic add operation sets the carry bit when overflow of  //
// the 63 bit vector element occurs. A new word is added to the vector when either the carry or //
// shift bit indicates.                                                                         //
//////////////////////////////////////////////////////////////////////////////////////////////////

void generate(int start, int count) {
    for (int rp = start; rp < count; rp++) {

        // Print time elapsed every 100,000 iterations
        if ((rp % 100000) == 0) {
            high_resolution_clock::time_point current = high_resolution_clock::now();
            duration<double> time_span = duration_cast<duration<double> >(current - startTime);
            std::cout << rp << ": " << time_span.count() << std::endl;
        }

        // We seed the value vector with an initial value, hence we calculate it's bin
        // number at the start of the loop.
        bins[bin(rp)] += 1;
        
        int carryBit = 0;
        int shiftedBit = 0;
        
        for (size_t index = 0; index < value.size(); index++) {
            // build the shifted value
            unsigned long shiftedValue = (value[index] << 1) + shiftedBit;
            // if the shifted value is greater than topValue then we adjust the shifted value
            // but keep track of the fact that we lost the most significant bit
            if (shiftedValue >= topValue) {
                shiftedValue = shiftedValue ^ topValue;
                shiftedBit = 1;
            } else {
                shiftedBit = 0;
            }
            
            // add the shifted value to the current value including a carry from the last value
            value[index] += shiftedValue + carryBit;
            // if the added value is greater than topValue then we adjust the current value
            // but keep track of the fact that we have a carry for the next value
            if (value[index] >= topValue) {
                // get rid of the 64th bit which we now know must be '1'
                value[index] = value[index] ^ topValue;
                carryBit = 1;
            } else
                carryBit = 0;
        }
        
        // Start a new word
        if ((shiftedBit == 1) || (carryBit == 1)) {
            value.push_back(shiftedBit + carryBit);
        }

        if (((rp + 1) % 100) == 0) {
            saveState(rp + 1);
            saveBins();
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cout << "error: you need a value to run this program" << std::endl;
        exit(1);
    }

    // Bins file (write only)
    outBins = argv[2];
    // States file (read/write)
    outState = argv[3];
    
    startTime = std::chrono::high_resolution_clock::now();

    for (int index = 0; index < noOfBins; index++)
        bins.push_back(0);

    // radix point
    int rp = restoreState();
    if (rp == 0) {
        value.push_back(3);
    }
    
    // Number of iterations from starting point
    generate(rp, rp + atoi(argv[1]));
    high_resolution_clock::time_point current = high_resolution_clock::now();
    duration<double> time_span = duration_cast<duration<double> >(current - startTime);
    std::cout << argv[1] << ": " << time_span.count() << std::endl;

    saveBins();
}

