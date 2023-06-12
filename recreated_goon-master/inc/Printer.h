#pragma once
#include "command.h"

#include <string>

namespace worse_io {

struct cpp {
    template<typename B, typename C>
    static void
      extractCommands(B& b, C const& content) {
        for(auto const& c : content) {
            if(std::holds_alternative<Command>(c)) {
                b.push_back(std::get<Command>(c));
            }
            if(std::holds_alternative<Namespace>(c)) {
                extractCommands(b, std::get<Namespace>(c).body.content);
            }
        }
    }

    template<typename C>
    static auto
      extractCommands(C const& content) {
        std::vector<Command> commands;
        extractCommands(commands, content);
        return commands;
    }

    template<typename B, typename C>
    static void
      extractCommandSets(B& b, C const& content) {
        for(auto const& c : content) {
            if(std::holds_alternative<CommandSet>(c)) {
                b.push_back(std::get<CommandSet>(c));
            }
            if(std::holds_alternative<Namespace>(c)) {
                extractCommandSets(b, std::get<Namespace>(c).body.content);
            }
        }
    }

    template<typename C>
    static auto
      extractCommandSets(C const& content) {
        std::vector<CommandSet> commandsets;
        extractCommandSets(commandsets, content);
        return commandsets;
    }

    template<typename B>
    static auto
      extractStructsByName(B const& b, std::string_view name) {
        std::vector<Struct> ret;
        for(auto const& c : b) {
            for(auto const& s : c.body.structs()) {
                if(s.name == name) {
                    ret.push_back(s);
                }
            }
        }
        return ret;
    }

    template<typename C>
    static auto
      extractRequests(C const& content) {
        auto const commands = extractCommands(content);
        return extractStructsByName(commands, "Request");
    }

    template<typename C>
    static auto
      extractResponses(C const& content) {
        auto const commands = extractCommands(content);
        return extractStructsByName(commands, "Response");
    }

    static std::string
      print_common(unsigned long long x) {
        return std::to_string(x);
    }

    template<typename I>
    std::string static print_static_cast_type(
      BaseClass const& x,
      Struct const&    s,
      I const&         interpreter) {
        auto tt = get_InterpreterTypeType_impl<typename I::ContentType>(interpreter, x.content);

        if(tt.first != InterpreterTypeType::None) {
            if(!tt.second) {
                throw std::runtime_error{"wtf??"};
            }
            return std::visit(
              [](auto const& v) -> std::string {
                  if constexpr(requires { v.full_name; }) {
                      return v.full_name;
                  }
                  throw std::runtime_error{"wtf"};
              },
              *tt.second);
        }

        std::string ret;
        std::string name = print_common(x.content);
        //TODO probably wrong
        if(!name.starts_with("::") && s.full_name != s.name) {
            ret += s.full_name.substr(0, s.full_name.size() - s.name.size());
        }
        ret += name;
        return ret;
    }

    std::string static print_common(UnscopedTypename const& x) {
        std::string ret;
        ret += x.name;
        if(x.list) {
            ret += '<';
            ret += cpp::print_common(*x.list);
            ret += '>';
        }
        return ret;
    }

    static std::string
      print_common(Typename const& x) {
        std::string ret;
        for(auto const& v : x.scoped_name) {
            ret += cpp::print_common(v);
        }
        return ret;
    }

    template<typename... Ts>
    static std::string
      print_common(ListItem<Ts...> const& x) {
        std::string ret;
        std::visit([&](auto const& v) { ret += cpp::print_common(v); }, x.content);
        return ret;
    }

    template<typename... Ts>
    static std::string
      print_common(List<Ts...> const& x) {
        std::string ret;
        for(bool first = true; auto const& v : x.content) {
            if(!first) {
                ret += ',';
            }
            first = false;
            ret += cpp::print_common(v);
        }
        return ret;
    }

    template<typename OS>
    struct NamespacePrinter {
        std::size_t num;
        OS&         os;
        NamespacePrinter(OS& os_, std::vector<std::string> const& namespaces)
          : os{os_}
          , num{namespaces.size()} {
            for(auto const& n : namespaces) {
                os << "namespace ";
                os << n;
                os << '{';
            }
        }
        ~NamespacePrinter() {
            while(num--) {
                os << '}';
            }
        }
    };

