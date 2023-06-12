#pragma once

#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <sstream>
#include <limits>
#include <cassert>
#include <cstdint>

#include <iostream>

namespace worse_io {

struct Injection_Body {
	std::vector<std::string> lines;
};

struct Injection {
	Injection_Body body;
};

template<typename... Ts>
struct ListItem {
	std::variant<Ts...> content;
};

template<typename... Ts>
struct List {
	using item_t = ListItem<Ts...>;
	std::vector<item_t> content;
};

struct Typename;
using TemplateArgumentList = List<Typename, unsigned long int>;
using TemplateArgumentItem = TemplateArgumentList::item_t;

struct UnscopedTypename {
	std::string name;
	std::optional<TemplateArgumentList> list;
};

struct Typename {
	std::vector<UnscopedTypename> scoped_name;
	
	Typename() = default;
	Typename(UnscopedTypename const& x)
		: scoped_name{x}
	{}
	Typename(UnscopedTypename const& x, UnscopedTypename const& y)
		: scoped_name{x, y}
	{}
	
	Typename& append(UnscopedTypename const& x) {
		scoped_name.push_back(UnscopedTypename{"::"});
		scoped_name.push_back(x);
		return *this;
	}
};

struct Attribute {
	Typename    type;
	std::string name;
};

struct Using {
	Typename    type;
	std::string name;
	std::string full_name;
    std::vector<std::string> namespaces;

	Using() = default;
	Using(Typename const& type_, std::string const& name_)
		: name(name_)
		, full_name(name_)
		, type(type_)
	{}
};

struct BaseClass{
    BaseClass(Typename const& t) :content{t}{}
    Typename content;
};

struct BaseClasses {
    std::vector<BaseClass> content;
};

struct Enum_Body {
	std::vector<std::string> content;
};

struct Enum {
	std::string name;
	std::string full_name;
	Enum_Body   body;
    std::vector<std::string> namespaces;
	
	Enum() = default;
	Enum(std::string const& name, Enum_Body const& body)
		: name(name)
		, full_name(name)
		, body(body)
	{}
	std::string type() const {
		if(body.content.size() <= std::numeric_limits<uint8_t>::max()) {
			return "std::uint8_t";
		}
		if(body.content.size() <= std::numeric_limits<uint16_t>::max()) {
			return "std::uint16_t";
		}
		if(body.content.size() <= std::numeric_limits<uint32_t>::max()) {
			return "std::uint32_t";
		}
		return "std::uint64_t";
	}
};

template<typename... Ts>
struct Body {
	std::vector<std::variant<Ts...>> content;
	
	template<typename body_t, typename value_t>
	struct iterator {
		body_t* parent;
		using selector_t = std::decay_t<value_t>;
		using iterator_t = decltype(parent->content.begin());
		iterator_t it;
		
		iterator(body_t* parent, iterator_t const& it)
			: parent(parent)
			, it(it)
		{
			if(it != parent->content.end() && !std::holds_alternative<selector_t>(*it)) {
				this->operator++();
			}
		}
		value_t& operator*() const {
			return std::get<selector_t>(*it);
		}
		value_t* operator->() const {
			return &(this->operator*());
		}
		iterator& operator++() {
			while(it != parent->content.end()) {
				++it;
				 if(std::holds_alternative<selector_t>(*it)) {
					 break;
				 }
			}
			return *this;
		}
		friend
		bool operator==(iterator const& a, iterator const& b) {
			return a.it == b.it;
		}
		friend
		bool operator!=(iterator const& a, iterator const& b) {
			return !(a == b);
		}
		operator iterator<body_t const, value_t const>() const {
			return {parent, it};
		}
	};
	
	template<typename body_t, typename value_t>
	struct filter {
		body_t* parent;
		using iterator = Body::iterator<body_t, value_t>;
		
		iterator begin() const {
			return iterator{parent, parent->content.begin()};
		}
		iterator end() const {
			return iterator{parent, parent->content.end()};
		}
		std::size_t size() const {
			iterator begin = this->begin();
			iterator end   = this->end();
			std::size_t result = 0;
			while(begin != end) {
				++begin;
				++result;
			}
			return result;
		}
		bool empty() const {
			return begin() == end();
		}
	};
};

template<typename Body>
struct Struct_base {
	std::string name;
	std::string full_name;
    BaseClasses bases;
	Body body;
    std::vector<std::string> namespaces;

