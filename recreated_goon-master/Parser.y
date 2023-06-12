%skeleton "lalr1.cc" /* -*- C++ -*- */
%require "3.0"
%defines
%define api.parser.class { Parser }

%define api.token.constructor
%define api.value.type variant
%define parse.assert
%define api.namespace { worse_io }
%code requires
{
    #include <iostream>
    #include <string>
    #include <vector>
    #include <stdint.h>
    #include "command.h"

    namespace worse_io {
        using ContentType = std::variant<Using, BaseClasses, Namespace, Command, CommandSet, Struct, Enum, Injection>;
        template<typename T> class Scanner;
        template<typename T> class Interpreter;
    }
}

// Bison calls yylex() function that must be provided by us to suck tokens
// from the scanner. This block will be placed at the beginning of IMPLEMENTATION file (cpp).
// We define this function here (function! not method).
// This function is called only inside Bison, so we make it static to limit symbol visibility for the linker
// to avoid potential linking conflicts.
%code top
{
    #include <iostream>
    #include "Scanner.h"
    #include "Parser.hpp"
    #include "Interpreter.h"
    #include "location.hh"

    // yylex() arguments are defined in parser.y
    static worse_io::Parser::symbol_type yylex(worse_io::Scanner<worse_io::ContentType>& scanner, worse_io::Interpreter<worse_io::ContentType>& driver) {
        return scanner.get_next_token();
    }

    template<typename T, typename U>
    void append(T& has_content, U const& v) {
        has_content.content.push_back(v);
    }
}

%lex-param {   worse_io::Scanner<worse_io::ContentType>&     scanner }
%lex-param {   worse_io::Interpreter<ContentType>&           driver  }
%parse-param { worse_io::Scanner<worse_io::ContentType>&     scanner }
%parse-param { worse_io::Interpreter<ContentType>&           driver  }
%locations
%define parse.trace
%define parse.error verbose

%define api.token.prefix {TOKEN_}

%token END 0                "end of file"
%token <std::string> STRING "token_string";
%token <uint64_t> NUMBER    "token_number";
%token LEFTPAR              "(";
%token RIGHTPAR             ")";
%token SEMICOLON            ";";
%token COLON                ":";
%token LEFTCURL             "{"
%token RIGHTCURL            "}"
%token LESS                 "<"
%token GREATER              ">"
%token COMMA                ",";
%token EQUALS               "=";
%token COMMAND              "Command";
%token COMMANDSET           "CommandSet";
%token USING                "using";
%token NAMESPACE            "namespace";
%token STRUCT               "struct";
%token SCOPE                "::";
%token ENUM                 "enum";
%token REQUEST              "Request";
%token RESPONSE             "Response";
%token INJECT_BEGIN         "Begin of Injection";
%token INJECT_END           "End of Injection";
%left SCOPE

%type< worse_io::TemplateArgumentItem > TemplateArgumentItem;
%type< worse_io::TemplateArgumentList > TemplateArgumentList;
%type< worse_io::UnscopedTypename     > UnscopedTypename;
%type< worse_io::Typename             > Typename;
%type< worse_io::Attribute            > Attribute;
%type< worse_io::Using                > Using;
%type< worse_io::Enum_Body            > Enum_Body;
%type< worse_io::Enum                 > Enum;
%type< worse_io::Struct_Body          > Struct_Body;
%type< worse_io::Struct               > Struct;
%type< worse_io::BaseClasses          > BaseClasses;
%type< worse_io::Request              > Request;
%type< worse_io::Response             > Response;
%type< worse_io::Command_Body         > Command_Body;
%type< worse_io::Command              > Command;
%type< worse_io::CommandSet           > CommandSet;
%type< worse_io::CommandSet_Body      > CommandSet_Body;
%type< worse_io::Namespace            > Namespace;
%type< worse_io::Namespace_Body       > Namespace_Body;
%type< worse_io::Injection_Body       > Injection_Body;
%type< worse_io::Injection            > Injection;

%start Protocol