    static bool
      isSameType(
        std::vector<std::string> const& namespaces,
        std::string_view                fullname,
        std::string_view                name,
        Typename const&                 type) {
        auto typeString = print_common(type);
        if(typeString == name) {
            return true;
        }
        if(typeString == fullname) {
            return true;
        }
        if(
          typeString.starts_with("::") && fullname.starts_with("::")
          && typeString.substr(2) == fullname.substr(2))
        {
            return true;
        }
        std::vector<std::string> reverse_namespaces{namespaces};
        std::reverse(begin(reverse_namespaces), end(reverse_namespaces));

        std::string namespacedName = std::string{name};

        for(auto const& namesp : reverse_namespaces) {
            namespacedName = namesp + "::" + namespacedName;
            if(typeString == namespacedName) {
                return true;
            }

            if(typeString.starts_with("::") && typeString.substr(2) == namespacedName) {
                return true;
            }
        }
        //TODO correct?

        return false;
    }

    enum class InterpreterTypeType { None, Struct, Enum, Using, Using_To_Enum, Using_To_Struct };

    template<typename T>
    static std::pair<InterpreterTypeType, T>
      get_InterpreterTypeType_(CommandSet const& c, Typename const& type) {
        return {InterpreterTypeType::None, {}};
    }
    template<typename T>
    static std::pair<InterpreterTypeType, T>
      get_InterpreterTypeType_(Attribute const& c, Typename const& type) {
        return {InterpreterTypeType::None, {}};
    }
    template<typename T>
    static std::pair<InterpreterTypeType, T>
      get_InterpreterTypeType_(Injection const& c, Typename const& type) {
        return {InterpreterTypeType::None, {}};
    }
    template<typename T>
    static std::pair<InterpreterTypeType, T>
      get_InterpreterTypeType_(BaseClasses const& c, Typename const& type) {
        return {InterpreterTypeType::None, {}};
    }

    template<typename T>
    static std::pair<InterpreterTypeType, T>
      get_InterpreterTypeType_(Using const& c, Typename const& type) {
        if(isSameType(c.namespaces, c.full_name, c.name, type)) {
            std::pair<InterpreterTypeType, T> foo{InterpreterTypeType::Using, c};
            return foo;
        };

        return {InterpreterTypeType::None, {}};
    }
    template<typename T>
    static std::pair<InterpreterTypeType, T>
      get_InterpreterTypeType_(Enum const& c, Typename const& type) {
        if(isSameType(c.namespaces, c.full_name, c.name, type)) {
            return {InterpreterTypeType::Enum, c};
        };
        return {InterpreterTypeType::None, {}};
    }

    template<typename T, typename... BodyArgs>
    static std::pair<InterpreterTypeType, T>
      get_InterpreterTypeType_(Body<BodyArgs...> const& b, Typename const& type) {
        for(auto const& t : b.content) {
            if(auto ttt = std::visit(
                 [&](auto const& tt) { return get_InterpreterTypeType_<T>(tt, type); },
                 t);
               ttt.first != InterpreterTypeType::None)
            {
                return ttt;
            }
        }
        return {InterpreterTypeType::None, {}};
    }

    template<typename T>
    static std::pair<InterpreterTypeType, T>
      get_InterpreterTypeType_(Command const& c, Typename const& type) {
        if(isSameType(c.namespaces, c.full_name, c.name, type)) {
            return {InterpreterTypeType::Struct, c};
        };
        return get_InterpreterTypeType_<T>(c.body, type);
    }

    template<typename T>
    static std::pair<InterpreterTypeType, T>
      get_InterpreterTypeType_(Struct const& c, Typename const& type) {
        if(isSameType(c.namespaces, c.full_name, c.name, type)) {
            return {InterpreterTypeType::Struct, c};
        };
        return get_InterpreterTypeType_<T>(c.body, type);
    }

    template<typename T>
    static std::pair<InterpreterTypeType, T>
      get_InterpreterTypeType_(Namespace const& c, Typename const& type) {
        return get_InterpreterTypeType_<T>(c.body, type);
    }

