// Starshot Transport Layer Simulator (STLS)
//
// Simple PHY that converts pulses to Poisson-distributed photon counts:
//     stls_pulse_to_photons_poisson.c
//
// Parses a data file of pulses and, for each slot containing a pulse, compute
//     a Poisson-distributed random number based on a specified mean (Lambda)
//     and write that into the corresponding location of the output file.
//     The input file can be binary or ACSII, and compressed or uncompressed.
//     The output file will be of the same format (i.e. binary vs ASCII,
//     compressed vs uncompressed) as the input file.
//
// Inputs:
//    Files: [infilename].pulses.bin or [infilename].pulses.txt
//        or [infilename].pulses.rle.bin or [infilename].pulses.rle.txt
//    Parameters:
//        - compressed flag: whether the input file is compressed with run-length encoding
//        - ASCII flag: whether the input file is ASCII text (assumes binary by default)
// Outputs:
//    Files: [outfilename].photons.bin or [outfilename].photons.txt
//        or [outfilename].photons.rle.bin or [outfilename].photons.rle.txt
//    Console:
//        Aany error messages, confirmation of successful completion.
//
// Notes:
//    Uncompressed input contains only 16-bit 0 or 1 (binary case) or characters "0" or "1" (ASCII case).
//    Compressed input contains binary 16-bit integers according to RLE algorithm (binary case)
//         or ASCII representations of 16-bit integers (ASCII case).
//
// Revision History:
//     29/08/2022        Ian Morrison        Original version
//
// To Do:
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "rle.h"
#include "poisson.h"

#define DEFAULT_MEAN_DETECTED_PHOTONS 0.2           // default value for the mean photon count per incident pulse

#define COMPRESSED_BUFFER_SIZE_IN_WORDS 1000        // number of 16-bit words to nominally be buffered on the input
                                                    // - long simulations will be broken into multiple runs
                                                    //   that don't exceed this length
                                                    // - can be longer if on a run of (2^16-1) values at the nominal size

#define UNCOMPRESSED_BUFFER_SIZE_IN_SLOTS 100000000 // allow an expansion factor of ~100,000 when uncompressing (a guess)
                                                    // - cannot exceed 2^32 or de-compression function will fail

void usage ()
{
    printf("stls_pulse_to_photons_poisson [options] infilename outfilename\n"
           "\n"
           "  Convert pulses to photon counts\n"
           "\n"
           "  -a          reads an ASCII text input file (default is a binary file)\n"
           "  -c          assumes compressed input when flag present [default is uncompressed]\n"
           "  -h          display this usage information\n"
           "  -k          mean number of detected photons in a slot per incident pulse (default is 0.2)\n"
           "\n"
           );
}


