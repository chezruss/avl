/*
 * Modifications Copyright (c) 2024 Russell A. Brown
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Top-down red-black tree test program
 *
 * To build the test executable, compile via:
 * 
 * g++ -std=c++11 -O3 -o test_tdrbTree test_tdrbTree.cpp
 * 
 * The tdrbTree.h file describes compilation options.
 * 
 * Usage:
 * 
 * test_tdrbTree [-k K] [-i I]
 * 
 * where the command-line options are interpreted as follows.
 * 
 * -k The number of keys to insert into the TDRB tree
 * 
 * -i The number of times to iterate the test
 */

#include "tdrbTree.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <stdexcept>
#include <utility>
#include <vector>

/*
  * Calculate the mean and standard deviation of the elements of a vector.
  *
  * Calling parameter:
  *
  * vec - a vector
  * 
  * return a pair that contains the mean and standard deviation
  */
 template <typename T>
std::pair<double, double> calcMeanStd(std::vector<T> const& vec) {
  double sum = 0, sum2 = 0;
  for (size_t i = 0; i < vec.size(); ++i) {
    double v = static_cast<double>(vec[i]);
    sum += v;
    sum2 += v * v;
  }
double n = static_cast<double>(vec.size());
return std::make_pair(sum / n, sqrt((n * sum2) - (sum * sum)) / n);
}