    template<typename T, typename I>
    static std::pair<InterpreterTypeType, std::optional<T>>
      get_InterpreterTypeType_impl(I const& i, Typename const& type) {
        for(auto const& t : i.content()) {
            using T_t = std::decay_t<decltype(t)>;
            if(auto ttt = std::visit(
                 [&](auto const& tt) { return get_InterpreterTypeType_<T_t>(tt, type); },
                 t);
               ttt.first != InterpreterTypeType::None)
            {
                if(
                  ttt.first == InterpreterTypeType::Using
                  && std::holds_alternative<Using>(ttt.second)) {
                    auto tttt
                      = get_InterpreterTypeType_impl<T>(i, std::get<Using>(ttt.second).type);
                    if(tttt.first == InterpreterTypeType::Enum) {
                        return {InterpreterTypeType::Using_To_Enum, tttt.second};
                    }
                    if(tttt.first == InterpreterTypeType::Struct) {
                        return {InterpreterTypeType::Using_To_Struct, tttt.second};
                    }
                    if(tttt.first == InterpreterTypeType::None) {
                        return {InterpreterTypeType::Using, {}};
                    }

                    return tttt;
                }
                return {ttt.first, ttt.second};
            }
        }
        return {InterpreterTypeType::None, {}};
    }

    template<typename I>
    static InterpreterTypeType
      get_InterpreterTypeType(I const& i, Typename const& type) {
        return get_InterpreterTypeType_impl<typename I::ContentType>(i, type).first;
    }

    static auto
      splitname(std::string_view full_name) {
        std::pair<std::string, std::vector<std::string>> ret;
        while(true) {
            auto it = full_name.find("::");
            if(it == std::string_view::npos) {
                break;
            }
            ret.second.push_back(std::string{full_name.begin(), it});
            full_name = full_name.substr(it + 2);
        }
        ret.first = full_name;
        return ret;
    }
};

template<typename OS, typename I>
struct CppCommandPrinter {
    I const& interpreter;
    OS&      os;

    bool        genType            = false;
    bool        genOstream         = false;
    bool        genInlineOstream   = false;
    bool        genFmt             = false;
    bool        genSerialize       = false;
    bool        genInlineSerialize = false;
    bool        genCompare         = false;
    bool        genInlineCompare   = false;
    bool        genEqual           = false;
    bool        genInlineEqual     = false;
    bool        genM5              = false;
    bool        genIncludes        = false;
    bool        genInject          = false;
    bool        genCommandVariant  = false;
    std::string serializerName{};

    CppCommandPrinter(OS& os_, I const& interpreter_) : interpreter{interpreter_}, os{os_} {}

    template<typename Args>
    void
      args(Args const& a) {
        genType = a.has_prefix("--type");

        genOstream = a.has_prefix("--ostream");

        genSerialize       = a.has_prefix("--serialize");
        genInlineSerialize = a.has_prefix("--inline_serialize");

        genCompare       = a.has_prefix("--compare");
        genInlineCompare = a.has_prefix("--inline_compare");

        genEqual       = a.has_prefix("--equal");
        genInlineEqual = a.has_prefix("--inline_equal");

        genOstream       = a.has_prefix("--ostream");
        genInlineOstream = a.has_prefix("--inline_ostream");

        genType           = a.has_prefix("--type");
        genM5             = a.has_prefix("--magic");
        genIncludes       = a.has_prefix("--include");
        genInject         = a.has_prefix("--inject");
        genFmt            = a.has_prefix("--fmt");
        genSerialize      = a.has_prefix("--serialize");
        genCommandVariant = a.has_prefix("--command_variant");
        serializerName    = a.template get<std::string>("--serializer_name=", "Serializer");
    }

    void
      print() {
        os << "#pragma once\n";

        if(genIncludes) {
            os << "#include <cstdint>\n";
            os << "#include <vector>\n";
            os << "#include <array>\n";
            os << "#include <variant>\n";
            os << "#include <optional>\n";
            os << "#include <string>\n";
            os << "#include <string_view>\n";

            if(genFmt) {
                os << "#include <fmt/format.h>\n";
                os << "#include <fmt/ranges.h>\n";
            }
        }
        if(genSerialize) {
            auto const [name, namespaces] = cpp::splitname(serializerName);
            cpp::NamespacePrinter np{os, namespaces};
            os << "template<typename T>struct ";
            os << name;
            os << ";\n";
        }

        if(genType) {
            for(auto const& c : interpreter.content()) {
                std::visit([&](auto const& x) { print_type(x); }, c);
            }
        }

        for(auto const& c : interpreter.content()) {
            std::visit([&](auto const& x) { print_functions(x); }, c);
        }

        if(genCommandVariant) {
            auto all_commands = cpp::extractCommands(interpreter.content());
            auto commandsets  = cpp::extractCommandSets(interpreter.content());

            for(auto const& cs : commandsets) {
                std::vector<Command> commands;
                for(auto const& tn : cs.body.typenames()) {
                    auto it
                      = std::find_if(all_commands.begin(), all_commands.end(), [&](auto const& c) {
                            return cpp::isSameType(c.namespaces, c.full_name, c.name, tn);
                        });
                    if(it == all_commands.end()) {
                        std::string err = "Error: Command \"" + cpp::print_common(tn)
                                        + "\" requested in CommandSet \"" + cs.full_name
                                        + "\" is not a Command!\n";

                        std::cerr << err << '\n';
                        throw std::runtime_error(err);
                    }
                    commands.push_back(*it);
                }
                print_CommandSet(cs.name, cs.namespaces, commands);
            }

            if(!all_commands.empty()) {
                std::vector<std::string> commonNamespaces;
                for(std::size_t index{}; auto const& n : all_commands.front().namespaces) {
                    bool allSame = true;
                    for(auto const& c : all_commands) {
                        if(c.namespaces.size() <= index || c.namespaces[index] != n) {
                            allSame = false;
                            break;
                        }
                    }
                    if(allSame) {
                        commonNamespaces.push_back(n);
                    } else {
                        break;
                    }
                    ++index;
                }
                print_CommandSet("", commonNamespaces, all_commands);
            }
        }

        os << '\n';
    }

