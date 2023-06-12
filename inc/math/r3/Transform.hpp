#pragma once
#include "math/Matrix.hpp"
#include "math/r3/UnitQuaternion.hpp"
#include "math/r3/Rotation.hpp"

struct Transform {
	using V = Vertex<double, 3>;
	Rotation D{};
	V        T{};

	Matrix<double, 3, 3> const& rotation_matrix() const {
		return D.D;
	}

	auto position() const noexcept
		->V const&
	{
		return T;
	}

	auto rotation() const noexcept
		-> Rotation const&
	{
		return D;
	}

	auto as_44() const noexcept
		-> Matrix<double, 4, 4>
	{
		return {
			  Vertex<double, 4>{D.D[0][0], D.D[0][1], D.D[0][2], T[0]}
			, Vertex<double, 4>{D.D[1][0], D.D[1][1], D.D[1][2], T[1]}
			, Vertex<double, 4>{D.D[2][0], D.D[2][1], D.D[2][2], T[2]}
			, Vertex<double, 4>{0.0      , 0.0      , 0.0      , 1.0 }
		};
	}
	auto as_gl_matrix() const noexcept
		-> std::array<double, 16>
	{
		Matrix<double, 4, 4> T = as_44().transpose();
		return {
			  T[0][0], T[0][1], T[0][2], T[0][3]
			, T[1][0], T[1][1], T[1][2], T[1][3]
			, T[2][0], T[2][1], T[2][2], T[2][3]
			, T[3][0], T[3][1], T[3][2], T[3][3]
		};
	}

	constexpr auto direction(V const& v) const noexcept
		-> V
	{
		return D * v;
	}

	constexpr auto position(V const& v) const noexcept
		-> V
	{
		return D * v + T;
	}

	constexpr auto invert() const noexcept
		-> Transform
	{
		// y = D * x + T
		// y - T = D * x
		// inverse(D) * (y - T) = x
		// inverse(D) * y + -inverse(D) * T = x
		// inverse(D) * y + inverse(D) * (-T) = x
		Rotation invD = D.invert();
		return {
			invD, invD * -T
		};
	}

	// 0 <= t <= 1
	friend
	auto interpolate(Transform const& a, Transform const& b, double t) noexcept
		-> Transform
	{
		return {
			  slerp_short_arc(
				    a.D.quaternion()
				  , b.D.quaternion()
				  , t
			  )
			, a.T + (b.T - a.T) * t
		};
	}

	friend
	constexpr auto operator*(Transform const& A, Transform const& B) noexcept
		-> Transform
	{
		// y1 = B.D * x + B.T
		// y2 = A.D * y1 + A.T
		// y2 = A.D * (B.D * x + B.T) + A.T
		// y2 = A.D * B.D * x + A.D * B.T + A.T
		//
		// y2 = A * B * x
		// D(A * B) <- A.D * B.D
		// T(A * B) <- A.D * B.T + A.T
		return {
			A.D * B.D, A.D * B.T + A.T
		};
	}

	static auto translate(V const& v) noexcept
		-> Transform
	{
		return {Rotation{}, v};
	}

	static auto translate(double x, double y, double z) noexcept
		-> Transform
	{
		return translate(V{x, y, z});
	}

	static auto rotate_x(double phi) noexcept
		-> Transform
	{
		return {Rotation::rotation_x(phi)};
	}

	static auto rotate_y(double phi) noexcept
		-> Transform
	{
		return {Rotation::rotation_y(phi)};
	}

	static auto rotate_z(double phi) noexcept
		-> Transform
	{
		return {Rotation::rotation_z(phi)};
	}

	template<int pi_halfs>
	constexpr static auto rotate_x_pi_halfs() noexcept
		-> Transform
	{
		return {Rotation::rotation_x_pi_halfs<pi_halfs>()};
	}

	template<int pi_halfs>
	constexpr static auto rotate_y_pi_halfs() noexcept
		-> Transform
	{
		return {Rotation::rotation_y_pi_halfs<pi_halfs>()};
	}

	template<int pi_halfs>
	constexpr static auto rotate_z_pi_halfs() noexcept
		-> Transform
	{
		return {Rotation::rotation_z_pi_halfs<pi_halfs>()};
	}

	static auto rotation(V const& axis, double phi) noexcept
		-> Transform
	{
		return {Rotation::rotation(axis, phi)};
	}

	static auto roll_pitch_yaw(double roll, double pitch, double yaw) noexcept
		-> Transform
	{
		return {Rotation::roll_pitch_yaw(roll, pitch, yaw)};
	}

	auto roll() const noexcept
		-> double
	{
		return D.roll();
	}

	auto pitch() const noexcept
		-> double
	{
		return D.pitch();
	}

	auto yaw() const noexcept
		-> double
	{
		return D.yaw();
	}

	static auto look_at(Vertex<double, 3> const& eye, Vertex<double, 3> const& center, Vertex<double, 3> const& up) noexcept
		-> Transform
	{
		Vertex<double,3> f = (center - eye).normalize();
		Vertex<double,3> s = cross_product(f, up.normalize());
		Vertex<double,3> u = cross_product(s.normalize(), f);
		Transform R{Rotation{Matrix<double,3,3> {s, u, -f}}};
		Transform T = translate(-eye);
		return R * T;
	}

	template<typename OS>
	friend
	OS& operator<<(OS& os, Transform const& t) {
		auto& D = t.rotation_matrix();
		os  << D[0] << " | " << t.T[0] << '\n'
			<< D[1] << " | " << t.T[1] << '\n'
			<< D[2] << " | " << t.T[2] << '\n'
		;
		return os;
	}
};
