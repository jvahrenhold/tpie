// Copyright (c) 1994 Darren Erik Vengroff
//
// File: test_ami_sort.cpp
// Author: Darren Erik Vengroff <darrenv@eecs.umich.edu>
// Created: 10/7/94
//
// A test for AMI_sort().

#define DEBUG_ASSERTIONS 1

#include <sys/types.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <iostream.h>
#include <fstream.h>
#include <strstream.h>
#include <assert.h>

// Get information on the configuration to test.
#include "app_config.h"
#include "parse_args.h"

// Define it all.
#include <ami_stream.h>
#include <ami_scan.h>
#include <ami_sort.h>
VERSION(test_ami_sort_cpp,"$Id: test_ami_sort.cpp,v 1.22 2002-01-03 18:20:15 tavi Exp $");

#include <ami_kb_sort.h>

// Utitlities for ascii output.
#include <ami_scan_utils.h>

#include "scan_random.h"
#include "scan_diff.h"
#include "merge_random.h"
#include "wall_timer.h"

static char def_srf[] = "/var/tmp/oss.txt";
static char def_rrf[] = "/var/tmp/osr.txt";

static char *sorted_results_filename = def_srf;
static char *rand_results_filename = def_rrf;

static bool report_results_random = false;
static bool report_results_sorted = false;

static bool sort_again = false;

static bool use_operator = false;

static bool kb_sort = false;

static const char as_opts[] = "R:S:rsaok";
void parse_app_opt(char c, char *optarg)
{
  switch (c) {
  case 'R':
    rand_results_filename = optarg;
  case 'r':
    report_results_random = true;
            break;
  case 'S':
    sorted_results_filename = optarg;
  case 's':
    report_results_sorted = true;
    break;
  case 'a':
    sort_again = !sort_again;
    break;
  case 'o':
    use_operator = !use_operator;
    break;
  case 'k':
    kb_sort = !kb_sort;
    break;
  }
}


int cc_int_cmp(CONST int &i1, CONST int &i2)
{
  return i1 - i2;
}

#if(0)
static void ___dummy_1() {
  AMI_STREAM<int> *s1 = NULL, *s2 = NULL;
    
  AMI_err ae;

  ae = AMI_sort(s1,s2,cc_int_cmp);
#if 0    
  ae = AMI_sort(s1,s2);
#endif
  ___dummy_1();
}
#endif


int
main(int argc, char **argv)
{
  wall_timer wt;
  long elapsed;
  AMI_err ae;
  
  parse_args(argc,argv,as_opts,parse_app_opt);

  if (verbose) {
    cout << "FLAGS: ";
#ifdef BTE_IMP_MMB
    cout << "BTE_IMP_MMB ";
#endif
#ifdef BTE_IMP_STDIO
    cout << "BTE_IMP_STDIO ";
#endif
#ifdef BTE_IMP_UFS
    cout << "BTE_IMP_UFS ";
#endif
#ifdef BTE_MMB_READ_AHEAD
    cout << "BTE_MMB_READ_AHEAD ";	  
#endif
    cout << "\n";
    cout << "test_size = " << test_size << ".\n";
    cout << "test_mm_size = " << test_mm_size << ".\n";
    cout << "random_seed = " << random_seed << ".\n";
  } else {
    cout << test_size << ' ' << test_mm_size << ' ' << random_seed << "\n";
  }
    
    // Set the amount of main memory:
  MM_manager.set_memory_limit (test_mm_size);
    
  AMI_STREAM<int> amis0;
  AMI_STREAM<int> amis1;
        
  // Write some ints.
  scan_random rnds(test_size,random_seed);
  
  ae = AMI_scan(&rnds, &amis0);
  assert(ae == AMI_ERROR_NO_ERROR);
  
  if (verbose) {
    cout << "Wrote the random values.\n";
    cout << "Stream length = " << amis0.stream_len() << '\n';
  }
  
  // Streams for reporting random vand/or sorted values to ascii
  // streams.
    
  ofstream *oss;
  cxx_ostream_scan<int> *rpts = NULL;
  ofstream *osr;
  cxx_ostream_scan<int> *rptr = NULL;
  
  if (report_results_random) {
    osr = new ofstream(rand_results_filename);
    rptr = new cxx_ostream_scan<int>(osr);
  }
  
  if (report_results_sorted) {
    oss = new ofstream(sorted_results_filename);
    rpts = new cxx_ostream_scan<int>(oss);
  }
  
  if (report_results_random) {
    ae = AMI_scan(&amis0, rptr);
    assert(ae == AMI_ERROR_NO_ERROR);
  }
  
  wt.start();
  
  if (kb_sort) {
    key_range range(KEY_MIN, KEY_MAX);
    ae = AMI_kb_sort(amis0, amis1, range);
    assert(ae == AMI_ERROR_NO_ERROR);
  } else {
    if (use_operator) {
      ae = AMI_sort(&amis0, &amis1);
      assert(ae == AMI_ERROR_NO_ERROR);
    } else {
      ae = AMI_sort(&amis0, &amis1, cc_int_cmp);
      assert(ae == AMI_ERROR_NO_ERROR);
    }
  }
  
  wt.stop();
  elapsed = wt.seconds();
  wt.reset();
  
  if (verbose) {
    cout << "Sorted them.\n";
    cout << "Sorted stream length = " << amis1.stream_len() << '\n';
    cout << "Time taken: " << elapsed << " seconds.\n";		
  } else {
    cout << "\ntest_size " << test_size << 
      " time_taken " << elapsed << "\n";		
  }
  
  if (report_results_sorted) {
    ae = AMI_scan(&amis1, rpts);
    assert(ae == AMI_ERROR_NO_ERROR);
  }
  
  cout << '\n';
  
  if (sort_again) {
    
    AMI_STREAM<int> amis2;
    AMI_STREAM<int> amis3;
    AMI_STREAM<scan_diff_out<int> > amisd;
    
    merge_random<int> mr;
    scan_diff<int> sd(-1);
    
    ae = AMI_generalized_partition_and_merge(&amis1, &amis2,
				 (merge_random<int> *)&mr);
    
    ae = AMI_sort_V1(&amis2, &amis3, cc_int_cmp);
    
    ae = AMI_scan(&amis1, &amis3, &sd, &amisd);
    
    if (verbose) {
      cout << "Length of diff stream = " <<
	amisd.stream_len() << ".\n";
    }
  }
  
  return 0;
}