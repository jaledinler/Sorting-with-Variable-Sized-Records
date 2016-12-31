/* Copyright (C) 2016 Jale Dinler*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "sort.h"

int comparator(const void *a, const void *b);

// compare function to be used in qsort
int
comparator(const void *a, const void *b) {
  const rec_dataptr_t **ptr1 = (const rec_dataptr_t **) a;
  const rec_dataptr_t **ptr2 = (const rec_dataptr_t **) b;
  return (*ptr1)->key - (*ptr2)->key;
}

/*write sorted records to a file. It takes the file name to be written, number of records that is going to be written in the file
and an array of rec_dataptr_t pointers. Each entry of array points to a rec_dataptr_t type defined in "sort.h".*/
void
writeToFile(rec_dataptr_t * records[], int numOfRecs, char *outputFile) {
  int fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
  if (fd < 0) {
      fprintf(stderr, "Error: Cannot open file %s\n", outputFile);
      exit(1);
}

  // output the number of keys as a header for this file
  int record = write(fd, &numOfRecs, sizeof (numOfRecs));
  if (record != sizeof (numOfRecs)) {
      perror("write");
      exit(1);
  }
  rec_data_t rec_data;
  int i, j, data_size;

  // for each record, write its key,data size and data to file
  for (i = 0; i < numOfRecs; i++) {
      rec_data.key = records[i]->key;
      rec_data.data_ints = records[i]->data_ints;

      for (j = 0; j < rec_data.data_ints; j++) {
          rec_data.data[j] = *((records[i]->data_ptr) + j);
      }
      data_size = 2 * sizeof (unsigned int) +
        rec_data.data_ints * sizeof (unsigned int);

      record = write(fd, &rec_data, data_size);

      if (record != data_size) {
          perror("write");
          exit(1);
      }
    }

  (void) close(fd);
}

/*Main function. I read the record file in main and sort them by using qsort, then call the function writeToFile to write sorted
records to a file. I use an array of rec_dataptr_t pointers, arrOfRecs, to store the records. At the beginning, before reading the records, 
I dynamically allocate space for the array since we know the size of rec_dataptr_t and how may records we have. I also dynamically allocate 
space for data of each record before reading the records. I have an array of unsigned int pointers,data_arr, to keep the data_ptr for each record's data. 
When I read a record, first I read it as a rec_nodata type to get its key and its data_ints. After getting data_ints, I allocate space for each entry j of 
data_arr and then start reading data_ints many data to the adresses data_arr[j],data_arr[j]+1,...,data_arr[j]+data_ints-1 to keep track of the data of the 
record j. Then I copy everything to a rec_dataptr_t type record and store its adress in arrOfRecs. After I am done with reading, I call qsort function to sort 
the records and then write them to a file by calling writeToFile. Then I free the heap space I allocated during reading. */
int
main(int argc, char *argv[]) {
  if (argc != 5) {
      fprintf(stderr, "Usage: varsort -i inputfile -o outputfile\n");
      exit(1);
  }
  char *inFile;
  char *outFile;
  int c;

  // check for the valid arguments and flags
  while ((c = getopt(argc, argv, "i:o:")) != -1) {
    switch (c) {
    case 'i':
      inFile = strdup(optarg);
      break;
    case 'o':
      outFile = strdup(optarg);
      break;
    default:
      fprintf(stderr, "Usage: varsort -i inputfile -o outputfile\n");
      exit(1);
    }
  }
  // check if all the arguments valid
  if (optind != 5) {
      fprintf(stderr, "Usage: varsort -i inputfile -o outputfile\n");
      exit(1);
  }
  // check if there is no arguments
  if (optind == 0) {
      fprintf(stderr, "Usage: varsort -i inputfile -o outputfile\n");
      exit(1);
  }
  // check if there is more arguments than it was supposed to
  int fd = open(inFile, O_RDONLY);
  if (fd < 0) {
      fprintf(stderr, "Error: Cannot open file %s\n", inFile);
      exit(1);
  }

  int numOfRecs;
  int record;
  rec_nodata_t rec_nodata;
  rec_dataptr_t rec_dataptr;
  int j, i;

  record = read(fd, &numOfRecs, sizeof (numOfRecs));
  if (record != sizeof (numOfRecs)) {
      perror("read");
      exit(1);
  }
  // allocate space for array of rec_dataptr_t pointers
  rec_dataptr_t **arrOfRecs = malloc (numOfRecs * sizeof (rec_dataptr_t *));
  if (arrOfRecs == NULL) {
      fprintf(stderr, "failed to allocate memory.\n");
      return -1;
  }
  // allocate space for array of unsigned int pointers.
  // This array is for storing the data_ptr of each record.
  unsigned int **data_arr = malloc (numOfRecs * sizeof (unsigned int *));
  if (data_arr == NULL) {
      fprintf(stderr, "failed to allocate memory.\n");
      return -1;
  }
  // read the key and data_ints of record
  for (j = 0; j < numOfRecs; j++) {
      record = read(fd, &rec_nodata, sizeof (rec_nodata_t));
      if (record != sizeof (rec_nodata_t)) {
          perror("read");
          exit(1);
      }

      assert(rec_nodata.data_ints <= MAX_DATA_INTS);
      // allocate space for data_ptr of record
      data_arr[j] = malloc (rec_nodata.data_ints*sizeof (unsigned int));

      if (data_arr[j] == NULL) {
          fprintf(stderr, "failed to allocate memory.\n");
          return -1;
      }
      // Read the data of the record
      record = read(fd, data_arr[j],
         rec_nodata.data_ints*sizeof (unsigned int));
      // copy everything to rec_dataptr_t type record and
      // store its address in arrOfRecs
      rec_dataptr.data_ptr = data_arr[j];
      rec_dataptr.key = rec_nodata.key;
      rec_dataptr.data_ints = rec_nodata.data_ints;
      arrOfRecs[j] = malloc (sizeof (rec_dataptr_t));
      *arrOfRecs[j] = rec_dataptr;
    }

  // close the file
  (void) close(fd);

  // sort records with respect to their keys in ascending order
  qsort(arrOfRecs, numOfRecs, sizeof (rec_dataptr_t *), comparator);
  // write sorted records to file
  writeToFile(arrOfRecs, numOfRecs, outFile);

  // free heap space
  for (i = 0; i < numOfRecs; i++) {
      free(data_arr[i]);
      free(arrOfRecs[i]);
  }
  free(data_arr);
  free(arrOfRecs);
  free(outFile);
  free(inFile);
  return 0;
}
