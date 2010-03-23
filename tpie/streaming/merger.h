// -*- mode: c++; tab-width: 4; indent-tabs-mode: t; eval: (progn (c-set-style "stroustrup") (c-set-offset 'innamespace 0)); -*-
// vi:set ts=4 sts=4 sw=4 noet :
// Copyright 2009, The TPIE development team
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
#include <algorithm>
#include <cassert>
#include <tpie/types.h>
#include <tpie/util.h>
#include <tpie/config.h>
#ifndef _TPIE_STREAMING_MERGER_H
#define _TPIE_STREAMING_MERGER_H

#ifndef TPIE_USE_CPP_0X
#warning "The k merger requires c++ 0x"
#endif
namespace tpie {
namespace streaming {

template <typename ...rest>
class merger_item {
private:
	const void * item;
	int n;
public:
	template <int i>
	inline void set(const typename ith_type<i, rest...>::T & x) {
		n=i;
		item = reinterpret_cast<const void *>(&x);
	}
	
	inline int stream() const {
		return n;
	}  
	
	template <int i> 
	inline const typename ith_type<i, rest...>::T & get() const {
		assert(n == i);
		return *reinterpret_cast<const typename ith_type<i, rest...>::T* >(item);
	}
};


template <int i, int j, typename key_t, typename T>
struct mkey {
	inline static auto f(const key_t & key, const T & a) -> decltype(key(a.template get<i>())) {
		if (a.stream() <= (i+j)>>1)
			return mkey<i, ((i+j)>>1), key_t, T>::f(key, a);
		else
			return mkey<(1+(i+j)>>1), j, key_t, T>::f(key, a);
	}
};

template <int i, typename key_t, typename T> 
struct mkey<i,i,key_t, T> {
	inline static auto f(const key_t & key, const T & a) -> decltype(key(a.template get<i>())) {
		return key(a.template get<i>());
	}
};

template <typename comp_t, typename key_t, typename ...rest> 
class multitype_pq {
public:
	typedef merger_item<rest...> item_type;
	item_type items[sizeof...(rest)];
	int count;
	typedef mkey<0, sizeof...(rest)-1, key_t, item_type> ke_t;
	
	struct mcomp_t {
	public:
		key_t key;
		comp_t comp;
		mcomp_t(const comp_t & c, const key_t & k): comp(c), key(k) {}
		inline bool operator()(const item_type & a, const item_type & b) const {
			return comp(ke_t::f(key, a), ke_t::f(key, b));
		}
	};
	
	mcomp_t comp;
	multitype_pq(const comp_t & c, const key_t & k): count(0), comp(c, k) {}

	inline void bubbleDown() {
		int i=0;
		while ((i+1)*2 < count) {
			if(comp(items[(i+1)*2-1], items[(i+1)*2])) {
				if (!comp(items[(i+1)*2-1], items[i])) break;
				std::swap(items[i], items[(i+1)*2-1]);
				i = (i+1)*2-1;
			} else {
				if (!comp(items[(i+1)*2], items[i])) break;
				std::swap(items[i], items[(i+1)*2]);
				i = (i+1)*2;
			}
		}
		if ((i+1)*2 == count && comp(items[(i+1)*2-1], items[i]))
			std::swap(items[i], items[(i+1)*2-1]);
	}
	
	inline void bubbleUp(int i) {
		while(i > 0 && comp(items[i], items[i>>1])) {
			std::swap(items[i], items[i>>1]);
			i = i>>1;
		}
	}
	
	inline const item_type & min() const {
		return items[0];
	}
	
	inline void deleteMinAndInsert(const item_type & item) {
		items[0] = item;
		bubbleDown();
	}
	
	inline void deleteMin() {
		--count;
		if(count >= 0) items[0] = items[count];
		bubbleDown();
	}
	
	inline void insert(const item_type & item) {
		items[count] = item;
		++count;
		bubbleUp(count-1);
	}  

	inline bool empty() const {
		return count == 0;
	}
	
//  	void dump() {
//  		std::cout << "PQ "; 
//  		for(int i=0; i < count; ++i) 
//  			std::cout << ke_t::f(key_t(), items[i]) << " ";
//  		std::cout << std::endl;
//  	}
};

template <typename T>
struct extract_all {
	typedef T result_type;

	const T & operator()(const T & item) const {
		return item;
	}
};


// template <typename dest_t, 
// 		  typename item_t,
// 		  typename comp_t,
// 		  typename key_t,
// 		  typename ...pools_t>
// class push_merger {
// public:
// 	multitype_pq<comp_t, key_t, item_t, pools_t::pull_type...> pq;
// };


template <typename pq_t, int i, typename ...pulls_t>
struct pull_container {};

template <typename pq_t, int i, typename pull_t, typename ...pulls_t>
struct pull_container<pq_t, i, pull_t, pulls_t...> {
	pull_t & pull;
	pull_container<pq_t, i+1, pulls_t...> tail;
	typedef typename pull_t::pull_type pull_type;
	typedef typename pq_t::item_type item_type;
	inline pull_container(pull_t & p, pulls_t &... pulls): pull(p), tail(pulls...) {};

	pull_type in;
	pull_type out;


	inline void pull_begin() {
		pull.pull_begin();
		tail.pull_begin();
	}

	inline void pull_end() {
		pull.pull_end();
		tail.pull_end();
	}
	
	inline void initial_fill(pq_t & pq) {
		item_type item;
		if (pull.can_pull()) {
			in = pull.pull();
			item.template set<i>(in);
			pq.insert(item);
		}
		tail.initial_fill(pq);
	}

	inline item_type insert_next(pq_t & pq, item_type & cur) {
		if (cur.stream() != i)
			return tail.insert_next(pq, cur); 
		out = in;
		if (pull.can_pull()) {
			item_type item;
			in = pull.pull();
			item.template set<i>(in);
			pq.deleteMinAndInsert(item);
		} else
			pq.deleteMin();
		item_type r;
		r.template set<i>(out);
		return r;
	}
};

template <typename pq_t,int i>
struct pull_container<pq_t, i> {
	typedef typename pq_t::item_type item_type;

	inline pull_container() {}
	inline void pull_begin() {}
	inline void pull_end() {}
	inline void initial_fill(pq_t & pq) {}
	inline item_type insert_next(pq_t & pq, item_type & cur) {}
};

template <typename comp_t,
		  typename key_t,
		  typename ...pulls_t> 
class pull_merger {
private:
	typedef multitype_pq<comp_t, key_t, typename pulls_t::pull_type...> pq_t;
	pq_t pq;
	pull_container<pq_t, 0, pulls_t...> c;
public:
	typedef typename pq_t::item_type pull_type;

	pull_merger(const comp_t & comp, const key_t & key, 
				pulls_t &... pulls): pq(comp, key), c(pulls...) {};
	
	void pull_begin() {
		c.pull_begin();
		c.initial_fill(pq);
	}
	
	void pull_end() {
		c.pull_end();
	}

	pull_type pull() {
		pull_type item = pq.min();
		return c.insert_next(pq, item);
	}

	bool can_pull() const {
		return !pq.empty();
	}
};

}}
#endif //_TPIE_STREAMING_MERGER_H
