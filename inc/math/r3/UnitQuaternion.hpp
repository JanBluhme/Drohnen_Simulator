#pragma once
#include "math/Matrix.hpp"
#include <iostream>
#include <iomanip>
class UnitQuaternion {
public:
	using V = Vertex<double, 3>;
	using M = Matrix<double, 3, 3>;

private:
	double w;
	double x;
	double y;
	double z;

	constexpr UnitQuaternion(double w, double x, double y, double z) noexcept
		: w{w}
		, x{x}
		, y{y}
		, z{z}
	{}

public:
	friend
	constexpr auto dot(UnitQuaternion const& a, UnitQuaternion const& b) noexcept
		-> double
	{
		return
			  a.w * b.w
			+ a.x * b.x
			+ a.y * b.y
			+ a.z * b.z
		;
	}

	// http://www.mrelusive.com/publications/papers/SIMD-From-Quaternion-to-Matrix-and-Back.pdf
	// note: above link uses alternative quaternion convention, see
	//    https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation#Alternative_conventions
	// -> rotation matrices are transposed in original and this implementation...
	UnitQuaternion(M const& m) noexcept {
		if(m[0][0] + m[1][1] + m[2][2] > 0.0) {
			double t = + m[0][0] + m[1][1] + m[2][2] + 1.0;
			double s = 0.5 * std::sqrt(1.0 / t);
			w = s * t;
			z = (m[1][0] - m[0][1]) * s;
			y = (m[0][2] - m[2][0]) * s;
			x = (m[2][1] - m[1][2]) * s;
		} else if(m[0][0] > m[1][1] && m[0][0] > m[2][2]) {
			double t = + m[0][0] - m[1][1] - m[2][2] + 1.0;
			double s = 0.5 * std::sqrt(1.0 / t);
			x = s * t;
			y = (m[1][0] + m[0][1]) * s;
			z = (m[0][2] + m[2][0]) * s;
			w = (m[2][1] - m[1][2]) * s;
		} else if(m[1][1] > m[2][2]) {
			double t = - m[0][0] + m[1][1] - m[2][2] + 1.0;
			double s = 0.5 * std::sqrt(1.0 / t);
			y = s * t;
			x = (m[1][0] + m[0][1]) * s;
			w = (m[0][2] - m[2][0]) * s;
			z = (m[2][1] + m[1][2]) * s;
		} else {
			double t = - m[0][0] - m[1][1] + m[2][2] + 1.0;
			double s = 0.5 * std::sqrt(1.0 / t);
			z = s * t;
			w = (m[1][0] - m[0][1]) * s;
			x = (m[0][2] + m[2][0]) * s;
			y = (m[2][1] + m[1][2]) * s;
		}
	}

	// http://www.mrelusive.com/publications/papers/SIMD-From-Quaternion-to-Matrix-and-Back.pdf
	// note: above link uses alternative quaternion convention, see
	//    https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation#Alternative_conventions
	// -> rotation matrices are transposed in original and this implementation...
	constexpr auto matrix() const noexcept
		-> M
	{
		double norm2 = dot(*this, *this);
		// note: s = 2 should do, scaling with norm2 is done to avoid cumulated numerical errors
		double s  = norm2 > 1e-6
			? 2.0 / norm2
			: 0.0
		;
		double wx = s * w * x;
		double wy = s * w * y;
		double wz = s * w * z;
		double xx = s * x * x;
		double xy = s * x * y;
		double xz = s * x * z;
		double yy = s * y * y;
		double yz = s * y * z;
		double zz = s * z * z;
		return {
			  V{1.0 - yy - zz,       xy - wz,       xz + wy}
			, V{      xy + wz, 1.0 - xx - zz,       yz - wx}
			, V{      xz - wy,       yz + wx, 1.0 - xx - yy}
		};
	}

	// https://www.geometrictools.com/Documentation/Quaternions.pdf
	auto pow(double t) const noexcept
		-> UnitQuaternion
	{
		// Equating coefficients:
		//    this->w = cos(omega)
		//    this->x = sin(omega) * v_x
		//    this->y = sin(omega) * v_y
		//    this->z = sin(omega) * v_z
		//    this->pow(t) -> cos(omega * t), [v_x, v_y, v_z] * sin(omega * t)
		//                 -> cos(omega * t), [  x,   y,   z] * sin(omega * t) / sin(omega)
		//    , limit_{omega -> 0}{sin(omega * t) / sin(omega)} = t

		// note: s = 1 should do, scaling with norm is done to avoid cumulated numerical errors
		double s     = 1.0 / std::sqrt(dot(*this, *this));
		double omega = std::acos(w * s);

		// limit_{omega -> 0}{sin(omega * t) / sin(omega)} = t
		double scale = std::abs(omega) > 1e-6
			? s * std::sin(omega * t) / std::sin(omega)
			: s * t
		;
		V      v     = V{x, y, z} * scale;
		return {
			std::cos(omega * t), v[0], v[1], v[2]
		};
	}

	friend
	constexpr auto operator*(UnitQuaternion const& a, UnitQuaternion const& b) noexcept
		-> UnitQuaternion
	{
		return {
			  a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z
			, a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y
			, a.w * b.y + a.y * b.w + a.z * b.x - a.x * b.z
			, a.w * b.z + a.z * b.w + a.x * b.y - a.y * b.x
		};
	}

	constexpr auto operator-() const noexcept
		-> UnitQuaternion
	{
		return {-w, -x, -y, -z};
	}

	constexpr auto invert() const noexcept
		-> UnitQuaternion
	{
		// norm          : x*x + y*y + z*z + w*w
		// UnitQuaternion: norm == 1;
		// conjugate     : -x, -y, -z, w
		// inv           : conjugate / norm
		return {w, -x, -y, -z};
	}

	// Spherical Linear Interpolation
	//     https://www.geometrictools.com/Documentation/Quaternions.pdf
	// 0 <= t <= 1
	friend
	auto slerp(UnitQuaternion const& q0, UnitQuaternion const& q1, double t) noexcept
		-> UnitQuaternion
	{
		return  q0 * (q0.invert() * q1).pow(t);
	}

	friend
	auto slerp_short_arc(UnitQuaternion const& q0, UnitQuaternion const& q1, double t) noexcept
		-> UnitQuaternion
	{
		if(dot(q0, q1) < 0.0) {
			return slerp(-q0, q1, t);
		} else {
			return slerp( q0, q1, t);
		}
	}

	template<typename OS>
	friend
	OS& operator<<(OS& os, UnitQuaternion const& q) {
		os  << '{'   << q.w
			<< ", [" << q.x
			<< ", "  << q.y
			<< ", "  << q.z
			<< "]}"
		;
		return os;
	}
};
