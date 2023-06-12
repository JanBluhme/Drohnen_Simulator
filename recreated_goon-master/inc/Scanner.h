#pragma once


/**
 * Generated Flex class name is yyFlexLexer by default. If we want to use more flex-generated
 * classes we should name them differently. See scanner.l prefix option.
 * 
 * Unfortunately the implementation relies on this trick with redefining class name
 * with a preprocessor macro. See GNU Flex manual, "Generating C++ Scanners" section
 */
#if ! defined(yyFlexLexerOnce)
#undef yyFlexLexer
#define yyFlexLexer worse_io_FlexLexer // the trick with prefix; no namespace here :(
#include <FlexLexer.h>
#endif

// Scanner method signature is defined by this macro. Original yylex() returns int.
// Sinice Bison 3 uses symbol_type, we must change returned type. We also rename it
// to something sane, since you cannot overload return type.
#undef YY_DECL
#define YY_DECL template<> worse_io::Parser::symbol_type worse_io::Scanner<worse_io::ContentType>::get_next_token()

#include "Parser.hpp" // this is needed for symbol_type

namespace worse_io {

// Forward declare interpreter to avoid include. Header is added inimplementation file.
template<typename ContentType>
class Interpreter; 

template<typename ContentType>
class Scanner
	: public yyFlexLexer
{
	Interpreter<ContentType>& m_driver;
public:
	Scanner(Interpreter<ContentType>& driver) : m_driver(driver) {}
	virtual ~Scanner() {}
	virtual worse_io::Parser::symbol_type get_next_token();
};

}
