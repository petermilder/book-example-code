/*   
        Example program to create a binary file

        From "Getting Started with the Xilinx Zynq FPGA and Vivado" 
        by Peter Milder (peter.milder@stonybrook.edu)

        Copyright (C) 2019 Peter Milder

        This program is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>

// This program will create a binary file mydata.bin, which will hold 2^21 integers
// (8 MB).

int main() {
    FILE * outFile;

    // Open the output file
    outFile = fopen("mydata.bin", "wb");
    
    // For our test, let's choose 2^21 ints = 8 MB of data
    int numInts = (1<<21);
    
    // Allocate data in memory
    int* myData = (int*)malloc(sizeof(int)*numInts); 
    
    // Initialize the data to be 9000+i, where i increments
    if ((myData) && (outFile)) { 
        int i;
        for (i=0; i < numInts; i++)
            myData[i] = i + 9000;
        
        // write the array to the binary file
        fwrite(myData, sizeof(int), numInts, outFile);

        // close the file
        fclose(outFile);
    }
    else
        printf("ERROR\n");
    
    return 0;
    
}
