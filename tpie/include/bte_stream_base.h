//
// File: bte_stream_base.h (formerly bte_base_stream.h)
// Author: Darren Erik Vengroff <dev@cs.duke.edu>
// Created: 5/11/94
//
// $Id: bte_stream_base.h,v 1.1 2002-01-14 16:20:28 tavi Exp $
//
#ifndef _BTE_STREAM_BASE_H
#define _BTE_STREAM_BASE_H

// For sysconf() and getpagesize().
#include <unistd.h>

#include <persist.h>
// Get the BTE error codes.
#include <bte_err.h>
// Get statistics definitions.
#include <tpie_stats_stream.h>

// Include the registration based memory manager.
#define MM_IMP_REGISTER
#include <mm.h>

// Inline commonly called functions.
//#define B_INLINE 
#define B_INLINE inline

// Max length of a stream file name.
#define BTE_STREAM_PATH_NAME_LEN 128

#define BTE_STREAM_HEADER_MAGIC_NUMBER	0xFEDCBA

// BTE stream types passed to constructors.
enum BTE_stream_type {
    BTE_READ_STREAM = 1, // Open existing stream for reading.
    BTE_WRITE_STREAM,    // Open for read/writing. Create if non-existent.
    BTE_APPEND_STREAM,   // Open for writing at end. Create if needed.
    BTE_WRITEONLY_STREAM // Open only for writing (allows mmb optimization)
                         // (must be sequential write through whole file)
};

// BTE stream status. 
enum BTE_stream_status {
    BTE_STREAM_STATUS_NO_STATUS = 0,
    BTE_STREAM_STATUS_INVALID = 1,
    BTE_STREAM_STATUS_EOS_ON_NEXT_CALL,
    BTE_STREAM_STATUS_END_OF_STREAM
};

// BTE stream header info.
class BTE_stream_header {
public:
  
  // Unique header identifier. Set to BTE_STREAM_HEADER_MAGIC_NUMBER.
  unsigned int magic_number;
  // Should be 1 for current version.
  unsigned int version;
  // The type of BTE_STREAM that created this header. Not all types of
  // BTE's are readable by all BTE implementations. For example,
  // BTE_STREAM_STDIO streams are not readable by either
  // BTE_STREAM_UFS or BTE_STREAM_MMAP implementations. The value 0 is
  // reserved for the base class. Use numbers bigger than 0 for the
  // various implementations.
  unsigned int type;
  // The number of bytes in this structure.
  size_t header_length;
  // The size of each item in the stream.
  size_t item_size;
  // The size of a physical block on the device this stream resides.
  size_t os_block_size;
  // Size in bytes of each logical block, if applicable.
  size_t block_size;
  // (What exactly is this?)
  off_t item_logical_eof;
};

// A base class for the base class :). The role of this class is to
// provide global variables, accessible by all streams, regardless of
// template.
class BTE_stream_base_generic {
protected:
  static tpie_stats_stream gstats_;
  static int remaining_streams;
public:
  // The number of globally available streams.
  static int available_streams() { return remaining_streams; }
  // The global stats.
  static const tpie_stats_stream& gstats() { return gstats_; }
};

// An abstract class template which implements a single stream of objects 
// of type T within the BTE.  This is the superclass of all actual 
// implementations of streams of T within the BTE (e.g. mmap() streams, 
// UN*X file system streams, and kernel streams).
template<class T> class BTE_stream_base: public BTE_stream_base_generic {
protected:
  // The persistence status of this stream.
  persistence per;
  // The status (integrity) of this stream.
  BTE_stream_status status_;
  // How deeply is this stream nested.
  unsigned int substream_level;
  // Non-zero if this stream was opened for reading only.
  int r_only; 

  // Check the given header for reasonable values.
  int check_header(BTE_stream_header* ph);

  // Initialize the header with as much information as is known here.
  void init_header(BTE_stream_header* ph);

  inline BTE_err register_memory_allocation (size_t sz);
  inline BTE_err register_memory_deallocation (size_t sz);

public:
  BTE_stream_base() {};

  // Tell the stream whether to leave its data on the disk or not
  // when it is destructed.
  void persist (persistence p) { per = p; }
  // Return true if a read-only stream.
  bool read_only () const { return r_only; }
  // Inquire the status.
  BTE_stream_status status() const { return status_; }

  // Inquire the OS block size.
  size_t os_block_size () const;

#if BTE_VIRTUAL_BASE
    
    // A virtual psuedo-constructor for substreams.
    virtual BTE_err new_substream(BTE_stream_type st,
                                  off_t sub_begin, off_t sub_end,
                                  BTE_stream_base<T> **sub_stream) = 0;
    
    virtual B_INLINE BTE_err read_item(T **elt) = 0;

    virtual B_INLINE BTE_err write_item(const T &elt) = 0;

    // Query memory usage
    virtual BTE_err main_memory_usage(size_t *usage,
                                      MM_stream_usage usage_type) = 0;

    virtual off_t stream_len(void) = 0;

    virtual BTE_err name(char **stream_name) = 0;
    
    virtual BTE_err seek(off_t offset) = 0;

    virtual BTE_err truncate(off_t offset) = 0;
    
    virtual ~BTE_stream_base(void) {};

    virtual int available_streams(void) = 0;    

    virtual off_t chunk_size(void) = 0;

#endif // BTE_VIRTUAL_BASE
};