    void
      print_CommandSet(std::string_view name, auto const& namespaces, auto commands) {
        if(!commands.empty()) {
            auto pred = [](auto const& a, auto const& b) {
                return std::less<>{}(a.full_name, b.full_name);
            };

            auto requests  = cpp::extractStructsByName(commands, "Request");
            auto responses = cpp::extractStructsByName(commands, "Response");

            std::sort(begin(commands), end(commands), pred);
            std::sort(begin(requests), end(requests), pred);
            std::sort(begin(responses), end(responses), pred);

            cpp::NamespacePrinter np{os, namespaces};

            auto plist = [&](auto const& b) {
                if(b.empty()) {
                    return;
                }
                os << b[0].full_name;
                for(std::size_t i = 1; i < b.size(); ++i) {
                    os << ',';
                    os << b[i].full_name;
                }
            };

            std::string const name_ = name.empty() ? std::string{""} : "_" + std::string{name};

            os << "using CommandSet";
            os << name_;
            os << " = std::variant<";
            plist(commands);
            os << ">;";

            os << "using RequestSet";
            os << name_;
            os << " = std::variant<";
            plist(requests);
            os << ">;";

            os << "using ResponseSet";
            os << name_;
            os << " = std::variant<";
            plist(responses);
            os << ">;";

            os << "using RequestResponseSet";
            os << name_;
            os << " = std::variant<";
            plist(requests);
            if(!requests.empty() && !responses.empty()) {
                os << ',';
            }
            plist(responses);
            os << ">;";
        }
    }

    void
      print_ostream(Struct const& x, std::string_view f = "") {
        bool isempty = x.bases.content.empty() && x.body.attributes().empty();

        os << "template<typename Ostream>";
        os << f;
        os << "Ostream& operator<<(Ostream&os,";
        os << x.full_name;
        os << " const&";
        os << (isempty ? "" : "x");
        os << "){";
        os << "using namespace std::literals::string_view_literals;os<<\"";
        os << x.full_name;
        os << "(";

        for(bool first = true; auto const& v : x.bases.content) {
            if(!first) {
                os << "os<<\", \"sv;";
            } else {
                os << "\"sv;";
            }
            first = false;

            os << "os<<static_cast<";
            os << cpp::print_static_cast_type(v, x, interpreter);
            os << " const&>(x);";
        }

        for(bool first = x.bases.content.empty(); auto const& v : x.body.attributes()) {
            if(!first) {
                os << "os<<\", ";
            }

            first = false;
            os << v.name;
            os << "=\"sv;";

            os << "os<<x.";
            os << v.name;
            os << ';';
        }

        if(x.body.attributes().empty() && x.bases.content.empty()) {
            os << ")\"sv;";
        } else {
            os << "os<<')';";
        }

        os << "return os;}";
    }

    void
      print_enumNameArray(Enum const& x) {
        os << "static constexpr std::array<std::string_view,";
        os << x.body.content.size();
        os << "> n{";
        for(bool first = true; auto const& v : x.body.content) {
            if(!first) {
                os << ',';
            }
            first = false;
            os << '\"';
            os << v;
            os << "\"sv";
        }
        os << "};";
    }