    static constexpr bool isNamespace(){
        if constexpr(requires {Body::isNamespace;}){
            if constexpr(Body::isNamespace){
                return true;
            }
        }
        return false;
    }

	Struct_base() = default;
	Struct_base(std::string const& name, Body const& body, BaseClasses const& bases)
		: name(name)
		, full_name(name)
		, body(body)
        , bases(bases)
	{
		prefix_full_names(name + "::");
        if constexpr(isNamespace()){
            add_namespace(name);
        }
	}

	Struct_base(std::string const& name, Body const& body)
		: Struct_base(name,body,{})
	{
	}

    void add_namespace(std::string const& name){
        if constexpr (requires { body.usings(); }) {
            for(auto& c : body.usings()) {
                c.namespaces.insert(c.namespaces.begin(),name);
            }
        }
        if constexpr (requires { body.enums(); }) {
            for(auto& c : body.enums()) {
                c.namespaces.insert(c.namespaces.begin(),name);
            }
        }
		if constexpr (requires { body.structs(); }) {
            for(auto& c : body.structs()) {
                c.namespaces.insert(c.namespaces.begin(),name);
                c.add_namespace(name);
            }
        }
		if constexpr (requires { body.namespaces(); }) {
            for(auto& c : body.namespaces()) {
                c.namespaces.insert(c.namespaces.begin(),name);
                c.add_namespace(name);
            }
        }
        if constexpr (requires { body.commands(); }) {
            for(auto& c : body.commands()) {
                c.namespaces.insert(c.namespaces.begin(),name);
                c.add_namespace(name);
            }
        }
        if constexpr (requires { body.commandsets(); }) {
            for(auto& c : body.commandsets()) {
                c.namespaces.insert(c.namespaces.begin(),name);
                c.add_namespace(name);
            }
        }
    }

