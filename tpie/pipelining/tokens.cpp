// -*- mode: c++; tab-width: 4; indent-tabs-mode: t; eval: (progn (c-set-style "stroustrup") (c-set-offset 'innamespace 0)); -*-
// vi:set ts=4 sts=4 sw=4 noet :
// Copyright 2012, The TPIE development team
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

#include <tpie/pipelining/tokens.h>
#include <tpie/pipelining/node.h>

namespace tpie {

namespace pipelining {

namespace bits {

node_map::id_t node_map::nextId = 0;

// Called by graph_traits
void node_map::send_successors() const {
	for (relmapit i = m_relations.begin(); i != m_relations.end(); ++i) {
		switch (i->second.second) {
			case pushes:
				m_tokens.find(i->first)->second->add_successor(m_tokens.find(i->second.first)->second);
				break;
			case pulls:
			case depends:
				m_tokens.find(i->second.first)->second->add_successor(m_tokens.find(i->first)->second);
				break;
		}
	}
}

void node_map::link(node_map::ptr target) {
	if (target.get() == this) {
		// self link attempted
		// we must never have some_map->m_authority point to some_map,
		// since it would create a reference cycle
		return;
	}
	// union by rank
	if (target->m_rank > m_rank)
		return target->link(ptr(self));

	for (mapit i = target->begin(); i != target->end(); ++i) {
		set_token(i->first, i->second);
	}
	for (relmapit i = target->m_relations.begin(); i != target->m_relations.end(); ++i) {
		m_relations.insert(*i);
	}
	for (relmapit i = target->m_relationsInv.begin(); i != target->m_relationsInv.end(); ++i) {
		m_relationsInv.insert(*i);
	}
	target->m_tokens.clear();
	target->m_authority = ptr(self);

	// union by rank
	if (target->m_rank == m_rank)
		++m_rank;
}

node_map::ptr node_map::find_authority() {
	if (!m_authority)
		return ptr(self);

	node_map * i = m_authority.get();
	while (i->m_authority) {
		i = i->m_authority.get();
	}
	ptr result(i->self);

	// path compression
	node_map * j = m_authority.get();
	while (j->m_authority) {
		node_map * k = j->m_authority.get();
		j->m_authority = result;
		j = k;
	}

	return result;
}

size_t node_map::out_degree(const relmap_t & map, id_t from, node_relation rel) const {
	size_t res = 0;
	relmapit i = map.find(from);
	while (i != map.end() && i->first == from) {
		if (i->second.second == rel) ++res;
		++i;
	}
	return res;
}

} // namespace bits

} // namespace pipelining

} // namespace tpie
