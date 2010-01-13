// -*- mode: c++; tab-width: 4; indent-tabs-mode: t; eval: (progn (c-set-style "stroustrup") (c-set-offset 'innamespace 0)); -*-
// vi:set ts=4 sts=4 sw=4 noet :
// Copyright 2010, The TPIE development team
// 
// This file is part of TPIE.
// 
// TPIE is free software: you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the
// Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
// 
// TPIE is distributed in the hope that it will be useful, but WITHOUT ANY
// WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
// License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with TPIE.  If not, see <http://www.gnu.org/licenses/>

///////////////////////////////////////////////////////////////////////////
/// \file tpie/stream.h Declares TPIE streams.
///////////////////////////////////////////////////////////////////////////
#ifndef _TPIE_AMI_STREAM_H
#define _TPIE_AMI_STREAM_H


// Include the configuration header.
#include <tpie/config.h>
#include <tpie/persist.h>
#include <tpie/file_stream.h>
#include <tpie/stats_stream.h>


//TODO THESE SHOULD NOT BE HERE
#include <tpie/tpie_assert.h>
#include <cstring>
#include <vector>

// Get the error codes.
#include <tpie/err.h>

#include <tpie/tempname.h>

namespace tpie {

namespace ami {

/** AMI stream types passed to constructors */
enum stream_type {
	READ_STREAM = 1,	// Open existing stream for reading
	WRITE_STREAM,		// Open for writing.  Create if non-existent
	APPEND_STREAM,		// Open for writing at end.  Create if needed.
	READ_WRITE_STREAM	// Open to read and write.
};
	
/**  AMI stream status. */
enum stream_status {
	/** Stream is valid */
	STREAM_STATUS_VALID = 0,
	/** Stream is invalid */
	STREAM_STATUS_INVALID
};


////////////////////////////////////////////////////////////////////////////////
/// A Stream<T> object stores an ordered collection of objects of
/// type T on external memory. 
/// \anchor stream_types The  type of a Stream indicates what
/// operations are permitted on the stream. 
/// Stream types provided in TPIE are the following:
///
/// \anchor READ_STREAM \par READ_STREAM: 
/// Input operations on the stream are permitted, but output is not permitted.
/// 
/// \anchor WRITE_STREAM \par WRITE_STREAM: 
/// Output operations are permitted, but input operations are not permitted. 
/// 
/// \anchor APPEND_STREAM \par APPEND_STREAM: 
/// Output is appended to the end of the stream. Input operations are not
/// permitted. This is similar to WRITE_STREAM except that if the stream is
/// constructed on a file containing an existing stream,
/// objects written to the stream will be appended at the end of the stream.
///
/// \anchor READ_WRITE_STREAM \par READ_WRITE_STREAM:
/// Both input and output operations are permitted.
////////////////////////////////////////////////////////////////////////////////
template<class T> 
class stream {
public:
	typedef T item_type;
    
    // We have a variety of constructors for different uses.

    ////////////////////////////////////////////////////////////////////////////
    /// A new stream of type \ref READ_WRITE_STREAM is constructed on
    /// the given device as a file with a randomly generated name, 
    /// prefixed by "".  
    ////////////////////////////////////////////////////////////////////////////
    stream();
    
    ////////////////////////////////////////////////////////////////////////////
    /// A new stream is constructed and 
    /// named and placed according to the given parameter pathname.
    /// Its type is given by st which defaults to \ref READ_WRITE_STREAM.
    ////////////////////////////////////////////////////////////////////////////
    stream(const std::string& path_name, 
		   stream_type st = READ_WRITE_STREAM);
  
    ////////////////////////////////////////////////////////////////////////////
    /// Destructor that frees the memory buffer and closes the file;
    /// if the persistence flag is set to PERSIST_DELETE, also removes the file.
    ////////////////////////////////////////////////////////////////////////////
    ~stream();
    
