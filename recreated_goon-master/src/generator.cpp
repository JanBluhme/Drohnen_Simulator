#include "Interpreter.h"
#include "Printer.h"
#include "command.h"

#include <fstream>
#include <iostream>

struct CommandLineArguments {
    std::vector<std::string> args;

    CommandLineArguments(int argc, char** argv) : args(argv, argv + argc) {}

    auto
      find_prefix(std::string_view prefix) const {
        using std::begin;
        using std::end;
        return std::find_if(begin(args), end(args), [&](std::string_view v) {
            return v.substr(0, prefix.size()) == prefix;
        });
    }

    bool
      has_prefix(std::string_view prefix) const {
        using std::end;
        return find_prefix(prefix) != end(args);
    }

    template<typename T>
    T
      get(std::string_view prefix, std::optional<T> const& default_value = std::nullopt) const {
        using std::end;
        auto it = find_prefix(prefix);
        if(it != end(args)) {
            std::stringstream ss(it->substr(prefix.size()));
            T                 value;
            if(ss >> value) {
                return value;
            }
        }
        if(!default_value) {
            std::cerr << "parameter \"" << prefix << "\" not found\n";
            std::exit(1);
        }
        return *default_value;
    }
};

// https://github.com/ezaquarii/bison-flex-cpp-example
int
  main(int argc, char** argv) {
    CommandLineArguments args{argc, argv};

    bool const requests_without_response_allowed
      = args.has_prefix("--requests_without_response_allowed");
    auto const ifilename = args.get<std::string>("--input=");
    auto const ofilename = args.get<std::string>("--output=");
    auto const generator = args.get<std::string>("--generator=");

    std::ifstream ifile(ifilename);
    std::ofstream ofile(ofilename);

    if(!ifile.is_open()) {
        std::cerr << "cant open IDL file \"" << ifilename << "\"\n";
        return 1;
    }

    if(!ofile.is_open()) {
        std::cerr << "cant generate output file \"" << ofilename << "\"\n";
        return 1;
    }

    worse_io::Interpreter<worse_io::ContentType> i(ifile, requests_without_response_allowed);
    if(i.parse() != 0) {
        std::cerr << "IDL parse failed\n";
        return 1;
    }

    if(generator == "cpp_command") {
        worse_io::CppCommandPrinter printer{ofile, i};
        printer.args(args);
        printer.print();
    } else if(generator == "cpp_environment") {
        worse_io::CppEnvironmentPrinter printer{ofile, i};
        printer.args(args);
        printer.print();
    } else {
        std::cerr << "unknown generator \"" << generator << "\"\n";
        return 1;
    }

    return 0;
}
