#pragma once
#include "math/Rectangle.hpp"
#include "math/Vertex.hpp"
#include "math/LineLineIntersection.hpp"
#include "math/PointLine.hpp"
#include <vector>
#include <array>
#include <optional>
#include <variant>
#include <algorithm>
#include <cassert>
#include <iostream>

namespace sm {

template<typename T>
struct ResourceBuffer {
	struct Entry {
		std::size_t begin;
		std::size_t end;
	};
	std::vector<Entry> entries;
	std::vector<T>     data;
	
	template<typename ResourceBuffer_t, typename T_t>
	struct ResourceIterator 
		: std::iterator<std::random_access_iterator_tag, T_t>
	{
		ResourceBuffer* parent;
		std::size_t     pos;
		
		ResourceIterator(ResourceBuffer* parent, std::size_t pos)
			: parent(parent)
			, pos(pos)
		{}
		ResourceIterator& operator++() {
			++pos;
			return *this;
		}
		friend
		std::ptrdiff_t operator-(ResourceIterator const& a, ResourceIterator const& b) {
			return a.pos - b.pos;
		}
		friend
		bool operator==(ResourceIterator const& a, ResourceIterator const& b) {
			return a.pos == b.pos;
		}
		friend
		bool operator!=(ResourceIterator const& a, ResourceIterator const& b) {
			return !(a == b);
		}
		T_t& operator*() const {
			return parent->data[pos];
		}
		T_t* operator->() const {
			return &parent->data[pos];
		}
		operator ResourceIterator<ResourceBuffer_t const, T_t const>() const {
			return {parent, pos};
		}
	};
	
	template<typename ResourceBuffer_t, typename T_t>
	struct Resource {
		using iterator       = ResourceIterator<ResourceBuffer_t,       T_t      >;
		using const_iterator = ResourceIterator<ResourceBuffer_t const, T_t const>;
		
		ResourceBuffer_t* parent;
		std::size_t       id;
		
		iterator begin() {
			return {parent, parent->entries[id].begin};
		}
		iterator end() {
			return {parent, parent->entries[id].end};
		}
		const_iterator begin() const {
			return {parent, parent->entries[id].begin};
		}
		const_iterator end() const {
			return {parent, parent->entries[id].end};
		}
		operator Resource<ResourceBuffer_t const, T_t const>() const {
			return {parent, id};
		}
		std::size_t size() const {
			return parent->entries[id].end - parent->entries[id].begin;
		}
	};
	
	using resource       = Resource<ResourceBuffer,       T      >;
	using const_resource = Resource<ResourceBuffer const, T const>;
	
	const_resource at(std::size_t id) const {
		return {this, id};
	}
	resource at(std::size_t id) {
		return {this, id};
	}
	
	/**
	 * example InsertingOperation:
	 * (let T == int)
	 * [](std::vector<int>& sink) {
	 * 		for(int i = 0; i < 23; ++i) {
	 * 			sink.push_back(i);
	 * 		}
	 * }
	 * 
	 * -> add inserts in data and returns inserted data as iterable resource
	 */
	template<typename InsertingOperation>
	resource add(InsertingOperation op) {
		Entry entry;
		entry.begin = data.size();
		op(data);
		entry.end = data.size();
		std::size_t id = entries.size();
		entries.push_back(entry);
		return {this, id};
	}
	void clear() {
		entries.clear();
		data.clear();
	}
};

template<typename CellEvaluator>
class QuadTree {
public:
	enum Role {
		  NORTH_EAST = 0
		, NORTH_WEST = 1
		, SOUTH_WEST = 2
		, SOUTH_EAST = 3
		, ROOT       = 8
	};
	enum Edge {
		  EAST  = 0
		, NORTH = 1
		, WEST  = 2
		, SOUTH = 3
	};
	using value_t             = typename CellEvaluator::CellValue;
	using parent_id_t         = std::optional<std::size_t>;
	using child_ids_t         = std::optional<std::array<std::size_t,4>>;
	using neighbours_buffer_t = ResourceBuffer<std::size_t>;
public:
	struct Node {
		std::size_t id;
		parent_id_t parent_id;
		Role        role;
		Rectangle   bounds;
		child_ids_t child_ids;
		value_t     value;
		std::array<std::optional<neighbours_buffer_t::resource>,4> edge_neighbours;
		std::optional<std::array<std::optional<std::size_t>,4>>    corner_neighbours;
		std::optional<neighbours_buffer_t::resource>               neighbours_4;
		std::optional<neighbours_buffer_t::resource>               neighbours_8;

		
		Node(std::size_t id, parent_id_t parent_id, Role role, Rectangle const& bounds)
			: id(id)
			, parent_id(parent_id)
			, role(role)
			, bounds(bounds)
			, child_ids{}
		{}
		