template<class T>
int BTE_stream_base<T>::check_header(BTE_stream_header* ph) {

  if (ph == NULL) {
    LOG_FATAL_ID ("Could not map header.");
    return -1;
  }

  if (ph->magic_number != BTE_STREAM_HEADER_MAGIC_NUMBER) {
    LOG_FATAL_ID ("header: magic number mismatch (expected/actual):");
    LOG_FATAL_ID (BTE_STREAM_HEADER_MAGIC_NUMBER);
    LOG_FATAL_ID (ph->magic_number);
    return -1;
  }

  if ((ph->version != 2) ||
      (ph->header_length != sizeof (*ph))) {
    LOG_FATAL_ID ("header: incorrect version (expecting 2).");
    return -1;
  }

  if (ph->type == 0) {
    LOG_FATAL_ID ("header: type is 0 (reserved for base class).");
    return -1;
  }

  if (ph->item_size != sizeof (T)) {
    LOG_FATAL_ID ("header: incorrect item size (expected/actual):");
    LOG_FATAL_ID (sizeof(T));
    LOG_FATAL_ID (ph->item_size);
    return -1;
  }

  if (ph->os_block_size != os_block_size()) {
    LOG_FATAL_ID ("header: incorrect OS block size (expected/actual):");
    LOG_FATAL_ID (os_block_size());
    LOG_FATAL_ID (ph->os_block_size);
    return -1;
  }

  return 0;
}

template<class T>
void BTE_stream_base<T>::init_header (BTE_stream_header* ph) {
  tp_assert(ph != NULL, "NULL header pointer");
  ph->magic_number = BTE_STREAM_HEADER_MAGIC_NUMBER;
  ph->version = 2;
  ph->type = 0; // Not known here.
  ph->header_length = sizeof(*ph);
  ph->item_size = sizeof(T);
  ph->os_block_size = os_block_size();
  ph->block_size = 0; // Not known here.
  ph->item_logical_eof = 0;
}

template<class T>
BTE_err BTE_stream_base<T>::register_memory_allocation (size_t sz) {
  MM_err mme;

  if ((mme = MM_manager.register_allocation (sz)) != MM_ERROR_NO_ERROR) {
    status_ = BTE_STREAM_STATUS_INVALID;
    LOG_FATAL_ID("Memory manager error in allocation.");
    return BTE_ERROR_MEMORY_ERROR;
  }

  return BTE_ERROR_NO_ERROR;
}

template<class T>
BTE_err BTE_stream_base<T>::register_memory_deallocation (size_t sz) {
  MM_err mme;

  if ((mme = MM_manager.register_deallocation (sz)) != MM_ERROR_NO_ERROR) {
    status_ = BTE_STREAM_STATUS_INVALID;
    LOG_FATAL_ID("Memory manager error in deallocation.");
    return BTE_ERROR_MEMORY_ERROR;
  }

  return BTE_ERROR_NO_ERROR;
}

template<class T>
size_t BTE_stream_base<T>::os_block_size () const {
#ifdef _SC_PAGE_SIZE
  return sysconf (_SC_PAGE_SIZE);
#else
  return getpagesize ();
#endif
}

#endif // _BTE_STREAM_BASE_H 