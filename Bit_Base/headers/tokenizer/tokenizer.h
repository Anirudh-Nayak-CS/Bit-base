#pragma once
#include <string>
#include <vector>
 
enum class TokenType {
    KEYWORD,      // INSERT, SELECT, DELETE, UPDATE, CREATE, DROP, TABLE, INTO, FROM, WHERE
    IDENTIFIER,   // table names, column names
    NUMBER,       // integers
    STRING,       // quoted or unquoted string values
    END           // end of input
};
 
struct Token {
    TokenType type;
    std::string value;
};
 
std::vector<Token> tokenize(const std::string& input);
std::string tokenTypeToString(TokenType t);