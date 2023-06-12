#pragma once
#include <math/Vertex.hpp>

namespace sm {

struct OrthonormalBasis {
	using V2 = Vertex<double,2>;
	V2 Ex;
	V2 Ey;
	V2 iEx;
	V2 iEy;
	
	OrthonormalBasis(V2 const& Ex) noexcept
		: Ex{Ex/Ex.length()}
		, Ey{ortho(this->Ex)}
		, iEx{this->Ex[0], this->Ey[0]}
		, iEy{this->Ex[1], this->Ey[1]}
	{}
	
	constexpr V2 to_local(V2 const& global) const noexcept {
		return {
			  Ex * global
			, Ey * global
		};
	}
	constexpr V2 to_global(V2 const& local) const noexcept {
		return {
			  iEx * local
			, iEy * local
		};
	}
};

} /** namespace sm */