	void prefix_full_names(std::string const& prefix) {
        if constexpr (requires { body.usings(); }) {
            for(auto& c : body.usings()) {
                c.full_name = prefix + c.full_name;
            }
        }
        if constexpr (requires { body.enums(); }) {
            for(auto& c : body.enums()) {
                c.full_name = prefix + c.full_name;
            }
        }
		if constexpr (requires { body.structs(); }) {
            for(auto& c : body.structs()) {
                c.full_name = prefix + c.full_name;
                c.prefix_full_names(prefix);
            }
        }
		if constexpr (requires { body.namespaces(); }) {
            for(auto& c : body.namespaces()) {
                c.full_name = prefix + c.full_name;
                c.prefix_full_names(prefix);
            }
        }
        if constexpr (requires { body.commands(); }) {
            for(auto& c : body.commands()) {
                c.full_name = prefix + c.full_name;
                c.prefix_full_names(prefix);
            }
        }
        if constexpr (requires { body.commandsets(); }) {
            for(auto& c : body.commandsets()) {
                c.full_name = prefix + c.full_name;
                c.prefix_full_names(prefix);
            }
        }
	}
};

struct Struct;
struct Struct_Body
	: Body<Struct, Enum, Using, Attribute, Injection>
{
	filter<Struct_Body const, Struct const   > structs() const    { return {this}; }
	filter<Struct_Body,       Struct         > structs()          { return {this}; }
	filter<Struct_Body const, Enum const     > enums() const      { return {this}; }
	filter<Struct_Body,       Enum           > enums()            { return {this}; }
	filter<Struct_Body const, Attribute const> attributes() const { return {this}; }
	filter<Struct_Body,       Attribute      > attributes()       { return {this}; }
	filter<Struct_Body const, Injection const> injections() const { return {this}; }
	filter<Struct_Body,       Injection      > injections()       { return {this}; }
    filter<Struct_Body const, Using const    > usings() const     { return {this}; }
	filter<Struct_Body,       Using          > usings()           { return {this}; }
};

struct Struct
	: Struct_base<Struct_Body>
{
	using Struct_base<Struct_Body>::Struct_base;
};

struct Command_Body
	: Body<Struct, Enum, Using, Injection>
{
	filter<Command_Body const, Struct const   > structs() const    { return {this}; }
	filter<Command_Body,       Struct         > structs()          { return {this}; }
	filter<Command_Body const, Enum const     > enums() const      { return {this}; }
	filter<Command_Body,       Enum           > enums()            { return {this}; }
	filter<Command_Body const, Injection const> injections() const { return {this}; }
	filter<Command_Body,       Injection      > injections()       { return {this}; }
    filter<Command_Body const, Using const    > usings() const     { return {this}; }
	filter<Command_Body,       Using          > usings()           { return {this}; }
	
	template<typename E, typename... E_args>
	void check(bool no_responce_ok,
               bool full_check,
               std::string const& command_name,
               E_args const&... as) const {
		auto count_type = [this](std::string const& name) {
			std::size_t cnt = 0;
			for(auto const& c : this->structs()) {
				if(c.name == name) {
					++cnt;
				}
			}
			return cnt;
		};
		auto c = [&](std::string const& what,std::size_t min, std::size_t max) {
			std::size_t n = count_type(what);
			if(    ( full_check && n > max)
				|| ( full_check && n < min)
				|| (!full_check && n >  max)
			) {
				std::string msg = "Number of " + what + " in Command ";
				if(!command_name.empty()) {
					msg += "\'" + command_name + "\' ";
				}
				msg += "is "
                    +  std::to_string(n)
                    +  ", "
                    +  std::to_string(min)
                    +  "-"
                    +  std::to_string(max)
                    +  " expected.";
				throw E(as..., msg);
			}
		};
		c("Request",1,1);
		c("Response",no_responce_ok?0:1,1);
	}
	
	template<typename E, typename... E_args>
	void full_check(bool no_responce_ok, std::string const& command_name, E_args const&... as) const {
		check<E>(no_responce_ok, true, command_name, as...);
	}
	template<typename E, typename... E_args>
	void partial_check(E_args const&... as) const {
		check<E>(true, false, "", as...);
	}
};



struct Request
	: Struct
{
	Request(Struct_Body const& body, BaseClasses const& bases) : Struct{"Request",body,bases} {}
	Request(Struct_Body const& body) : Request{body,{}} {}
	Request() : Request{{},{}} {}
};

struct Response
	: Struct
{
	Response(Struct_Body const& body, BaseClasses const& bases) : Struct{"Response",body,bases} {}
	Response(Struct_Body const& body) : Response{body,{}} {}
	Response() : Response{{},{}} {}
};

struct Command
	: Struct_base<Command_Body>
{
	Command()
		: Struct_base<Command_Body>{}
	{}
	Command(std::string const& name, Command_Body const& body, BaseClasses const& bases)
		: Struct_base<Command_Body>{name, body, bases}
	{}
	Command(std::string const& name, Command_Body const& body)
		: Command{name, body, {}}
	{}
};

struct CommandSet_Body
	: Body<Typename>
{
    filter<CommandSet_Body const, Typename const    > typenames() const     { return {this}; }
	filter<CommandSet_Body,       Typename          > typenames()           { return {this}; }
};

struct CommandSet
    : Struct_base<CommandSet_Body>
{
	CommandSet()
        : Struct_base<CommandSet_Body>{}
    {}
	CommandSet(std::string const& name, CommandSet_Body const& body)
		: Struct_base<CommandSet_Body>{name, body}
	{}
};

struct Namespace;

struct Namespace_Body : Body<Namespace, Struct, Enum, Using, Injection, Command, CommandSet> {
    static constexpr bool isNamespace = true;
    filter<Namespace_Body const, Struct const     > structs() const    { return {this}; }
	filter<Namespace_Body,       Struct           > structs()          { return {this}; }
	filter<Namespace_Body const, Enum const       > enums() const      { return {this}; }
	filter<Namespace_Body,       Enum             > enums()            { return {this}; }
	filter<Namespace_Body const, Injection const  > injections() const { return {this}; }
	filter<Namespace_Body,       Injection        > injections()       { return {this}; }
	filter<Namespace_Body const, Namespace const  > namespaces() const { return {this}; }
	filter<Namespace_Body,       Namespace        > namespaces()       { return {this}; }
	filter<Namespace_Body const, Command const    > commands() const   { return {this}; }
	filter<Namespace_Body,       Command          > commands()         { return {this}; }
	filter<Namespace_Body const, CommandSet const > commandsets() const{ return {this}; }
	filter<Namespace_Body,       CommandSet       > commandsets()      { return {this}; }
    filter<Namespace_Body const, Using const      > usings() const     { return {this}; }
	filter<Namespace_Body,       Using            > usings()           { return {this}; }
};

struct Namespace
	: Struct_base<Namespace_Body>
{
	using Struct_base<Namespace_Body>::Struct_base;
};

}