%%
Protocol
	: Command            { driver.addContent($1); }
	| CommandSet         { driver.addContent($1); }
	| Struct             { driver.addContent($1); }
	| Enum               { driver.addContent($1); }
	| Injection          { driver.addContent($1); }
	| Namespace          { driver.addContent($1); }
	| Using              { driver.addContent($1); }
	| Protocol Command   { driver.addContent($2); }
	| Protocol CommandSet{ driver.addContent($2); }
	| Protocol Struct    { driver.addContent($2); }
	| Protocol Enum      { driver.addContent($2); }
	| Protocol Injection { driver.addContent($2); }
	| Protocol Namespace { driver.addContent($2); }
	| Protocol Using     { driver.addContent($2); }
	;
Namespace
	: NAMESPACE STRING LEFTCURL Namespace_Body RIGHTCURL { $$ = {$2, $4}; }
	| NAMESPACE STRING LEFTCURL                RIGHTCURL { $$ = {$2, {}}; }
	;
Namespace_Body
	: Command            { append($$ = {}, $1); }
	| CommandSet         { append($$ = {}, $1); }
	| Struct             { append($$ = {}, $1); }
	| Enum               { append($$ = {}, $1); }
	| Injection          { append($$ = {}, $1); }
	| Namespace          { append($$ = {}, $1); }
	| Using              { append($$ = {}, $1); }
	| Namespace_Body Command   { append($$ = $1, $2); }
	| Namespace_Body CommandSet{ append($$ = $1, $2); }
	| Namespace_Body Struct    { append($$ = $1, $2); }
	| Namespace_Body Enum      { append($$ = $1, $2); }
	| Namespace_Body Injection { append($$ = $1, $2); }
	| Namespace_Body Namespace { append($$ = $1, $2); }
	| Namespace_Body Using     { append($$ = $1, $2); }
	;
Command
	: COMMAND STRING                   LEFTCURL Command_Body RIGHTCURL SEMICOLON {
		$4.full_check<worse_io::Parser::syntax_error>(driver.requests_without_response_allowed(),
                                                      $2,
                                                      driver.location());
		$$ = {$2, $4};
	}
	| COMMAND STRING COLON BaseClasses LEFTCURL Command_Body RIGHTCURL SEMICOLON {
		$6.full_check<worse_io::Parser::syntax_error>(driver.requests_without_response_allowed(),
                                                      $2,
                                                      driver.location());
		$$ = {$2, $6, $4};
	}
	;

CommandSet
    : COMMANDSET STRING LEFTCURL CommandSet_Body RIGHTCURL SEMICOLON{ $$ = {$2, $4}; }
    | COMMANDSET STRING LEFTCURL                 RIGHTCURL SEMICOLON{ $$ = {$2, {}}; }

CommandSet_Body
	: Typename SEMICOLON                 { append($$ = {}, $1); }
	| CommandSet_Body Typename SEMICOLON { append($$ = $1, $2); }

Struct_Body
	: Struct                { append($$ = {}, $1); }
	| Enum                  { append($$ = {}, $1); }
	| Attribute             { append($$ = {}, $1); }
	| Injection             { append($$ = {}, $1); }
	| Using                 { append($$ = {}, $1); }
	| Struct_Body Struct    { append($$ = $1, $2); }
	| Struct_Body Enum      { append($$ = $1, $2); }
	| Struct_Body Attribute { append($$ = $1, $2); }
	| Struct_Body Injection { append($$ = $1, $2); }
	| Struct_Body Using     { append($$ = $1, $2); }
	;
Command_Body
	: Request                { append($$ = {}, $1); }
	| Response               { append($$ = {}, $1); }
	| Struct                 { append($$ = {}, $1); }
	| Enum                   { append($$ = {}, $1); }
	| Injection              { append($$ = {}, $1); }
	| Using                  { append($$ = {}, $1); }
	| Command_Body Request   { append($$ = $1, $2); $$.partial_check<worse_io::Parser::syntax_error>(driver.location());}
	| Command_Body Response  { append($$ = $1, $2); $$.partial_check<worse_io::Parser::syntax_error>(driver.location());}
	| Command_Body Struct    { append($$ = $1, $2); }
	| Command_Body Enum      { append($$ = $1, $2); }
	| Command_Body Injection { append($$ = $1, $2); }
	| Command_Body Using     { append($$ = $1, $2); }
	;
