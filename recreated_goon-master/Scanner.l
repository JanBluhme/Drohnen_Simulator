%{
    #include <iostream>
    #include <cstdlib>
    #include "Scanner.h"
    #include "Interpreter.h"
    #include "Parser.hpp"
    #include "location.hh"

    // Original yyterminate() macro returns int. Since we're using Bison 3 variants
    // as tokens, we must redefine it to change type from `int` to `Parser::semantic_type`
    #define yyterminate() worse_io::Parser::make_END(m_driver.location());

    // This will track current scanner location.
    // Action is called when length of the token is known.
    #define YY_USER_ACTION m_driver.advance_column(yyleng);

    using Parser = worse_io::Parser;
%}
%option noyywrap
%option c++
%option yyclass="Scanner"
%option prefix="worse_io_"

%x COMMENT INJECT
%%
<INITIAL>^[\t ]*"[[inject{\n"   {  BEGIN(INJECT);  return Parser::make_INJECT_BEGIN(  m_driver.advance_line(1)); }
<INJECT>^[\t ]*"}]]\n"          {  BEGIN(INITIAL); return Parser::make_INJECT_END(    m_driver.advance_line(1)); }
<INJECT>.*\n                    {                  return Parser::make_STRING(yytext, m_driver.advance_line(1)); }
<INITIAL>Command                { return Parser::make_COMMAND(                        m_driver.location()); }
<INITIAL>CommandSet             { return Parser::make_COMMANDSET(                     m_driver.location()); }
<INITIAL>namespace              { return Parser::make_NAMESPACE(                      m_driver.location()); }
<INITIAL>struct                 { return Parser::make_STRUCT(                         m_driver.location()); }
<INITIAL>using                  { return Parser::make_USING(                          m_driver.location()); }
<INITIAL>enum                   { return Parser::make_ENUM(                           m_driver.location()); }
<INITIAL>Request                { return Parser::make_REQUEST(                        m_driver.location()); }
<INITIAL>Response               { return Parser::make_RESPONSE(                       m_driver.location()); }
<INITIAL>[a-z_A-Z][a-z_A-Z0-9]* { return Parser::make_STRING(yytext,                  m_driver.location()); }
<INITIAL>::                     { return Parser::make_SCOPE(                          m_driver.location()); }
<INITIAL>:                      { return Parser::make_COLON(                          m_driver.location()); }
<INITIAL>\(                     { return Parser::make_LEFTPAR(                        m_driver.location()); }
<INITIAL>\)                     { return Parser::make_RIGHTPAR(                       m_driver.location()); }
<INITIAL>\{                     { return Parser::make_LEFTCURL(                       m_driver.location()); }
<INITIAL>\}                     { return Parser::make_RIGHTCURL(                      m_driver.location()); }
<INITIAL>\<                     { return Parser::make_LESS(                           m_driver.location()); }
<INITIAL>\>                     { return Parser::make_GREATER(                        m_driver.location()); }
<INITIAL>=                      { return Parser::make_EQUALS(                         m_driver.location()); }
<INITIAL>;                      { return Parser::make_SEMICOLON(                      m_driver.location()); }
<INITIAL>,                      { return Parser::make_COMMA(                          m_driver.location()); }
<INITIAL>[1-9][0-9]*            { return Parser::make_NUMBER(strtoull(yytext, 0, 10), m_driver.location()); }
<INITIAL>"/*"                   { BEGIN(COMMENT);           }
<COMMENT>[^*\n]*                {                           } /* eat anything that's not a '*' */
<COMMENT>"*"+[^*/\n]*           {                           } /* eat up '*'s not followed by '/'s */
<COMMENT>\n                     { m_driver.advance_line(1); }
<COMMENT>"*"+"/"                { BEGIN(INITIAL);           }
<INITIAL>"//".*$                {                           }
<INITIAL>[\t ]                  {                           }
<INITIAL>[\n ]                  { m_driver.advance_line(1); }
<INITIAL><<EOF>>                { return yyterminate();     }
<INITIAL>.                      { // return Parser::make_YYerror(m_driver.location()); // bison-3.4.6
                                  m_driver.m_parser.error(m_driver.location(), "unexpected input..."); // bison-3.3.2
                                  return yyterminate();     }
%%