    void
      print_ostream(Enum const& x, std::string_view f = "") {
        os << "template<typename Ostream>";
        if(!f.empty()) {
            std::string namespaceString;
            for(auto const& s : x.namespaces) {
                namespaceString += s + "::";
            }

            //is in class
            if(namespaceString != x.full_name.substr(0, x.full_name.size() - x.name.size())) {
                os << f;
            }
        }
        os << "Ostream& operator<<(Ostream&os,";
        os << x.full_name;
        os << " const&x){using namespace std::literals::string_view_literals;";

        if(x.body.content.size() != 0) {
            print_enumNameArray(x);
        }

        os << " auto const v = static_cast<";
        os << x.type();
        os << ">(x);";

        os << "os<<\"";
        os << x.full_name;
        os << "::\"sv;";

        if(x.body.content.size() != 0) {
            os << "if(v>";
            os << x.body.content.size() - 1;
            os << "){os<<\"INVALID_VALUE::\"sv;os<<(0U+v);}else{os<<n[v];}return os;}";
        } else {
            os << "os<<(0U+v);return os;}";
        }
    }

    /* void
      print_fmt_header_parse(std::string_view full_name) {}*/

    void
      print_fmt(Struct const& x) {
        bool isempty = x.bases.content.empty() && x.body.attributes().empty();

        cpp::NamespacePrinter n{os, {"fmt"}};
        os << "template<>struct formatter<";
        os << x.full_name;
        os << ">{bool fullname{};bool no_argname{};bool no_paren{};";
        os << "template<typename C>constexpr auto parse(C&c){";
        os << "using namespace std::literals::string_literals;";
        os << "if(auto const end=std::find(c.begin(),c.end(),'}');end!=c.begin()){";
        os << "auto beg=c.begin();while(beg!=end){";
        os << "if(*beg=='#'&&!fullname&&!no_paren){fullname=true;}";
        os << "else if(*beg=='!'&&!no_argname){no_argname=true;}";
        os << "else if(*beg=='('&&!no_paren&&!fullname){no_paren=true;}";
        os << "else{throw fmt::format_error(\"invalid format_spec ";
        os << R"(\""s+std::string{c.begin(),end}+"\" for )";
        os << x.full_name;
        os << "\"s);}++beg;}c.advance_to(end);}return c.begin();}";

        os << "template<typename C>auto format(";
        os << x.full_name;
        os << " const&";
        os << (isempty ? "" : "x");
        os << ",C&c){using namespace std::literals::string_view_literals;";

        os << "auto const p = !no_paren;";

        os << "auto const na{fullname?\"";
        os << x.full_name;
        os << "\"sv:\"";
        os << x.name;
        os << "\"sv};";

        auto genFmt = [&](bool print_name, auto baseArg, auto attributeArg) {
            os << "\"{}{}";

            for(bool first = true; auto const& v : x.bases.content) {
                if(!first) {
                    os << ", ";
                }
                first = false;

                os << "{:";
                os << baseArg;
                os << "(}";
            }

            for(bool first = x.bases.content.empty(); auto const& v : x.body.attributes()) {
                if(!first) {
                    os << ", ";
                }
                first = false;

                if(print_name) {
                    os << v.name;
                    os << "=";
                }

                if(auto it = cpp::get_InterpreterTypeType(interpreter, v.type);
                   it != cpp::InterpreterTypeType::None && it != cpp::InterpreterTypeType::Using)
                {
                    if(attributeArg == "") {
                        if(
                          it == cpp::InterpreterTypeType::Enum
                          || it == cpp::InterpreterTypeType::Using_To_Enum) {
                            os << "{}";
                        } else {
                            os << "({:(})";
                        }
                    } else {
                        os << "{:";
                        os << attributeArg;
                        os << '}';
                    }
                } else {
                    os << "{}";
                }
            }

            os << "{}\"sv";
        };

        os << "auto const fmt_string{no_argname?fullname?";
        genFmt(false, "!", "#!");
        os << ':';
        genFmt(false, "!", "!");
        os << ":fullname?";
        genFmt(true, "", "#");
        os << ':';
        genFmt(true, "", "");
        os << "};";

        os << "return fmt::format_to(c.out(),fmt_string,p?na:\"\"sv,p?\"(\"sv:\"\"sv";

        for(auto const& v : x.bases.content) {
            os << ",static_cast<";
            os << cpp::print_static_cast_type(v, x, interpreter);
            os << " const&>(x)";
        }

        for(auto const& v : x.body.attributes()) {
            os << ",x.";
            os << v.name;
        }
        os << ",p?\")\"sv:\"\"sv);}};";
    }

