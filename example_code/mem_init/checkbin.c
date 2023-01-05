/*   
        Example program to read a binary file and check it

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

int main() {
    FILE * inFile;
    // open the output file
    inFile = fopen("mydataout.bin", "rb");
    
    int numInts = (1<<22);
    
    // Allocate data in memory
    int* myData = (int*)malloc(sizeof(int)*numInts); 
    
    int errors = 0;

    // write the data
    fread(myData, sizeof(int), numInts, inFile);

    // close the file
    fclose(inFile);

    if ((myData) && (inFile)) { 
        int i;
        for (i=0; i < numInts/2; i++)
            if (myData[i] != i + 9000)
                errors++;

        for (i=numInts/2; i < numInts; i++)
            if (myData[i] != i - numInts/2 + 9027)
                errors++;


        printf("%d errors detected\n", errors);
    }
    else
        printf("ERROR\n");

    return 0;
    
}