    ////////////////////////////////////////////////////////////////////////////
    /// Returns the status of the stream instance; the result is either
    /// STREAM_STATUS_VALID or STREAM_STATUS_INVALID. 
    /// The only operation that can leave the stream invalid is the constructor
    /// (if that happens, the log file contains more information). No items 
    /// should be read from or written to an invalid stream.
    ////////////////////////////////////////////////////////////////////////////
    stream_status status() const { 
		return m_status; 
    }
    
    ////////////////////////////////////////////////////////////////////////////
    /// Returns wether the status of the stream is STREAM_STATUS_VALID.
    /// \sa status()
    ////////////////////////////////////////////////////////////////////////////
    bool is_valid() const { 
		return m_status == STREAM_STATUS_VALID; 
    }

    ////////////////////////////////////////////////////////////////////////////
    /// Returns true if the block's status is not BLOCK_STATUS_VALID. 
    /// \sa is_valid(), status()
    ////////////////////////////////////////////////////////////////////////////
    bool operator!() const { 
		return !is_valid(); 
    }

    ////////////////////////////////////////////////////////////////////////////
    /// Reads the current item from the stream and advance the "current item"
    /// pointer to the next item. The item read is pointed to by 
    /// *elt. If no error has occurred, return \ref NO_ERROR.
    /// If the ``current item'' pointer is beyond the last item in the stream,
    /// ERROR_END_OF_STREAM is returned
    ////////////////////////////////////////////////////////////////////////////
    err read_item(T **elt);
    
    ////////////////////////////////////////////////////////////////////////////
    /// Writes elt to the stream in the current position. Advance the 
    /// "current item" pointer to the next item. If no error has occurred
    /// \ref NO_ERROR is returned.
    ////////////////////////////////////////////////////////////////////////////
    err write_item(const T &elt);
  
    ////////////////////////////////////////////////////////////////////////////
    /// Reads *len items from the current position of the stream into
    /// the array mm_array. The "current position" pointer is increased
    /// accordingly.
	/// \deprecated
    ////////////////////////////////////////////////////////////////////////////
    err read_array(T *mm_space, stream_offset_type *len);

    ////////////////////////////////////////////////////////////////////////////
    /// Reads len items from the current position of the stream into
    /// the array mm_array. The "current position" pointer is increased
    /// accordingly.
    ////////////////////////////////////////////////////////////////////////////
    err read_array(T *mm_space, memory_size_type & len);
    
    ////////////////////////////////////////////////////////////////////////////
    /// Writes len items from array |mm_array to the
    /// stream, starting in the current position. The "current item"
    /// pointer is increased accordingly.
    ////////////////////////////////////////////////////////////////////////////
    err write_array(const T *mm_space, memory_size_type len);
    
    ////////////////////////////////////////////////////////////////////////////
    /// Returns the number of items in the stream.
    ////////////////////////////////////////////////////////////////////////////
    stream_offset_type stream_len(void) const { 
		return str.size();
    }
  
    ////////////////////////////////////////////////////////////////////////////
    /// Returns the path name of this stream in newly allocated space.
    ////////////////////////////////////////////////////////////////////////////
    std::string name() const;
  
    ////////////////////////////////////////////////////////////////////////////
    /// Move the current position to off (measured in terms of items.
    ////////////////////////////////////////////////////////////////////////////
    err seek(stream_offset_type offset);
    
    ////////////////////////////////////////////////////////////////////////////
    /// Returns the current position in the stream measured of items from the
    /// beginning of the stream.
    ////////////////////////////////////////////////////////////////////////////
    stream_offset_type tell() const { 
		return str.offset();
    }

    ////////////////////////////////////////////////////////////////////////////
    /// Resize the stream to off items. If off is less than the
    /// number of objects in the stream, truncate()
    /// truncates the stream to off objects. If
    /// off is more than the number of objects in the
    /// stream, truncate() extends the stream to the
    /// specified number of objects. In either case, the "current
    /// item" pointer will be moved to the new end of the stream.
    ////////////////////////////////////////////////////////////////////////////
    err truncate(stream_offset_type offset);
    