		template<typename OS>
		friend
		OS& operator<<(OS& os, Node const& node) {
			os << "id: " << node.id;
			os << ", role: ";
			switch(node.role) {
				case NORTH_WEST: os << "NORTH_WEST"; break;
				case NORTH_EAST: os << "NORTH_EAST"; break;
				case SOUTH_WEST: os << "SOUTH_WEST"; break;
				case SOUTH_EAST: os << "SOUTH_EAST"; break;
				case ROOT:       os << "ROOT      "; break;
				default:         os << "UNKWOWN   ";
			}
			os << ", center: " << node.bounds.center();
			os << ", width: "  << node.bounds.width();
			os << ", parent: [";
			if(node.parent_id) {
				os << *node.parent_id;
			}
			os << "]";
			os << ", children: [";
			if(node.child_ids) {
				os <<         (*node.child_ids)[0];
				os << ", " << (*node.child_ids)[1];
				os << ", " << (*node.child_ids)[2];
				os << ", " << (*node.child_ids)[3];
			}
			os << "]";
// 			os << ", value: [";
// 			os << node.value;
// 			os << "]";
			return os;
		}
	};
	
	struct Context {
		std::vector<Node>   nodes_buffer;
		neighbours_buffer_t neighbours_buffer;
		std::vector<Role>   track_buffer;
	};

	std::vector<Node>&   nodes;
	neighbours_buffer_t& neighbours;
	std::vector<Role>&   track;
	CellEvaluator const& cell_evaluator;
	
