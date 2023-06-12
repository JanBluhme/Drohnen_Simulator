#pragma once
#include "math/Matrix.hpp"
#include "math/r3/UnitQuaternion.hpp"

namespace detail::math_foo {
	//  1% 4 ->  1 (+0) -> 1
	// -1% 4 -> -1 (+4) -> 3
	// -1%-4 -> -1 (+0) ->-1
	//  1%-4 ->  1 (-4) ->-3
	template<int a, int b>
	constexpr int modulo = a % b + (a < 0 != b < 0 ? b : 0);

	template<int pi_halfs>
	           inline constexpr double sin_pi_half    =  sin_pi_half<modulo<pi_halfs,4>>;
	template<> inline constexpr double sin_pi_half<0> =  0.0;
	template<> inline constexpr double sin_pi_half<1> =  1.0;
	template<> inline constexpr double sin_pi_half<2> =  0.0;
	template<> inline constexpr double sin_pi_half<3> = -1.0;

	template<int pi_halfs>
	inline constexpr double cos_pi_half = sin_pi_half<pi_halfs + 1>;
} /* namespace detail::math_foo */

class Rotation {
public:
	using V = Vertex<double, 3>;
	using M = Matrix<double, 3, 3>;

private:
	friend class Transform;

	M D = identity<double, 3>();

	constexpr Rotation(M const& D) noexcept
		: D{D}
	{}

	static constexpr auto rotation_x(double cos, double sin) noexcept
		-> Rotation
	{
		return M{
			  V{1.0, 0.0,  0.0}
			, V{0.0, cos, -sin}
			, V{0.0, sin,  cos}
		};
	}

	static constexpr auto rotation_y(double cos, double sin) noexcept
		-> Rotation
	{
		return M{
			  V{ cos, 0.0, sin}
			, V{ 0.0, 1.0, 0.0}
			, V{-sin, 0.0, cos}
		};
	}

	static constexpr auto rotation_z(double cos, double sin) noexcept
		-> Rotation
	{
		return M{
			  V{cos, -sin, 0.0}
			, V{sin,  cos, 0.0}
			, V{0.0,  0.0, 1.0}
		};
	}

	static constexpr auto rotation(V const& n, double cos, double sin) noexcept
		-> Rotation
	{
		M N {
			  n * n[0]
			, n * n[1]
			, n * n[2]
		};
		M D {
			  V{+cos       , -sin * n[2], +sin * n[1]}
			, V{+sin * n[2], +cos       , -sin * n[0]}
			, V{-sin * n[1], +sin * n[0], +cos       }
		};
		return {N * (1.0 - cos) + D};
	}

public:
	constexpr Rotation() noexcept = default;
	constexpr Rotation(UnitQuaternion const& q) noexcept
		: D{q.matrix()}
	{}
	constexpr auto get_D() const noexcept
		-> M const&
	{
		return D;
	}

	auto quaternion() const noexcept
		-> UnitQuaternion
	{
		  return {D};
	}

	friend
	constexpr auto operator*(Rotation const& R, V const& x) noexcept
		-> V
	{
		return R.D * x;
	}

	friend
	constexpr auto operator*(Rotation const& A, Rotation const& B) noexcept
		-> Rotation
	{
		return {A.D * B.D};
	}

	constexpr auto invert() const noexcept
		-> Rotation
	{
		return {simple_inverse_orthonormal(D)};
	}

	static auto rotation_x(double phi) noexcept
		-> Rotation
	{
		return rotation_x(std::cos(phi), std::sin(phi));
	}

	static auto rotation_y(double phi) noexcept
		-> Rotation
	{
		return rotation_y(std::cos(phi), std::sin(phi));
	}

	static auto rotation_z(double phi) noexcept
		-> Rotation
	{
		return rotation_z(std::cos(phi), std::sin(phi));
	}

	static auto rotation(V const& axis, double phi) noexcept
		-> Rotation
	{
		return rotation(axis.normalize(), std::cos(phi), std::sin(phi));
	}

	template<int pi_halfs>
	constexpr static auto rotation_x_pi_halfs() noexcept
		-> Rotation
	{
		using namespace detail::math_foo;
		return rotation_x(cos_pi_half<pi_halfs>, sin_pi_half<pi_halfs>);
	}

	template<int pi_halfs>
	constexpr static auto rotation_y_pi_halfs() noexcept
		-> Rotation
	{
		using namespace detail::math_foo;
		return rotation_y(cos_pi_half<pi_halfs>, sin_pi_half<pi_halfs>);
	}

	template<int pi_halfs>
	constexpr static auto rotation_z_pi_halfs() noexcept
		-> Rotation
	{
		using namespace detail::math_foo;
		return rotation_z(cos_pi_half<pi_halfs>, sin_pi_half<pi_halfs>);
	}

	// lavalle: http://planning.cs.uiuc.edu/node102.html
	static auto roll_pitch_yaw(double roll, double pitch, double yaw) noexcept
		-> Rotation
	{
		return rotation_z(yaw) * rotation_y(pitch) * rotation_x(roll);
	}

	// lavalle: http://planning.cs.uiuc.edu/node103.html
	auto roll() const noexcept
		-> double
	{
		return std::atan2(D[2][1], D[2][2]);
	}

	// lavalle: http://planning.cs.uiuc.edu/node103.html
	auto pitch() const noexcept
		-> double
	{
		return std::atan2(-D[2][0], std::sqrt(D[2][1]*D[2][1] + D[2][2]*D[2][2]));
	}

	// lavalle: http://planning.cs.uiuc.edu/node103.html
	auto yaw() const noexcept
		-> double
	{
		return std::atan2(D[1][0], D[0][0]);
	}

	template<typename OS>
	friend
	OS& operator<<(OS& os, Rotation const& r) {
		os << r.D;
		return os;
	}
};