    ////////////////////////////////////////////////////////////////////////////
    /// This function is used for obtaining the amount of main memory used by an
    /// Stream<T> object (in bytes).
    /// \param[in] usage_type of type \ref MM_stream_usage and is 
    /// one of the following:
    /// \par MM_STREAM_USAGE_CURRENT
    /// Total amount of memory currently used by the stream.
    /// \par MM_STREAM_USAGE_MAXIMUM
    /// Max amount of memory that will ever be used by the stream.
    /// \par MM_STREAM_USAGE_OVERHEAD
    /// The amount of memory used by the object itself, without the data buffer.
    /// \par MM_STREAM_USAGE_BUFFER
    /// The amount of memory used by the data buffer.
    /// \par MM_STREAM_USAGE_SUBSTREAM
    /// The additional amount of memory that will be used by each substream created.
    /// \param[out] usage amount of memory in bytes used by the stream
    ////////////////////////////////////////////////////////////////////////////
    err main_memory_usage(memory_size_type *usage,
			  mem::stream_usage usage_type);

	////////////////////////////////////////////////////////////////////////////
    /// Returns the number of bytes that count streams will maximaly consume
    ////////////////////////////////////////////////////////////////////////////
	static memory_size_type memory_usage(memory_size_type count);
  
    ////////////////////////////////////////////////////////////////////////////
    /// Returns a \ref tpie_stats_stream object containing  statistics of 
    /// the stream. 
    ////////////////////////////////////////////////////////////////////////////
    //const stats_stream& stats() const { 
	//	return m_bteStream->stats(); 
    //}

    ////////////////////////////////////////////////////////////////////////////
    /// Returns a \ref tpie_stats_stream object containing  statistics of 
    /// the entire tpie system. 
    ////////////////////////////////////////////////////////////////////////////
	static const stats_stream& gstats() {
		stats_stream x;
		return x;
	//	return bte::stream_base_generic::gstats();
	}

	memory_size_type chunk_size(void) const {
		return 1024*1024*2/sizeof(T);
	}

    ////////////////////////////////////////////////////////////////////////////
    /// Returns the number of globally available streams.
    /// The number should resemble the the maximum
    /// number of streams allowed (which is OS-dependent) minus the number
    /// of streams currently opened by TPIE.
    ////////////////////////////////////////////////////////////////////////////
    int available_streams(void) {
		return 512;
    }
    
    ////////////////////////////////////////////////////////////////////////////
    /// Set the stream's \ref persistence flag to p, which can have one of two values:
    /// \ref PERSIST_DELETE or \ref PERSIST_PERSISTENT.}
    ////////////////////////////////////////////////////////////////////////////
    void persist(persistence p) {
		per = p;
    }
    
    ////////////////////////////////////////////////////////////////////////////
    /// Set the stram's \ref persistence flag to \ref PERSIST_PERSISTENT, thereby
    /// ensuring it is not deleted when destructed.
    ////////////////////////////////////////////////////////////////////////////
    persistence persist() const { 
		return per;
    }

    ////////////////////////////////////////////////////////////////////////////
    /// Return a string describing the stream.
    // This function gives easy access to the stream's file name and its length.
    ////////////////////////////////////////////////////////////////////////////
	std::string& sprint();
    
private:
	void setup(const std::string & path_name, stream_type st);

    /** Restricted copy constructor */
    stream(const stream<T>& other);
    /** Restricted assignment operator*/
    stream<T>& operator=(const stream<T>& other);
	
