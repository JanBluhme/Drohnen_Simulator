#include "math/Matrix.hpp"
#include "math/r3/Transform.hpp"
#include <iostream>

void test_matrix_inverse() {
	Matrix<double, 3,3> A {
		  Vertex<double,3>{ 1.0,  2.0,  3.0}
		, Vertex<double,3>{ 7.0,  9.0, 11.0}
		, Vertex<double,3>{13.0, 10.0,  5.0}
	};
	std::cout << "A:" << A << '\n';
	auto Ai = invert(A);
	std::cout << "Ai:" << Ai << '\n';
	auto AiA = Ai * A;
	std::cout << "Ai A:" << AiA << '\n';
	auto I = identity<double,3>();
	auto E = AiA - I;
	std::cout << "E:" << E << '\n';
	std::cout << "E_max: " << E.abs_max() << '\n';
}


auto test_quaternion_matrix_conversion()
	-> double
{
	double d_max = 0.0;
	for(double y = -M_PI; y < M_PI; y += 0.1) {
		for(double p = -M_PI; p < M_PI; p += 0.1) {
			for(double r = -M_PI; r < M_PI; r += 0.1) {
				using V = Vertex<double, 3>;
				Rotation      D = Rotation::roll_pitch_yaw(r, p, y);
				UnitQuaternion Q = D.quaternion();
				Rotation      DD{Q};
				auto delta = [&](V const& v) {
					V a = D * v;
					V b = DD * v;
					V d = a - b;
					return d * d;
				};
				double max = std::max({
					  delta(V{1.0, 0.0, 0.0})
					, delta(V{0.0, 1.0, 0.0})
					, delta(V{0.0, 0.0, 1.0})
				});
				d_max = std::max(max, d_max);
// 				std::cout << y << ", " << p << ", " << r << " -> " << d_max << ", " << max << '\n';
			}
		}
	}
	return d_max;
}

template<typename OS, typename T>
OS& funny_print(OS& os, unsigned int a, unsigned int b, T const& x) {
	if constexpr(
		   std::is_same_v<T, float>
		|| std::is_same_v<T, double>
		|| std::is_same_v<T, long double>
	) {
		std::array<char, 46> fmt;
		snprintf(fmt.data(), fmt.size(), "%%%u.%uf", a + b + 1, b);
// 		std::cerr << "#### : " << fmt.data() << " ####\n";
		std::array<char, 1024> buffer;
		if(snprintf(buffer.data(), buffer.size(), fmt.data(), x) < buffer.size()) {
			os << buffer.data();
		} else {
			os << x;
		}
	} else {
		os << x;
	}
	return os;
}
template<typename OS, typename T, std::size_t N>
OS& funny_print(OS& os, unsigned int a, unsigned int b, Vertex<T, N> const& v) {
	os << '[';
	if constexpr(N) {
		funny_print(os, a, b, v[0]);
		[&]<std::size_t... Is>(std::index_sequence<0, Is...>) {
			((os << ", ", funny_print(os, a, b, v[Is])), ...);
		}(std::make_index_sequence<N>{});
	}
	os << ']';
	return os;
}
template<typename OS, typename T, std::size_t N, std::size_t M>
OS& funny_print(OS& os, unsigned int a, unsigned int b, Matrix<T, N, M> const& v) {
	os << "[\n";
	[&]<std::size_t... Is>(std::index_sequence<Is...>) {
		((os << "  ", funny_print(os, a, b, v[Is]) << '\n'), ...);
	}(std::make_index_sequence<N>{});
	os << "]\n";
	return os;
}
template<typename OS>
OS& funny_print(OS& os, unsigned int a, unsigned int b, Rotation const& v) {
	return funny_print(os, a, b, v.get_D());
}
template<typename OS>
OS& funny_print(OS& os, unsigned int a, unsigned int b, Transform const& T) {
	os << "[\n";
	for(std::size_t i = 0; i < 3; ++i) {
		os << "    ";
		funny_print(os, a, b, T.D.get_D()[i]);
		os << " | ";
		funny_print(os, a, b, T.T[i]);
		os << '\n';
	}
	os << "]\n";
	return os;
}