Request
	: REQUEST                   LEFTCURL Struct_Body RIGHTCURL SEMICOLON { $$ = {$3}; }
	| REQUEST                   LEFTCURL             RIGHTCURL SEMICOLON { $$ = {  }; }
	| REQUEST COLON BaseClasses LEFTCURL             RIGHTCURL SEMICOLON { $$ = {{}, $3}; }
	| REQUEST COLON BaseClasses LEFTCURL Struct_Body RIGHTCURL SEMICOLON { $$ = {$5, $3}; }
	;
Response
	: RESPONSE                   LEFTCURL Struct_Body RIGHTCURL SEMICOLON { $$ = {$3}; }
	| RESPONSE                   LEFTCURL             RIGHTCURL SEMICOLON { $$ = {  }; }
	| RESPONSE COLON BaseClasses LEFTCURL             RIGHTCURL SEMICOLON { $$ = {{}, $3}; }
	| RESPONSE COLON BaseClasses LEFTCURL Struct_Body RIGHTCURL SEMICOLON { $$ = {$5, $3}; }
	;
Struct
	: STRUCT STRING                   LEFTCURL Struct_Body RIGHTCURL SEMICOLON { $$ = {$2, $4}; }
	| STRUCT STRING                   LEFTCURL             RIGHTCURL SEMICOLON { $$ = {$2, {}}; }
	| STRUCT STRING COLON BaseClasses LEFTCURL             RIGHTCURL SEMICOLON { $$ = {$2, {}, $4}; }
	| STRUCT STRING COLON BaseClasses LEFTCURL Struct_Body RIGHTCURL SEMICOLON { $$ = {$2, $6, $4}; }
	;
BaseClasses
	: Typename                   { append($$ = {}, $1); }
	| BaseClasses COMMA Typename { append($$ = $1, $3); }
	;
Enum
	: ENUM STRING LEFTCURL Enum_Body RIGHTCURL SEMICOLON { $$ = {$2, $4}; }
	| ENUM STRING LEFTCURL           RIGHTCURL SEMICOLON { $$ = {$2, {}}; }
	;
Enum_Body
	: STRING                 { append($$ = {}, $1); }
	| Enum_Body COMMA STRING { append($$ = $1, $3); }
	;
Attribute
	: Typename STRING SEMICOLON { $$ = {$1, $2}; }
	;
Using
	: USING STRING EQUALS Typename SEMICOLON { $$ = {$4, $2}; }
	;
TemplateArgumentItem
	: Typename { $$ = {$1}; }
	| NUMBER   { $$ = {$1}; }
	;
TemplateArgumentList
	: TemplateArgumentItem                            { append($$ = {}, $1); }
	| TemplateArgumentList COMMA TemplateArgumentItem { append($$ = $1, $3); }
	;
UnscopedTypename
	: STRING                                   { $$ = {{$1}, {}}; }
	| STRING LESS TemplateArgumentList GREATER { $$ = {{$1}, $3}; }
	;
Typename
	: UnscopedTypename                { $$ = {{$1}};        }
	| SCOPE UnscopedTypename          { $$ = {{"::"}, $2};  }
	| Typename SCOPE UnscopedTypename { $$ = $1.append($3); }
	;
Injection_Body
	: STRING                { $$ = {}; $$.lines.push_back($1); }
	| Injection_Body STRING { $1.lines.push_back($2); $$ = $1; }
	;
Injection
	: INJECT_BEGIN Injection_Body INJECT_END { $$ = {}; $$.body = $2; }
	;
%%

// Bison expects us to provide implementation - otherwise linker complains
void worse_io::Parser::error(const location &loc , const std::string &message) {
    std::cerr << "Error: " << message << std::endl << "Error location: " << loc << '\n';
    std::cerr << driver.current_line();
    for(std::size_t i = 0; i < loc.begin.column - 1; ++i) {
        if(driver.current_line()[i] == '\t' ) {
            std::cerr << '\t';
        } else {
            std::cerr << ' ';
        }
    }
    for(std::size_t i = loc.begin.column; i != loc.end.column; ++i) {
        std::cerr << '*';
    }
    std::cerr << '\n';
}