	QuadTree(
		  Context&             context
		, CellEvaluator const& cell_evaluator
		, Rectangle const&     bounds
	)
		: nodes(context.nodes_buffer)
		, neighbours(context.neighbours_buffer)
		, track(context.track_buffer)
		, cell_evaluator(cell_evaluator)
	{
		nodes.clear();
		neighbours.clear();
		insert({}, ROOT, bounds);
	}
	void collect_leave_ids(std::vector<std::size_t>& leaves) {
		leaves.clear();
		collect_leave_ids(leaves, 0);
	}
	bool is_refined(std::size_t id) const {
		return nodes[id].child_ids.has_value();
	}
	std::size_t find_id(Vertex<double,2> const& p) {
		std::size_t id = 0;
		while(true) {
			refine_if_necessary(id);
			if(is_refined(id)) {
				if(p[0] > nodes[child_id(id, SOUTH_WEST)].bounds.max[0]) {
					if(p[1] > nodes[child_id(id, SOUTH_WEST)].bounds.max[1]) {
						id = child_id(id, NORTH_EAST);
					} else {
						id = child_id(id, SOUTH_EAST);
					}
				} else {
					if(p[1] > nodes[child_id(id, SOUTH_WEST)].bounds.max[1]) {
						id = child_id(id, NORTH_WEST);
					} else {
						id = child_id(id, SOUTH_WEST);
					}
				}
			} else {
				return id;
			}
		}
	}
	neighbours_buffer_t::resource find_neighbour4_ids(std::size_t id) {
		if(nodes[id].neighbours_4) {
			return *nodes[id].neighbours_4;
		}
		nodes[id].neighbours_4 = neighbours.add(
			[&](std::vector<std::size_t>& sink) {
				auto south = find_edge_neighbour_ids(id, SOUTH);
				auto east  = find_edge_neighbour_ids(id, EAST);
				auto north = find_edge_neighbour_ids(id, NORTH);
				auto west  = find_edge_neighbour_ids(id, WEST);
				sink.reserve(
					  sink.size()
					+ south.size()
					+ east.size()
					+ north.size()
					+ west.size()
				);
				for(std::size_t n : south) {
					sink.push_back(n);
				}
				for(std::size_t n : east) {
					sink.push_back(n);
				}
				for(std::size_t n : north) {
					sink.push_back(n);
				}
				for(std::size_t n : west) {
					sink.push_back(n);
				}
			}
		);
		return *nodes[id].neighbours_4;
	}
	neighbours_buffer_t::resource find_edge_neighbour_ids(std::size_t id, Edge direction) {
		if(nodes[id].edge_neighbours[direction]) {
			return *nodes[id].edge_neighbours[direction];
		}
		nodes[id].edge_neighbours[direction] = neighbours.add(
			[&](std::vector<std::size_t>& sink) {
				track.clear();
				parent_id_t neighbour_id = find_neighbour_id(id, direction, track);
				while(!track.empty()) {
					if(!neighbour_id.has_value()) {
						return;
					}
					refine_if_necessary(*neighbour_id);
					if(!is_refined(*neighbour_id)) {
						sink.push_back(*neighbour_id);
						return;
					}
					neighbour_id = child_id(*neighbour_id, track.back());
					track.pop_back();
				}
				static Edge opposite[4] = {
					WEST,SOUTH,EAST,NORTH
				};
				if(neighbour_id.has_value()) {
					collect_border_leaves(*neighbour_id, sink,opposite[direction]);
				}
			}
		);
		return *nodes[id].edge_neighbours[direction];
	}
	std::array<std::optional<std::size_t>,4> find_corner_neighbour_ids(std::size_t id) {
		if(nodes[id].corner_neighbours) {
			return *nodes[id].corner_neighbours;
		}
		nodes[id].corner_neighbours = std::array<std::optional<std::size_t>, 4> {
			  find_corner_neighbour_id(id, NORTH_EAST)
			, find_corner_neighbour_id(id, NORTH_WEST)
			, find_corner_neighbour_id(id, SOUTH_WEST)
			, find_corner_neighbour_id(id, SOUTH_EAST)
		};
		return *nodes[id].corner_neighbours;
	}
	neighbours_buffer_t::resource find_neighbour8_ids(std::size_t id) {
		if(nodes[id].neighbours_8) {
			return *nodes[id].neighbours_8;
		}
		nodes[id].neighbours_8 = neighbours.add(
			[&](std::vector<std::size_t>& sink) {
				std::size_t begin = sink.size();
				auto n4 = find_neighbour4_ids(id);
				sink.reserve(sink.size() + n4.size() + 4);
				for(std::size_t n : n4) {
					sink.push_back(n);
				}
				for(auto c : find_corner_neighbour_ids(id)) {
					if(c) {
						sink.push_back(*c);
					}
				}
				std::sort(sink.begin() + begin, sink.end());
				sink.erase(
					  std::unique(sink.begin() + begin, sink.end())
					, sink.end()
				);
			}
		);
		return *nodes[id].neighbours_8;
	}
	