std::pair<Rotation, std::string> bla(std::size_t Dir, int N) {
	auto l = []<int N>(std::size_t Dir, std::integral_constant<int, N>)
		-> std::pair<Rotation, std::string>
	{
		switch(Dir % 3) {
			default:
			case 0: return {Rotation::rotation_x_pi_halfs<N>(), "rot_x<" + std::to_string(N) + ">"};
			case 1: return {Rotation::rotation_y_pi_halfs<N>(), "rot_y<" + std::to_string(N) + ">"};
			case 2: return {Rotation::rotation_z_pi_halfs<N>(), "rot_z<" + std::to_string(N) + ">"};
		}
	};
	N %= 4;
	if(N < 0) {
		N += 4;
	}
	switch(N) {
		default:
		case 0: return l(Dir, std::integral_constant<int, 0>{});
		case 1: return l(Dir, std::integral_constant<int, 1>{});
		case 2: return l(Dir, std::integral_constant<int, 2>{});
		case 3: return l(Dir, std::integral_constant<int, 3>{});
	}
}

void foo() {
	double a = 1.0;
	double c = 2.0;
	double phi = std::atan(a / c);
	Vertex<double,3> eye{-c, 0.0, a};
	Transform A
		= Transform{}
		* Transform::translate(eye)
		* Transform::rotate_y(phi)
	;
	Transform B = Transform::look_at(
		  eye
		, Vertex<double,3>{0.0, 0.0, 0.0}
		, Vertex<double,3>{1.0, 0.0, 2.0}
	);

	std::cout << "A" << '\n';
	funny_print(std::cout, 2, 3, A);
	std::cout << "B" << '\n';
	funny_print(std::cout, 2, 3, B);
	std::cout << "A.D" << '\n';
	funny_print(std::cout, 2, 3, A.D);

	auto diff = [](Rotation const& a, Rotation const& b) {
		Matrix<double,3,3> const& A = a.get_D();
		Matrix<double,3,3> const& B = b.get_D();
		Matrix<double,3,3> D = A - B;
		double max = 0.0;
		for(std::size_t i = 0; i < 3; ++i) {
			for(std::size_t j = 0; j < 3; ++j) {
				max = std::max(max, std::abs(D[i][j]));
			}
		}
		return max;
// 		return (a.get_D() - b.get_D()).abs_max();
	};
// 	for(std::size_t dir_1 = 0; dir_1 < 3; ++dir_1) {
// 		for(int N_1 = 0; N_1 < 4; ++N_1) {
// 			auto p_1 = bla(dir_1, N_1);
// 			for(std::size_t dir_2 = 0; dir_2 < 3; ++dir_2) {
// 				if(dir_1 == dir_2) {
// 					continue;
// 				}
// 				for(int N_2 = 0; N_2 < 4; ++N_2) {
// 					auto p_2 = bla(dir_2, N_2);
// 					auto C = p_1.first * p_2.first * B.D;
// // 					auto C = B.D * p_1.first * p_2.first;
// // 					auto C = p_1.first * B.D *  p_2.first;
// 					C = C.invert();
// 					if(diff(C, A.D) < 0.1) {
// 						std::cout
// 							<< p_1.second
// 							<< ", " << p_2.second << ": " << diff(C, A.D) << '\n';
// 						;
// 						funny_print(std::cout, 2, 3, C);
// 					}
// 				}
// 			}
// 		}
// 	}

// 	funny_print(std::cout, 2, 3,invert((Rotation::rotation_x_pi_halfs<1>() * Rotation::rotation_y_pi_halfs<-1>() * B.D).get_D()));
// 	funny_print(std::cout, 2, 3,(Rotation::rotation_x_pi_halfs<1>() * Rotation::rotation_y_pi_halfs<-1>() * B.D).invert());
	funny_print(std::cout, 2, 3,(Transform::rotate_x_pi_halfs<1>() * Transform::rotate_y_pi_halfs<-1>() * B).invert());
}

int main() {
// 	double a = test_quaternion_matrix_conversion();
// 	std::cout << "matrix_conversion_e_max: " << a << '\n';
	foo();
	return 0;
}