    void
      print_fmt(Enum const& x) {
        cpp::NamespacePrinter n{os, {"fmt"}};
        os << "template<>struct formatter<";
        os << x.full_name;
        os << ">{bool fullname{};bool no_name{};";
        os << "template<typename C>constexpr auto parse(C&c){";
        os << "using namespace std::literals::string_literals;";
        os << "if(auto const end=std::find(c.begin(),c.end(),'}');end!=c.begin()){";
        os << "auto beg=c.begin();bool paren{};while(beg!=end){";
        os << "if(*beg=='#'&&!fullname){fullname=true;}";
        os << "else if(*beg=='!'&&!no_name){no_name=true;}";
        os << "else if(*beg=='(' && !paren){paren=true;}";
        os << "else{throw fmt::format_error(\"invalid format_spec ";
        os << R"(\""s+std::string{c.begin(),end}+"\" for )";
        os << x.full_name;
        os << "\"s);}++beg;}c.advance_to(end);}return c.begin();}";

        os << "template<typename C>auto format(";
        os << x.full_name;
        os << " const&x,C&c){using namespace std::literals::string_view_literals;";

        if(x.body.content.size() != 0) {
            print_enumNameArray(x);
        }

        os << "auto const v = static_cast<";
        os << x.type();
        os << ">(x);";

        os << "auto const na{no_name?\"\"sv:fullname?\"";
        os << x.full_name;
        os << "::\"sv:\"";
        os << x.name;
        os << "::\"sv};";

        if(x.body.content.size() != 0) {
            os << "if(v>";
            os << x.body.content.size() - 1;
            os << "){return fmt::format_to(c.out(),\"{}INVALID_VALUE::{}\"sv,na,v);}";
            os << "else{return fmt::format_to(c.out(),\"{}{}\"sv,na,n[v]);}";
        } else {
            os << "return fmt::format_to(c.out(),\"{}{}\"sv,na,v);";
        }
        os << "}};";
    }

    void
      print_serialize(Struct const& x) {
        bool isempty = x.bases.content.empty() && x.body.attributes().empty();

        auto const [sname, snamespaces] = cpp::splitname(serializerName);
        cpp::NamespacePrinter n{os, snamespaces};
        os << "template<>struct ";
        os << sname;
        os << "<";
        os << x.full_name;
        os << ">{";

        auto gen = [&](auto sn, auto n, auto co) {
            os << "template<typename Buffer>";

            os << "bool ";
            os << n;
            os << "serialize(Buffer& buffer,";
            os << x.full_name;
            os << ' ';
            os << co;
            os << "&";
            os << (isempty ? "" : "x");
            os << ") {";
            os << "return ";
            os << sn;
            os << "::";
            os << n;
            os << "serialize(";
            os << "buffer";
            for(auto const& b : x.bases.content) {
                os << ", ";

                os << "static_cast<";
                os << cpp::print_static_cast_type(b, x, interpreter);
                os << ' ';
                os << co;
                os << "&>(x)";
            }

            for(auto const& v : x.body.attributes()) {
                os << ", x.";
                os << v.name;
                ;
            }
            os << ");}";
        };

        gen(sname, "", "const");
        gen(sname, "de", "");

        os << "};";
    }

    void
      print_inlineSerialize(Struct const& x) {
        bool isempty = x.bases.content.empty() && x.body.attributes().empty();
        auto gen     = [&](auto n, auto co) {
            os << "template<typename Serializer,typename Buffer>";

            os << "static bool ";
            os << n;
            os << "serialize(Buffer& buffer,";
            os << x.name;
            os << ' ';
            os << co;
            os << "&";
            os << (isempty ? "" : "x");
            os << ") {";
            os << "return Serializer::";
            os << n;
            os << "serialize(";
            os << "buffer";
            for(auto const& b : x.bases.content) {
                os << ", ";

                os << "static_cast<";
                os << cpp::print_static_cast_type(b, x, interpreter);
                os << ' ';
                os << co;
                os << "&>(x)";
            }

            for(auto const& v : x.body.attributes()) {
                os << ", x.";
                os << v.name;
                ;
            }
            os << ");}";
        };

        gen("", "const");
        gen("de", "");
    }

    void
      print_operatorHeader(Struct const& x, std::string_view op, std::string_view f, bool isempty) {
        os << f;
        os << "bool operator";
        os << op;
        os << '(';
        os << x.full_name;
        os << " const&";
        os << (isempty ? "" : "a");
        os << ",";
        os << x.full_name;
        os << " const&";
        os << (isempty ? "" : "b");
        os << "){";
    }

