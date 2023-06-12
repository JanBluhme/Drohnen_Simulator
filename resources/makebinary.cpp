#include <charconv>
#include <fstream>
#include <iostream>
#include <vector>
#include <string_view>

int make_hpp(
	  std::string_view Infile
	, std::string_view Namespace
	, std::string_view Name
) {
	std::cout << "#pragma once\n";
	std::cout << "#include <string_view>\n";
	std::cout << "namespace " << Namespace << " {\n";
	std::cout << "std::string_view " << Name << "() noexcept;\n";
	std::cout << "}\n";
	return 0;
}

int make_cpp(
	  std::string_view Infile
	, std::string_view Namespace
	, std::string_view Name
	, std::string_view HeaderFile
) {
	std::ifstream file(std::string{Infile}, std::ios_base::in | std::ios_base::binary);
	if(!file.is_open()) {
		std::cerr << "cant read file " << Infile << "\n";
		return 1;
	}
	std::cout << "#include \"" << HeaderFile << "\"\n";
	std::cout << "namespace " << Namespace << " {\n";
	std::cout << "std::string_view " << Name << "() noexcept{\n";
	std::cout << "    static constexpr char const data[] {\n";
	std::vector<char> buffer;
	std::size_t       writecount = 0;
	bool              first      = true;
	while(file) {
		buffer.resize(1024);
		buffer.resize(file.readsome(buffer.data(), buffer.size()));
		if(buffer.empty()) {
			break;
		}

		for(auto c : buffer) {
			char buf[24];
			auto it = std::begin(buf);
			*it++   = first ? ' ' : ',';
			*it++   = ' ';
			*it++   = '\'';
			*it++   = '\\';

			auto res = std::to_chars(it, std::end(buf), static_cast<unsigned char>(c), 8);
			if(res.ec != std::errc()) {
				std::cerr << "fail...\n";
				return 1;
			}
			auto charsSize = std::distance(it, res.ptr);
			if(charsSize > 3 || charsSize == 0) {
				std::cerr << "fail...\n";
			}
			if(charsSize == 1) {
				char tmp = *it;
				*it++    = '0';
				*it++    = '0';
				*it++    = tmp;
			} else if(charsSize == 2) {
				auto start = it;
				char tmp1  = *it++;
				char tmp2  = *it;
				*start++   = '0';
				*start++   = tmp1;
				*start++   = tmp2;
				it         = start;
			} else {
				it = res.ptr;
			}

			*it++ = '\'';
			++writecount;
			if(writecount % 8 == 0) {
				*it++ = '\n';
				for(std::size_t i = 0; i < 8; ++i) {
					*it++ = ' ';
				}
			}
			std::cout << std::string_view{
				  std::begin(buf)
				, static_cast<std::size_t>(std::distance(std::begin(buf), it))
			};
			first = false;
		}
	}

	std::cout << "\n    };";
	std::cout << "\n    return {&data[0], sizeof(data)};\n";
	std::cout << "}\n}\n\n";
	return 0;
}

int main(int argc, char const* const* argv) {
	auto usage = [&]() {
		std::cerr << argv[0] << " hpp infile namespace name\n";
		std::cerr << "    -> generate header file\n";
		std::cerr << argv[0] << " cpp infile namespace name header_file\n";
		std::cerr << "    -> generate source file\n";
	};
	if(argc < 5 || argc > 6) {
		usage();
		return 1;
	}
	if(std::string_view{argv[1]} == "cpp") {
		return make_cpp(argv[2], argv[3], argv[4], argv[5]);
	}
	if(std::string_view{argv[1]} == "hpp") {
		return make_hpp(argv[2], argv[3], argv[4]);
	}
	usage();
	return 1;
}
