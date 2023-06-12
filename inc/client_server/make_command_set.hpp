#pragma once
#include <variant>

template<typename OS, typename... Ts>
void print_variant(OS& os, std::variant<Ts...> const& v) {
	std::visit(
		[&](auto x) {
			os << x;
		}
		, v
	);
}

template<typename... Ts>
struct make_command_set {
	using request_variant_t  = std::variant<typename Ts::Request...>;
	using response_variant_t = std::variant<typename Ts::Response...>;
	struct Response;
	
	template<std::size_t I>
	using command = std::tuple_element_t<I,std::tuple<Ts...>>;
	
	constexpr static std::size_t size = sizeof...(Ts);
	
	struct Response {
		response_variant_t response;
		
		Response() = default;
		Response(response_variant_t response)
			: response(std::move(response))
		{}
		
		template<typename Serializer, typename Buffer>
		static bool serialize(Buffer& buffer, Response const& r) {
			return Serializer::serialize(buffer, r.response);
		}
		template<typename Serializer, typename Buffer>
		static bool deserialize(Buffer& buffer, Response& r) {
			return Serializer::deserialize(buffer, r.response);
		}
		template<typename OS>
		friend
		OS& operator<<(OS& os, Response const& r) {
			os << "CommandSet::Response:[";
			print_variant(r.response);
			return os << ']';
		}
	};
	
	struct Request {
		request_variant_t  request;
		
		Request() = default;
		Request(request_variant_t  request)
			: request(std::move(request))
		{}
		
		template<typename Serializer, typename Buffer>
		static bool serialize(Buffer& buffer, Request const& r) {
			return Serializer::serialize(buffer, r.request);
		}
		template<typename Serializer, typename Buffer>
		static bool deserialize(Buffer& buffer, Request& r) {
			return Serializer::deserialize(buffer, r.request);
		}
		template<typename OS>
		friend
		OS& operator<<(OS& os, Request const& r) {
			os << "CommandSet::Request:[";
			print_variant(r.request);
			return os << ']';
		}
	};
};

template<typename... Ts>
auto make_command_set_from_variant_(std::variant<Ts...>)
	-> make_command_set<Ts...>
{
	return {};
}

template<typename Variant>
using make_command_set_from_variant = decltype(make_command_set_from_variant_(Variant{}));