    void
      print_equal(Struct const& x, std::string_view f = "") {
        bool isempty = x.bases.content.empty() && x.body.attributes().empty();
        print_operatorHeader(x, "==", f, isempty);

        os << "return ";

        for(bool first = true; auto const& b : x.bases.content) {
            if(!first) {
                os << "&&";
            }
            first = false;
            os << "static_cast<";
            os << cpp::print_static_cast_type(b, x, interpreter);
            os << " const&>(a)";
            os << "==";
            os << "static_cast<";
            os << cpp::print_static_cast_type(b, x, interpreter);
            os << " const&>(b)";
        }

        for(bool first = x.bases.content.empty(); auto const& a : x.body.attributes()) {
            if(!first) {
                os << "&&";
            }
            first = false;
            os << "a.";
            os << a.name;
            os << "==b.";
            os << a.name;
        }

        if(x.bases.content.empty() && x.body.attributes().empty()) {
            os << "true";
        }
        os << ";}";

        print_operatorHeader(x, "!=", f, false);
        os << "return !(a == b);}";
    }

    void
      print_compare(Struct const& x, std::string_view f = "") {
        bool isempty = x.bases.content.empty() && x.body.attributes().empty();
        print_operatorHeader(x, "<", f, isempty);
        for(auto const& b : x.bases.content) {
            os << "if(";
            os << "static_cast<";
            os << cpp::print_static_cast_type(b, x, interpreter);
            os << " const&>(a)";
            os << "<";
            os << "static_cast<";
            os << cpp::print_static_cast_type(b, x, interpreter);
            os << " const&>(b)";
            os << "){return true;}";
        }
        for(auto const& a : x.body.attributes()) {
            os << "if(a.";
            os << a.name;
            os << "<b.";
            os << a.name;
            os << "){return true;}";
        }
        os << "return false;}";

        print_operatorHeader(x, ">", f, false);
        os << "return b<a;}";

        print_operatorHeader(x, "<=", f, false);
        os << "return !(b<a);}";

        print_operatorHeader(x, ">=", f, false);
        os << "return !(a<b);}";
    }

    void
      print_M5(Struct const& x) {
        os << '~';
        os << x.name;
        os << "()=default;";

        os << x.name;
        os << "()=default;";

        os << x.name;
        os << '(';
        os << x.name;
        os << " const&)=default;";

        os << x.name;
        os << '(';
        os << x.name;
        os << "&&)=default;";

        os << x.name;
        os << "&operator=(";
        os << x.name;
        os << " const&)=default;";

        os << x.name;
        os << "&operator=(";
        os << x.name;
        os << "&&)=default;";
    }

    void
      print_type(Using const& x) {
        os << "using ";
        os << x.name;
        os << '=';
        os << cpp::print_common(x.type);
        os << ';';
    }

    void
      print_type(Command const& x) {
        os << "struct ";
        os << x.name;
        os << '{';
        os << "static constexpr bool isCommand=true;";

        for(auto const& v : x.body.content) {
            std::visit(
              [&](auto const& vv) {
                  if constexpr(std::is_same_v<std::decay_t<decltype(vv)>, Struct>) {
                      auto makeInject = [&](std::string const& name) {
                          std::string ret;
                          ret = "using Command=" + x.name + ";";
                          ret += "static constexpr bool is" + name + "=true;";
                          return ret;
                      };

                      if(vv.name == "Request") {
                          print_type(vv, makeInject("Request"));
                      } else if(vv.name == "Response") {
                          print_type(vv, makeInject("Response"));
                      } else {
                          print_type(vv);
                      }
                  } else {
                      print_type(vv);
                  }
              },
              v);
        }

        os << "};";
    }

    void
      print_type(CommandSet const& x) {}

    void
      print_type(Attribute const& x) {
        os << cpp::print_common(x.type);
        os << ' ';
        os << x.name;
        os << "{};";
    }

    template<typename... Ts>
    void
      print_type(Body<Ts...> const& x) {
        for(auto const& v : x.content) {
            std::visit([this](auto const& vv) { print_type(vv); }, v);
        }
    }

    void
      print_type(Namespace const& x) {
        os << "namespace ";
        os << x.name;
        os << '{';
        print_type(x.body);
        os << '}';
    }

    void
      print_type(BaseClasses const& x) {
        for(bool first = true; auto const& v : x.content) {
            if(first) {
                os << ": ";
            } else {
                os << ',';
            }
            first = false;
            os << cpp::print_common(v.content);
        }
    }