int main(int argc, char **argv) {
    
    using std::cout;
    using std::endl;
    using std::ostringstream;
    using std::runtime_error;
    using std::setprecision;
    using std::shuffle;
    using std::string;
    using std::vector;

    int iterations = 1;
    int keys = 4194304;

    // Parse the command-line arguments.
    for (size_t i = 1; i < argc; ++i) {
        if (0 == strcmp(argv[i], "-k") || 0 == strcmp(argv[i], "--keys")) {
            keys = atol(argv[++i]);
            if (keys <= 0) {
                ostringstream buffer;
                buffer << "\n\nnodes = " << keys << "  <= 0" << endl;
                throw runtime_error(buffer.str());
            }
            continue;
        }
        if (0 == strcmp(argv[i], "-i") || 0 == strcmp(argv[i], "--iterations")) {
            iterations = atol(argv[++i]);
            if (iterations <= 0) {
                ostringstream buffer;
                buffer << "\n\niterations = " << iterations << "  <= 0" << endl;
                throw runtime_error(buffer.str());
            }
            continue;
        }
        {
            ostringstream buffer;
            buffer << "\n\nillegal command-line argument: " << argv[i] << endl;
            throw runtime_error(buffer.str());
        }
    }

    // Create vectors to store the execution times and rotations for each iteration.
    vector<double> insertTime(iterations), searchTime(iterations), deleteTime(iterations);
    vector<size_t> sri(iterations), dri(iterations), sre(iterations), dre(iterations);
    vector<size_t> ri(iterations), re(iterations);

    // Create a vector of unique unsigned integers as large as keys.
    vector<uint32_t> numbers(keys);
    for (size_t i = 0; i < keys; ++i) {
        numbers[i] = i;
    }

    // Prepare to shuffle the vector of integers.
    std::mt19937_64 g(std::mt19937_64::default_seed);

    // Create an RB tree that has integer keys and preallocate its freed list.
    tdrbTree<uint32_t> root;
    root.freedPreallocate( keys, 0 );
#ifndef DISABLE_FREED_LIST
    if ( root.freedSize() != keys ) {
        ostringstream buffer;
        buffer << endl << "freed list size following pre-allocate = " << root.freedSize()
               << "  != number of keys = " << keys << endl;
        throw runtime_error(buffer.str());
    }
#endif

    // Build and test the TDRB tree.
    size_t treeSize;
    for (size_t it = 0; it < iterations; ++it) {

        // Reset the rotation counters to 0.
        root.singleRotationCount = root.doubleRotationCount = 0;

        // Shuffle the keys and add each key to the TDRB tree.
        shuffle(numbers.begin(), numbers.end(), g);
        auto startTime = std::chrono::steady_clock::now();
        for (size_t i = 0; i < numbers.size(); ++i) { 
            if ( root.insert( numbers[i] ) == false) {
                ostringstream buffer;
                buffer << endl << "key " << numbers[i] << " is already in tree for insert" << endl;
                throw runtime_error(buffer.str());
            }
        }
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        insertTime[it] = static_cast<double>(duration.count()) / 1000000.;

        // Record rotation counts for insertion.
        sri[it] = root.singleRotationCount;
        dri[it] = root.doubleRotationCount;
        // A double rotation also counts 2 single rotations
        ri[it] = root.singleRotationCount - 2*root.doubleRotationCount;

        // Verify that the correct number of nodes were added to the TDRB tree.
        treeSize = root.size();
        if (treeSize != numbers.size()) {
            ostringstream buffer;
            buffer << endl << "expected size for tree = " << treeSize
                   << " differs from actual size = " << numbers.size() << endl;
            throw runtime_error(buffer.str());
        }

        // Verify that each path to the bottom of the string tree has an equal
        // number of BLACK nodes and check the validity of the tree.
        root.checkTree();

        // No need to reshuffle the keys prior to searching the TDRB tree
        // for each key because search does not rebalance the tree and
        // hence the insertion order of the keys is irrelevant to search.
        startTime = std::chrono::steady_clock::now();
        for (size_t i = 0; i < numbers.size(); ++i) {
            if ( root.contains( numbers[i] ) == false ) {
                ostringstream buffer;
                buffer << endl << "key " << numbers[i] << " is not in tree for contains" << endl;
                throw runtime_error(buffer.str());
            }
        }
        endTime = std::chrono::steady_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        searchTime[it] = static_cast<double>(duration.count()) / 1000000.;

        // Reset the rotation counters to 0.
        root.singleRotationCount = root.doubleRotationCount = 0;

        // Reshuffle the keys prior to deleting each key from the TDRB tree
        // because deletion rebalances the tree and hence the insertion
        // order of the keys may influence the performance of deletion.
        shuffle(numbers.begin(), numbers.end(), g);
        startTime = std::chrono::steady_clock::now();
        for (size_t i = 0; i < numbers.size(); i++) {
            if ( root.erase( numbers[i] ) == false ) {
                ostringstream buffer;
                buffer << endl << "key " << numbers[i] << " is not in tree for erase" << endl;
                throw runtime_error(buffer.str());
            }
        }
        endTime = std::chrono::steady_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        deleteTime[it] = static_cast<double>(duration.count()) / 1000000.;

       // Record rotation counts for deletion.
        sre[it] = root.singleRotationCount;
        dre[it] = root.doubleRotationCount;
        // A double rotation also counts 2 single rotations
        re[it] = root.singleRotationCount - 2*root.doubleRotationCount;
        
        // Verify that the TDRB tree is empty.
        if ( root.empty() == false ) {
            ostringstream buffer;
            buffer << endl << root.size() << " nodes remain in tree following erasure" << endl;
            throw runtime_error(buffer.str());
        }

        // Check the size of the freed list.
#ifndef DISABLE_FREED_LIST
        if ( root.freedSize() != keys ) {
            ostringstream buffer;
            buffer << endl << "freed list size following erasure = " << root.freedSize()
                << "  != number of keys = " << keys << endl;
            throw runtime_error(buffer.str());
        }
#endif
    }

    // Report statistics including means and standard deviations.
    cout << endl << "node size = " << root.nodeSize()
         << " bytes\tnumber of keys in tree = " << treeSize
         << "\titerations = " << iterations << endl << endl;
    
    auto timePair = calcMeanStd<double>(insertTime);
    cout << "insert time = " << setprecision(4) << timePair.first
         << "\tstd dev = " << timePair.second << " seconds" << endl;
         
    timePair = calcMeanStd<double>(searchTime);
    cout << "search time = " << setprecision(4) << timePair.first
         << "\tstd dev = " << timePair.second << " seconds" << endl;
         
    timePair = calcMeanStd<double>(deleteTime);
    cout << "delete time = " << setprecision(4) << timePair.first
         << "\tstd dev = " << timePair.second << " seconds" << endl << endl;

    timePair = calcMeanStd<size_t>(ri);
    cout << "insert single rotate = " << static_cast<size_t>(timePair.first)
         << "\tstd dev = " << static_cast<size_t>(timePair.second);

    timePair = calcMeanStd<size_t>(dri);
    cout << "\tdouble rotate = " << static_cast<size_t>(timePair.first)
         << "\tstd dev = " << static_cast<size_t>(timePair.second);

    timePair = calcMeanStd<size_t>(sri);
    cout << "\ttotal rotate = " << static_cast<size_t>(timePair.first)
         << "\tstd dev = " << static_cast<size_t>(timePair.second) << endl << endl;

    timePair = calcMeanStd<size_t>(re);
    cout << "delete single rotate = " << static_cast<size_t>(timePair.first)
         << "\tstd dev = " << static_cast<size_t>(timePair.second);

    timePair = calcMeanStd<size_t>(dre);
    cout << "\tdouble rotate = " << static_cast<size_t>(timePair.first)
         << "\tstd dev = " << static_cast<size_t>(timePair.second);

    timePair = calcMeanStd<size_t>(sre);
    cout << "\ttotal rotate = " << static_cast<size_t>(timePair.first)
         << "\tstd dev = " << static_cast<size_t>(timePair.second) << endl << endl;

    // Clear the TDRB tree.
    root.clear();

    return 0;
}