#include "../../headers/tokenizer/tokenizer.h"
#include <algorithm>
#include <cctype>
#include <unordered_set>

static const std::unordered_set<std::string> KEYWORDS = {
    "INSERT", "SELECT", "DELETE", "UPDATE",  "CREATE", "DROP", "TABLE",
    "INTO",   "FROM",   "WHERE",  "PRIMARY", "KEY",    "SET",  "VALUES",
    "ORDER",  "BY"     };

static bool isKeyword(const std::string &word) {
  std::string upper = word;
  std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
  return KEYWORDS.count(upper) > 0;
}

std::vector<Token> tokenize(const std::string &input) {
  std::vector<Token> tokens;
  size_t i = 0;
  size_t n = input.size();

  while (i < n) {

    if (std::isspace(input[i])) {
      i++;
      continue;
    }

  
    if (input[i] == '(' || input[i] == ')' || input[i] == ',') {
      tokens.push_back({TokenType::PUNCTUATION, std::string(1, input[i])});
      i++;
      continue;
    }

  
    if (input[i] == '\'' || input[i] == '"') {
      char quote = input[i++];
      std::string val;
      while (i < n && input[i] != quote)
        val += input[i++];
      if (i < n)
        i++; 
      tokens.push_back({TokenType::STRING, val});
      continue;
    }

  
    if (std::isdigit(input[i]) ||
        (input[i] == '-' && i + 1 < n && std::isdigit(input[i + 1]))) {
      std::string val;
      if (input[i] == '-')
        val += input[i++];
      while (i < n && std::isdigit(input[i]))
        val += input[i++];
      tokens.push_back({TokenType::NUMBER, val});
      continue;
    }

    
    if (std::isalpha(input[i]) || input[i] == '_' || input[i] == '@' ||
        input[i] == '.') {
      std::string val;
  
      while (i < n && (std::isalnum(input[i]) || input[i] == '_' ||
                       input[i] == '@' || input[i] == '.' || input[i] == '-'))
        val += input[i++];

      std::string upper = val;
      std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

      if (isKeyword(val))
        tokens.push_back({TokenType::KEYWORD, upper});
      else
        tokens.push_back(
            {TokenType::IDENTIFIER, val}); 
      continue;
    }

  
    if (input[i] == '=' || input[i] == '<' || input[i] == '>' ||
        input[i] == '*' || input[i] == '+' || input[i] == '-' ||
        input[i] == '/') {
      std::string val;
      val += input[i];
   
      if ((input[i] == '<' || input[i] == '>') &&
          i + 1 < n && input[i + 1] == '=') {
        val += '=';
        i++;
      }
      tokens.push_back({TokenType::OPERATOR, val});
      i++;
      continue;
    }

 
    i++;
  }

  tokens.push_back({TokenType::END, ""});
  return tokens;
}

std::string tokenTypeToString(TokenType t) {
  switch (t) {
  case TokenType::KEYWORD:
    return "KEYWORD";
  case TokenType::IDENTIFIER:
    return "IDENTIFIER";
  case TokenType::NUMBER:
    return "NUMBER";
  case TokenType::STRING:
    return "STRING";
  case TokenType::OPERATOR:
    return "OPERATOR";
  case TokenType::END:
    return "END";
  case TokenType::PUNCTUATION:
    return "PUNCTUATION";
  }
  return "UNKNOWN";
}