	void trace_segment(
		  Segment<double,2> const& segment
		, std::vector<std::size_t>& cell_trace
	) {
		std::size_t id_start = find_id(segment.a);
		std::size_t id_end = find_id(segment.b);
		auto trace =[&](std::size_t id) {
			auto look = [&](Edge direction)
				-> std::optional<Vertex<double,2>>
			{
				auto edge = [&]() {
					Rectangle const& bounds = nodes[id].bounds;
					if(direction == SOUTH) {
						return Segment{bounds.A(), bounds.B()};
					} else if(direction == EAST) {
						return Segment{bounds.B(), bounds.C()};
					} else if(direction == NORTH) {
						return Segment{bounds.C(), bounds.D()};
					} else {
						return Segment{bounds.D(), bounds.A()};
					}
				};
				return SegmentSegmentIntersection(segment, edge()).intersection();
			};
			auto horizontal_check = [&](Vertex<double,2> const& point) {
				return [&](Rectangle const& bounds) {
					return point[0] >= bounds.min[0]
						&& point[0] <= bounds.max[0]
					;
				};
			};
			auto vertical_check = [&](Vertex<double,2> const& point) {
				return [&](Rectangle const& bounds) {
					return point[1] >= bounds.min[1]
						&& point[1] <= bounds.max[1]
					;
				};
			};
			auto check_intersection = [&](
				  Edge direction
				, auto point_check
			) {
				std::optional<Vertex<double,2>> intersection = look(direction);
				if(intersection) {
					auto match = point_check(*intersection);
					for(std::size_t n : find_edge_neighbour_ids(id, direction)) {
						if(match(nodes[n].bounds)) {
							cell_trace.push_back(n);
						}
					}
				}
			};
			std::size_t trace_begin = cell_trace.size();
			if(segment.b[1] < segment.a[1]) {
				check_intersection(SOUTH, horizontal_check);
			}
			if(segment.b[0] > segment.a[0]) {
				check_intersection(EAST, vertical_check);
			}
			if(segment.b[1] > segment.a[1]) {
				check_intersection(NORTH, horizontal_check);
			}
			if(segment.b[0] < segment.a[0]) {
				check_intersection(WEST, vertical_check);
			}
			for(std::size_t i = trace_begin; i < cell_trace.size(); ++i) {
				if(cell_trace[i] == id_end) {
					return id_end;
				}
			}
			auto next = std::min_element(
				  cell_trace.begin() + trace_begin
				, cell_trace.end()
				, [&](std::size_t node_id_a, std::size_t node_id_b) {
					Vertex<double,2> center_a = nodes[node_id_a].bounds.center();
					Vertex<double,2> center_b = nodes[node_id_b].bounds.center();
					return Distance(center_a, segment.b) < Distance(center_b, segment.b);
				}
			);
			if(next != cell_trace.end()) {
				return *next;
			}
			return id_end;
		};
		cell_trace.clear();
		while(id_start != id_end) {
			id_start = trace(id_start);
		}
	}
	Node& operator[](std::size_t id) {
		return nodes[id];
	}
	Node const& operator[](std::size_t id) const {
		return nodes[id];
	}
	std::size_t size() const {
		return nodes.size();
	}
	auto begin() const {
		return nodes.begin();
	}
	auto end() const {
		return nodes.end();
	}
	auto begin() {
		return nodes.begin();
	}
	auto end() {
		return nodes.end();
	}
private:
	std::size_t insert(parent_id_t parent_id, Role role, Rectangle const& bounds) {
		nodes.emplace_back(nodes.size(), parent_id, role, bounds);
		return nodes.back().id;
	}
	std::size_t child_id(std::size_t id, Role role) {
		return (*nodes[id].child_ids)[role];
	}
	std::size_t parent_id(std::size_t id) {
		return *nodes[id].parent_id;
	}
	void refine_if_necessary(std::size_t id) {
		if(!is_refined(id)) {
			if(cell_evaluator(nodes[id].value, nodes[id].bounds)) {
				Vertex<double,2> _center = nodes[id].bounds.center();
				Rectangle NE(nodes[id].bounds);
				Rectangle NW(nodes[id].bounds);
				Rectangle SE(nodes[id].bounds);
				Rectangle SW(nodes[id].bounds);
				NW.min[1] = _center[1];
				NW.max[0] = _center[0];
				NE.min=_center;
				SW.max=_center;
				SE.min[0] = _center[0];
				SE.max[1] = _center[1];
				nodes[id].child_ids = std::array<std::size_t, 4>{};
				(*nodes[id].child_ids)[NORTH_EAST] = insert(id, NORTH_EAST, NE);
				(*nodes[id].child_ids)[NORTH_WEST] = insert(id, NORTH_WEST, NW);
				(*nodes[id].child_ids)[SOUTH_EAST] = insert(id, SOUTH_EAST, SE);
				(*nodes[id].child_ids)[SOUTH_WEST] = insert(id, SOUTH_WEST, SW);
			}
		}
	}
	void collect_leave_ids(std::vector<std::size_t>& leaves, std::size_t id) {
		refine_if_necessary(id);
		if(is_refined(id)) {
			collect_leave_ids(leaves, child_id(id, NORTH_EAST));
			collect_leave_ids(leaves, child_id(id, NORTH_WEST));
			collect_leave_ids(leaves, child_id(id, SOUTH_EAST));
			collect_leave_ids(leaves, child_id(id, SOUTH_WEST));
		} else {
			leaves.push_back(id);
		}
	}
	parent_id_t find_neighbour_id(std::size_t id, Edge direction,std::vector<Role>& track) {
		if(!nodes[id].parent_id.has_value()) {
			return {};
		}
		static constexpr Role transition[4][4] = {
			//NORTH_EAST	NORTH_WEST	SOUTH_WEST	SOUTH_EAST
			 {NORTH_WEST,	NORTH_EAST,	SOUTH_EAST,	SOUTH_WEST}	//EAST
			,{SOUTH_EAST,	SOUTH_WEST,	NORTH_WEST,	NORTH_EAST}	//NORTH
			,{NORTH_WEST,	NORTH_EAST,	SOUTH_EAST,	SOUTH_WEST}	//WEST
			,{SOUTH_EAST,	SOUTH_WEST,	NORTH_WEST,	NORTH_EAST}	//SOUTH
		};
		static constexpr bool recurse[4][4] = {
			//NORTH_EAST	NORTH_WEST	SOUTH_WEST	SOUTH_EAST
			 {true,			false,		false,		true}	//EAST
			,{true,			true,		false,		false}	//NORTH
			,{false,		true,		true,		false}	//WEST
			,{false,		false,		true,		true}	//SOUTH
		};
		Role role = nodes[id].role;
		std::size_t parent_id = *nodes[id].parent_id;
		if(recurse[direction][role]) {
			track.push_back(transition[direction][role]);
			return find_neighbour_id(parent_id, direction,track);
		} else {
			return child_id(parent_id, transition[direction][role]);
		}
	}
	void collect_border_leaves(std::size_t id, std::vector<std::size_t>& leaves, Edge border) {
		refine_if_necessary(id);
		if(is_refined(id)) {
			static Role _a[4]={	NORTH_EAST,NORTH_EAST,NORTH_WEST,SOUTH_EAST	};
			static Role _b[4]={	SOUTH_EAST,NORTH_WEST,SOUTH_WEST,SOUTH_WEST	};
			collect_border_leaves(child_id(id, _a[border]), leaves, border);
			collect_border_leaves(child_id(id, _b[border]), leaves, border);
		} else {
			leaves.push_back(id);
		}
	}
	std::optional<std::size_t> find_corner_neighbour_id(std::size_t id, Role direction) {
		auto minimum_neighbour_id = [&](std::size_t id, Edge direction, auto cmp)
			-> std::optional<std::size_t>
		{
			auto ids = find_edge_neighbour_ids(id, direction);
			auto it = std::min_element(ids.begin(), ids.end(), cmp);
			if(it == ids.end()) {
				return {};
			}
			return *it;
		};
		auto minimum_neighbour2_id = [&](Edge direction1, Edge direction2)
			-> std::optional<std::size_t>
		{
			auto cmp1 = [&](std::size_t id_a, std::size_t id_b) {
				if(direction2 == WEST) {
					return nodes[id_a].bounds.max[0] < nodes[id_b].bounds.max[0];
				} else {
					return nodes[id_a].bounds.max[0] > nodes[id_b].bounds.max[0];
				}
			};
			auto cmp2 = [&](std::size_t id_a, std::size_t id_b) {
				if(direction1 == NORTH) {
					return nodes[id_a].bounds.max[1] < nodes[id_b].bounds.max[1];
				} else {
					return nodes[id_a].bounds.max[1] > nodes[id_b].bounds.max[1];
				}
			};
			std::optional<std::size_t> tmp = minimum_neighbour_id(id, direction1, cmp1);
			if(tmp) {
				tmp = minimum_neighbour_id(*tmp, direction2,cmp2);
			}
			if(tmp) {
				if(    (direction2 == WEST && nodes[*tmp].bounds.max[0] == nodes[id].bounds.min[0])
					|| (direction2 == EAST && nodes[*tmp].bounds.min[0] == nodes[id].bounds.max[0])
				) {
					auto n4 = find_neighbour4_ids(id);
					if(std::find(n4.begin(), n4.end(), *tmp) == n4.end()) {
						return tmp;
					}
				}
			}
			return {};
		};
		if(direction == NORTH_WEST) {
			return minimum_neighbour2_id(NORTH, WEST);
		}
		if(direction == NORTH_EAST) {
			return minimum_neighbour2_id(NORTH, EAST);
		}
		if(direction == SOUTH_WEST) {
			return minimum_neighbour2_id(SOUTH, WEST);
		}
		if(direction == SOUTH_EAST) {
			return minimum_neighbour2_id(SOUTH, EAST);
		}
		return {};
	}
};

} /** namespace sm */
