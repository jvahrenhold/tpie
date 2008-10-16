#ifndef _TPIE_PQ_OVERFLOW_HEAP_H_
#define _TPIE_PQ_OVERFLOW_HEAP_H_

#include "pq_internal_heap.h"

namespace tpie{

/////////////////////////////////////////////////////////
///
///  \class pq_overflow_heap
///  \author Lars Hvam Petersen
///
///  Overflow Priority Queue, based on a simple Heap
///
/////////////////////////////////////////////////////////
template<typename T, typename Comparator = std::less<T> >
class pq_overflow_heap {
	public:
		/////////////////////////////////////////////////////////
		///
		/// Constructor
		///
		/// \param maxsize Maximal size of queue
		///
		/////////////////////////////////////////////////////////
		pq_overflow_heap(TPIE_OS_SIZE_T maxsize);

		/////////////////////////////////////////////////////////
		///
		/// Destructor
		///
		/////////////////////////////////////////////////////////
		~pq_overflow_heap();

		/////////////////////////////////////////////////////////
		///
		/// Insert an element into the priority queue
		///
		/// \param x The item
		///
		/////////////////////////////////////////////////////////
		void push(const T& x);

		/////////////////////////////////////////////////////////
		///
		/// Remove the top element from the priority queue
		///
		/////////////////////////////////////////////////////////
		void pop();

		/////////////////////////////////////////////////////////
		///
		/// See whats on the top of the priority queue
		///
		/// \return Top element
		///
		/////////////////////////////////////////////////////////
		const T& top();

		/////////////////////////////////////////////////////////
		///
		/// Returns the size of the queue
		///
		/// \return Queue size
		///
		/////////////////////////////////////////////////////////
		const TPIE_OS_OFFSET size();

		/////////////////////////////////////////////////////////
		///
		/// Return true if queue is empty otherwise false
		///
		/// \return Boolean - empty or not
		///
		/////////////////////////////////////////////////////////
		const bool empty();

		/////////////////////////////////////////////////////////
		///
		/// The factor of the size, total, which is returned 
		/// sorted 
		///
		/////////////////////////////////////////////////////////
		static const double sorted_factor = 1; 

		/////////////////////////////////////////////////////////
		///
		/// Returns whether the overflow heap is full or not
		///
		/// \return Boolean - full or not
		///
		/////////////////////////////////////////////////////////
		const bool full();

		/////////////////////////////////////////////////////////
		///
		/// Sorts the underlying array and returns a pointer to it, this operation invalidades the heap.
		///
		/// \return A pointer to the sorted underlying array
		///
		/////////////////////////////////////////////////////////
		T* sorted_array();

		/////////////////////////////////////////////////////////
		///
		/// Return size of sorted array
		///
		/// \return Size
		///
		/////////////////////////////////////////////////////////
		const TPIE_OS_OFFSET sorted_size();

		/////////////////////////////////////////////////////////
		///
		/// Remove all elements from queue 
		///
		/////////////////////////////////////////////////////////
		void sorted_pop();

	private:
		Comparator comp_;
		Heap<T, Comparator>* h;
		TPIE_OS_OFFSET maxsize;
		T dummy;
};

#include "pq_overflow_heap.inl"

}
#endif