int main(int argc, char **argv)
{
    int ascii = 0;                    // when set, input file is human readable ASCII (default = 0)
    int compressed = 0;               // when set, input file is compressed (default = 0)
    int loop_count = 0;               // counter that increments each main loop
    int character;                    // a place to store an input ASCII character
    uint8_t byte;                     // a place to store an input binary byte
    int num_read;                     // number of items read/scanned
    int eof_flag = 0;                 // flag set when EOF reached
    uint32_t slots_this_loop;         // number of slots read in during current loop (uncompressed cases)
    uint32_t words_this_loop;         // number of words read in during current loop (compressed cases)
    unsigned long total_slots = 0;    // total number of slots processed
    unsigned long occupied_slots = 0; // number of slots with a 1 (occupied with a pulse or at least one photon)
    unsigned long total_writes = 0;   // tally of the total number of values written to the output file
    uint32_t * uncompressed_buffer;   // pointer to array for storage of raw or uncompressed data
    uint32_t * uncompressed_pointer;  // pointer to current location in the uncompressed buffer
    uint16_t * compressed_buffer;     // pointer to array of data that has been run-length encoded for compression
    uint16_t * compressed_pointer;    // pointer to current location in the compressed buffer
    FILE * in_fp;                     // the input file pointer
    FILE * out_fp;                    // the output file pointer
    char * infilename;                // the filename for the input file
    char * outfilename;               // the filename for the output file
    uint32_t *histogram;              // pointer to table accumulating photon count stats
    int hist_index;                   // index into histogram table
    int erasures = 0;                 // count of erasures across the whole input data set
    double mean_detected_photons;     // the desired mean number of detected photons per incident pulse
    mean_detected_photons = (double)DEFAULT_MEAN_DETECTED_PHOTONS;

    // parse command line options
    int arg = 0;
    while ((arg = getopt(argc, argv, "achk:")) != -1)
    {
        switch (arg)
        {
            case 'a':
                ascii = 1;
                break;

            case 'c':
                compressed = 1;
                break;

            case 'h':
                usage();
                exit(0);

            case 'k':
                mean_detected_photons = atof(optarg);
                break;

            default:
                usage();
                exit(0);
        }
    }

    // should be two command line arguments
    if (argc-optind != 2)
    {
        printf("\nERROR: 2 arguments are required: the input and output filenames\n");
        usage();
        exit(0);
    }

    infilename = malloc(100);   // allow filename up to 100 chars
    if (sscanf(argv[optind], "%s", infilename) != 1)
    {
        fprintf(stderr, "\nERROR: could not parse filename from %s\n", argv[optind]);
        exit(0);
    }

    outfilename = malloc(100);   // allow filename up to 100 chars
    if (sscanf(argv[optind+1], "%s", outfilename) != 1)
    {
        fprintf(stderr, "\nERROR: could not parse filename from %s\n", argv[optind+1]);
        exit(0);
    }

    if (ascii)
        in_fp = fopen(infilename, "r");
    else
        in_fp = fopen(infilename, "rb");

    if (in_fp == NULL)
    {
        printf("\nError opening input file %s\n", infilename);
        exit(0);
    }

    if (ascii)
        out_fp = fopen(outfilename, "w");
    else
        out_fp = fopen(outfilename, "wb");

    if (out_fp == NULL)
    {
        printf("\nError opening output file %s\n", outfilename);
        exit(0);
    }

    // display all selections
    printf("\nProcessing pulse data from input file %s\n", infilename);
    printf("Writing photon count data to output file %s\n", outfilename);
    if (ascii)
        printf("Input file is assumed to be ASCII text, and output file will be the same\n");
    else
        printf("Input file is assumed to be binary, and output file will be the same\n");
    if (compressed)
        printf("Input file is assumed to be compressed, and output file will be the same\n\n");
    else
        printf("Input file is assumed to be uncompressed, and output file will be the same\n");
    printf("The specified mean number of detected photons per incident pulse = %f\n\n", mean_detected_photons);

    // some memory alocations
    compressed_buffer = malloc((int)COMPRESSED_BUFFER_SIZE_IN_WORDS*2*sizeof(uint16_t));  // double it to allow for extra (2^16-1) values
    uncompressed_buffer = malloc((int)UNCOMPRESSED_BUFFER_SIZE_IN_SLOTS*sizeof(uint32_t));
    histogram = calloc(21,sizeof(uint32_t));      // allow from 0 to 20 photons in a slot (should rarely exceed 3)

    // set input parameter to poisson() function - which expects L = exp(-lambda) (= the erasure rate)
    double L = exp(-mean_detected_photons);


    // begin processing

    // if input is compressed, uncompress into buffer, otherwise store directly into uncompressed buffer

    // outer loop until EOF

    do   // process successive input buffer's worth of data until whole file done
    {
        loop_count++;
        printf("Loop %d\n", loop_count);

        slots_this_loop = 0;  // reset slot and word counts
        words_this_loop = 0;

        // fill the input buffer and process that data
        if (compressed)   // compressed case
        {
            compressed_pointer = compressed_buffer;  // reset pointer to start of buffer

            // read until EOF or filled a buffer's worth, but past any (2^16-1)s
            do
            {
                uint16_t this_zero_run_length;
                uint16_t this_pulse_flag;
                // read in a run length (or set of values making up a long run length)
                do
                {
                    this_zero_run_length = 0;
                    if (ascii)
                        num_read = fscanf(in_fp,"%hu", &this_zero_run_length);
                    else
                        num_read = fread((void *)(&this_zero_run_length), 2, 1, in_fp);   // read in a single word
                    if (num_read != 1)    // if can't read a number, assume we've hit EOF
                    {
                        printf("Reached end of input file\n");
                        eof_flag = 1;
                    }
                    if ((num_read == 1) && (this_zero_run_length > 65535))
                    {
                        printf("\nERROR: invalid content in input file\n\n");
                        exit(0);
                    }
                    if (num_read == 1)    // valid case
                    {
                        *compressed_pointer++ = this_zero_run_length;
                        words_this_loop++;
                    }
                }
                while((this_zero_run_length == 65535) && (eof_flag == 0));

                // read in a pulse flag (so long as we haven't hit EOF)
                if (eof_flag == 0)
                {
                    this_pulse_flag = 0;
                    if (ascii)
                        num_read = fscanf(in_fp,"%hu", &this_pulse_flag);
                    else
                        num_read = fread((void *)(&this_pulse_flag), 2, 1, in_fp);   // read in a single word
                    if (num_read != 1)    // if can't read a number, assume we've hit EOF
                    {
                        printf("\nERROR: this shouldn't happen - odd word count in input?\n\n");
                        exit(0);
                    }
                    if ((num_read == 1) && (this_pulse_flag != 0) && (this_pulse_flag != 1))   // only expect 0 or 1 (pulses = 1)
                    {
                        printf("\nERROR: invalid content in input file: %hu\n\n", this_pulse_flag);
                        exit(0);
                    }
                    if ((num_read == 1) && (this_pulse_flag == 0))    // valid case - end of a compressed block ending with zeros
                    {
                        *compressed_pointer++ = this_pulse_flag;
                        words_this_loop++;
                    }
                    if ((num_read == 1) && (this_pulse_flag == 1))    // valid case - a pulse - write it to the slot
                    {
                        *compressed_pointer++ = this_pulse_flag;
                        occupied_slots++;   // always a 1 here indicates an occupied slot
                        words_this_loop++;
                    }
                }
            }
            while ((words_this_loop < (uint32_t)COMPRESSED_BUFFER_SIZE_IN_WORDS) && (eof_flag == 0));

            printf("words this loop = %u\n", words_this_loop);

            // uncompress the buffer
            slots_this_loop = run_length_decode(compressed_buffer, uncompressed_buffer, words_this_loop);

            printf("slots this loop = %u\n", slots_this_loop);
        }
        else    // uncompressed case
        {
            uncompressed_pointer = uncompressed_buffer;  // reset write pointer to start of buffer
            // read until EOF or filled a buffer's worth
            if (ascii)   // ASCII case
            {
                do
                {
                    character = getc(in_fp);
                    if (character == EOF)
                    {
                        printf("\nReached end of input file\n\n");
                        eof_flag = 1;
                    }
                    else
                    {
                        slots_this_loop++;
                        if (character == '0') *uncompressed_pointer++ = 0;
                        else if (character == '1')
                        {
                            *uncompressed_pointer++ = 1;
                            occupied_slots++;
                        }
                        else
                        {
                            printf("\nERROR: invalid content in input file\n\n");
                            exit(0);
                        }
                    }
                }
                while((slots_this_loop < (uint32_t)UNCOMPRESSED_BUFFER_SIZE_IN_SLOTS) && (eof_flag == 0));
            }
            else   // binary case
            {
              do
              {
                  num_read = fread((void *)(&byte), 1, 1, in_fp);   // read in a single byte
                  if (num_read != 1)    // if can't read one byte, assume we've hit EOF
                  {
                      //printf("\nReached end of input file\n\n");
                      eof_flag = 1;
                  }
                  else
                  {
                      slots_this_loop++;
                      if (byte == 0) *uncompressed_pointer++ = 0;
                      else if (byte == 1)
                      {
                          *uncompressed_pointer++ = 1;
                          occupied_slots++;
                      }
                      else
                      {
                          printf("\nERROR: invalid content in input file\n\n");
                          exit(0);
                      }
                  }
              }
              while((slots_this_loop < (uint32_t)UNCOMPRESSED_BUFFER_SIZE_IN_SLOTS) && (eof_flag == 0));
            }
        }

        // tally up total slots
        total_slots += slots_this_loop;

        // process the pulse data of one uncompressed buffer's worth of input
        for (int i = 0; i<slots_this_loop; i++)
        {
            if (uncompressed_buffer[i] == 1)
            {
                uncompressed_buffer[i] = poisson(L);
                hist_index = uncompressed_buffer[i];
                if (hist_index > 20) hist_index = 20;   // cap at 20
                histogram[hist_index]++;
                if (hist_index == 0) erasures++;     // here was an occupied slot but zero photons were detected
            }
        }

#if 1
        // write out photon count data, in the same format as the input data
        if (compressed)  // compressed case
        {
            uint32_t num_compressed_words = run_length_encode(uncompressed_buffer, compressed_buffer, slots_this_loop);

            printf("writing %u compressed words to output file\n", num_compressed_words);

            if (ascii)  // ASCII text file case
            {
                compressed_pointer = compressed_buffer;
                uint16_t compressed_value;
                for (uint32_t word=0; word<num_compressed_words; word++)
                {
                    compressed_value = *compressed_pointer++;
                    fprintf(out_fp, "%u ", compressed_value);
                }
            }
            else   // binary file case
                fwrite((const void *)compressed_buffer, sizeof(uint16_t), (size_t)num_compressed_words, out_fp);
        }
        else  // uncompressed case
        {
            printf("writing %u slots to output file\n", slots_this_loop);
            uncompressed_pointer = uncompressed_buffer;
            if (ascii)  // ASCII text file case
            {
                uint32_t uncompressed_value;
                for (uint32_t slot=0; slot<slots_this_loop; slot++)
                {
                    uncompressed_value = *uncompressed_pointer++;
                    fprintf(out_fp, "%u ", uncompressed_value);
                }
            }
            else   // binary file case
            {
                for (uint32_t slot=0; slot<slots_this_loop; slot++)
                {
                    uint8_t byte = (uint8_t)(*uncompressed_pointer++);
                    fwrite((const void *)(&byte), 1, 1, out_fp);  // write a single byte
                }
            }
            total_writes += slots_this_loop;
        }
#endif

    }
    while (eof_flag == 0);


    // if not compressed, check expected number of writes were performed
    if (!compressed)
    {
        if (total_writes != total_slots)
            printf("ERROR: Expected to fill %lu slots, but actually wrote %lu to file\n", total_slots, total_writes);
        else
            printf("\nWrote a total of %lu slots to file\n", total_writes);
    }


    // processing done

    // calculate stats and write to screen
    double average_power = (double)occupied_slots/(double)total_slots;
    double peak_power = 1.0;    // the power of an occupied slot
    double peak_to_average_power = peak_power/average_power;
    printf("\nTotal slots processed = %lu\n", total_slots);
    printf("Those occupied with a pulse = %lu (fraction = %f)\n", occupied_slots, average_power);
    printf("Peak-to-Average Power Ratio = %f\n", peak_to_average_power);
    printf("\nExpected erasure rate = %f\n", L);
    printf("Measured erasure rate = %f\n", (double)erasures/(double)occupied_slots);
    printf("Histogram of photon counts:\n");
    printf("  count     number\n");
    for (int j=0; j<=20; j++)
        printf("   %d       %d\n", j, histogram[j]);

    // close the input and output files
    fclose(in_fp);
    fclose(out_fp);

    // free allocated memory
    free(infilename);
    free(outfilename);
    free(compressed_buffer);
    free(uncompressed_buffer);
    free(histogram);

    // all done
    printf("\nDone!\n\n");

    return(1);
}
