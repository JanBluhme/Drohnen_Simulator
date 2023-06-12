#pragma once
#include "view/CoordinateSystemComponent.hpp"

namespace robo {

struct CoordinateSystemView {
	EnvironmentViewBase&      parent;
	CoordinateSystemComponent sys;
	
	CoordinateSystemView(EnvironmentViewBase& parent)
		: parent(parent)
		, sys{parent, 1.0, 0.05}
	{}

	void show() {
		if(parent.is_show_coordinates) {
			sys.show();
		}
	}
};

} /** namespace robo */