	persistence per;
    stream_status m_status;
	file_stream<T> str;
};
	

template<class T>
stream<T>::stream() {
	per = PERSIST_DELETE;
	setup(tempname::tpie_name(""), READ_WRITE_STREAM);
};


template<class T>
stream<T>::stream(const std::string& path_name, stream_type st) {
	per = PERSIST_PERSISTENT;
	setup(path_name, st);
}

template<class T>
void stream<T>::setup(const std::string & path_name, stream_type st) {
	file_base::access_type at;
	switch (st) {
	case READ_STREAM: 
		at = file_base::read;
		break;
	case WRITE_STREAM:
	case APPEND_STREAM:	
		at = file_base::write;
		break;
	case READ_WRITE_STREAM:
	default:
		at = file_base::read_write;
		break;
	}
	
	m_status = STREAM_STATUS_INVALID;
	try {
		str.open(path_name, at);
		if (st==APPEND_STREAM) str.seek( str.size() );
	    m_status = STREAM_STATUS_VALID;
	} catch(stream_exception & e) {
		TP_LOG_FATAL_ID(e.what());
	}
}

template<class T>
std::string stream<T>::name() const {
	return str.path();
}

template<class T>
err stream<T>::seek(stream_offset_type offset) {
	try {
		str.seek(offset);
	} catch(stream_exception & e) {
		TP_LOG_WARNING_ID(e.what());		
		return BTE_ERROR;
	}
	return NO_ERROR;
}

template<class T>
err stream<T>::truncate(stream_offset_type offset) {
	try {
		str.truncate(offset);
	} catch(stream_exception & e) {
		TP_LOG_WARNING_ID(e.what());		
		return BTE_ERROR;
	}
	return NO_ERROR;
}

template<class T>
memory_size_type stream<T>::memory_usage(memory_size_type count) {
	return file_stream<T>::memory_usage(1, 1.0, true) + sizeof(stream<T>)*count;
}

// Query memory usage
template<class T>
err stream<T>::main_memory_usage(memory_size_type *usage,
									   mem::stream_usage usage_type) {
	*usage = memory_usage(1);
	return NO_ERROR;
}

template<class T>
stream<T>::~stream() {
	if (!per) remove(str.path());
}

template<class T>
err stream<T>::read_item(T **elt) {
	try {
		*elt = &str.read();
	} catch(end_of_stream_exception & e) {
		TP_LOG_DEBUG_ID("eos in read_item");
		return END_OF_STREAM;
	} catch(stream_exception & e) {
		TP_LOG_DEBUG_ID(e.what());
		return BTE_ERROR;
	}
	return NO_ERROR;
}

template<class T>
err stream<T>::write_item(const T &elt) {
	try {
		str.write(elt);
	} catch(stream_exception & e) {
		TP_LOG_WARNING_ID(e.what());
		return BTE_ERROR;
	}
	return NO_ERROR;
}

template<class T>
err stream<T>::read_array(T *mm_space, stream_offset_type *len) {
	memory_size_type l=*len;
	err e = read_array(mm_space, l);
	*len = l;
	return e;
}

template<class T>
err stream<T>::read_array(T *mm_space, memory_size_type & len) {
	try {
		str.read(mm_space, mm_space+len);
	} catch(end_of_stream_exception & e) {
		TP_LOG_DEBUG_ID("eos in read_item");
		return END_OF_STREAM;
	} catch(stream_exception & e) {
		TP_LOG_DEBUG_ID(e.what());
		return BTE_ERROR;
	}
	return NO_ERROR;
}

template<class T>
err stream<T>::write_array(const T *mm_space, memory_size_type len) {
	try {
		str.write(mm_space, mm_space+len);
	} catch(stream_exception & e) {
		TP_LOG_DEBUG_ID(e.what());
		return BTE_ERROR;
	}
	return NO_ERROR;
}

template<class T>
std::string& stream<T>::sprint() {
	static std::string buf;
	std::stringstream ss;
	ss << "STREAM " << name() <<  " " << static_cast<long>(stream_len());
	ss >> buf;
	return buf;
}

}  //  ami namespace
}  //  tpie namespace
#include <tpie/stream_compatibility.h>
#endif // _TPIE_AMI_STREAM_H