    void
      print_type(Struct const& x, std::string_view inject = "") {
        os << "struct ";
        os << x.name;
        print_type(x.bases);
        os << '{';
        if(!inject.empty()) {
            os << inject;
        }
        print_type(x.body);

        if(genM5) {
            print_M5(x);
        }
        if(genInlineSerialize) {
            print_inlineSerialize(x);
        }
        if(genInlineOstream) {
            print_ostream(x, "friend ");
        }
        if(genInlineCompare) {
            print_compare(x, "friend ");
        }
        if(genInlineEqual) {
            print_equal(x, "friend ");
        }
        os << "};";
    }

    void
      print_type(Enum const& x) {
        os << "enum class ";
        os << x.name;
        os << ':';
        os << x.type();
        os << '{';
        for(bool first = true; auto& v : x.body.content) {
            if(!first) {
                os << ',';
            }
            first = false;
            os << v;
        }
        os << "};";

        if(genInlineOstream) {
            print_ostream(x, "friend ");
        }
    }

    void
      print_type(Injection const& x) {
        if(genInject) {
            for(auto const& l : x.body.lines) {
                os << l;
            }
        }
    }

    void
      print_functions(CommandSet const& x) {}
    void
      print_functions(Injection const& x) {}
    void
      print_functions(Using const& x) {}
    void
      print_functions(BaseClasses const& x) {}
    void
      print_functions(Attribute const& x) {}

    void
      print_functions(Struct const& x) {
        print_functions(x.body);

        if(genSerialize) {
            print_serialize(x);
        }
        if(genOstream) {
            cpp::NamespacePrinter n{os, x.namespaces};
            print_ostream(x);
        }
        if(genFmt) {
            print_fmt(x);
        }
        if(genEqual) {
            cpp::NamespacePrinter n{os, x.namespaces};
            print_equal(x);
        }
        if(genCompare) {
            cpp::NamespacePrinter n{os, x.namespaces};
            print_compare(x);
        }
    }

    void
      print_functions(Command const& x) {
        print_functions(x.body);
    }

    void
      print_functions(Enum const& x) {
        if(genOstream) {
            cpp::NamespacePrinter n{os, x.namespaces};
            print_ostream(x);
        }
        if(genFmt) {
            print_fmt(x);
        }
    }

    template<typename... Ts>
    void
      print_functions(Body<Ts...> const& x) {
        for(auto const& v : x.content) {
            std::visit([this](auto const& vv) { print_functions(vv); }, v);
        }
    }

    void
      print_functions(Namespace const& x) {
        print_functions(x.body);
    }
};

template<typename OS, typename I>
struct CppEnvironmentPrinter {
    I const& interpreter;
    OS&      os;

    std::string class_name;

    CppEnvironmentPrinter(OS& os_, I const& interpreter_) : interpreter{interpreter_}, os{os_} {}

    template<typename Args>
    void
      args(Args const& a) {
        class_name = a.template get<std::string>("--class_name=", "foo");
    }

    void
      print() {
        auto commands = cpp::extractCommands(interpreter.content());

        std::sort(begin(commands), end(commands), [](auto const& a, auto const& b) {
            return std::less<>{}(a.full_name, b.full_name);
        });

        os << "#pragma once\n";
        os << "struct ";
        os << class_name;
        os << " {using CommandSet = make_command_set<";
        if(!commands.empty()) {
            os << commands[0].full_name;
            for(std::size_t i = 1; i < commands.size(); ++i) {
                os << ',';
                os << commands[i].full_name;
            }
        }

        os << ">;\n\n";

        for(auto const& c : commands) {
            os << c.full_name;
            os << "::Response handle(";
            os << c.full_name;
            os << "::Request const& request) {\n";

            for(auto const& s : c.body.structs()) {
                if(s.name == "Request") {
                    for(auto const& a : s.body.attributes()) {
                        os << "/* request.";
                        os << a.name;
                        os << " : ";
                        os << cpp::print_common(a.type);
                        os << " */\n";
                    }
                    break;
                }
            }

            for(auto const& s : c.body.structs()) {
                if(s.name == "Response") {
                    for(auto const& a : s.body.attributes()) {
                        os << "/* Response::";
                        os << a.name;
                        os << " : ";
                        os << cpp::print_common(a.type);
                        os << " */\n";
                    }
                    break;
                }
            }
            os << "return {};}";
        }
        os << "};\n";
    }
};

}   // namespace worse_io
