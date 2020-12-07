#pragma once
#include <vector>
#include <unordered_map>
#include <map>
#include "pyErrors.hpp"
#include "pyFilehandling.hpp"

namespace pyInterp {

  class Token {
    void* value;
    
    public:
    std::string type;

    int line = 0;
    int index = 0;

    Token() {
      type = "Null";
      value = nullptr;
    };

    Token(std::string type, const char* val): type(type) {
      value = new std::string(val);
    }
    Token(std::string type, char val): type(type) {
      value = new std::string(1, val);
    }
    Token(std::string type, int val): type(type) {
      value = new int(val);
    }
    Token(std::string type, double val): type(type) {
      value = new double(val);
    }

    Token(std::string val, bool isIdentifier = false) {
      type = isIdentifier ? "Identifier" : "String";

      value = new std::string(val);
    }

    Token(std::string type, void* value): type(type), value(value) {}

    std::string getString() {
      return *(std::string*)value;
    }

    int getInt(bool cast = false) {
      if (cast)
        return std::stoi(getString());

      return *(int*)value;
    }

    bool isNull() {
      return type == "Null";
    }

    void debugPrint() {
      std::string text = "Token<'" + type + "', '" + getString() + "'>";
      std::cout << text << std::endl;
    }
  };

  // GRAMMAR
  std::unordered_map<std::string, std::string> keywords = { // Wait why am I using maps. Oh well
    { "and", "Operator" },
    { "or", "Operator" },

    { "def", "Keyword" },
    { "if", "Keyword" },
    { "elif", "Keyword" },
    { "else", "Keyword" },
    { "return", "Keyword" },

    { "char", "Datatype" },
    { "string", "Datatype" },
    { "int", "Datatype" },
    { "float", "Datatype" },
    { "void", "Datatype" }
  };

  class Lexer {
    public:
    std::string input;
    int length;

    std::vector<Token> tokens;

    int pos = 0;
    int line = 1;
    int index = 1;

    char curChar;

    Lexer(std::string input): input(input) {
      length = input.length() - 1;
    }

    char advance(int amt = 1) {
      pos += amt;
      index += amt;

      if (pos > length) return curChar = '\0';

      return curChar = input[pos];
    }

    char peek(int amt = 1) {
      if (pos + amt > length) return '\0';

      return input[pos + amt];
    }

    bool isWhitespace(char c) {
      return c == ' ' || c == '\r' || c == '\t';
    }

    bool isAlpha(char c) {
      return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
    }

    bool isDigit(char c) {
      return c >= '0' && c <= '9';
    }

    bool isNumber(char c) {
      return (
        (c == '-' && isDigit(peek()))
        || isDigit(c)
      );
    }

    bool isQuote(char c) {
      return c == '\"' || c == '\'';
    }

    // Kind ugly but whatever
    int isOperator(char c) {
      if (
        (c == '=' && peek() == '=') // '==' operator
        || (c == '!' && peek() == '=') // '!=' operator
        || (c == '&' && peek() == '&') // '&&' operator
        || (c == '|' && peek() == '|') // '||' operator
      ) return 2;
      else if (
        (c == '+')
        || (c == '-')
        || (c == '*')
        || (c == '/')
        || (c == '%')
        || (c == '=')
      ) return 1;
      else return 0;
    }

    bool isDelimiter(char c) {
      return (
        (c == '(')
        || (c == ')')
        || (c == '{')
        || (c == '}')
        || (c == '[')
        || (c == ']')
        || (c == ';')
        || (c == ':')
        || (c == ',')
        || (c == '.')
      );
    }

    bool isComment(char c, bool first = true) {
      return (first && curChar == '/' && peek() == '*') || (!first && curChar == '*' && peek() == '/');
    }

    std::vector<Token> tokenize() {
      curChar = input[0];

      while (curChar != '\0') {
        int lastPos = pos;

        if (isWhitespace(curChar)) {
          std::string spaces = "";

          while (isWhitespace(curChar)) {
            if (curChar == '\t')
              spaces += "  ";
            else
              spaces += curChar;
            advance();
          }

          tokens.push_back(Token("Whitespace", spaces.c_str()));
        }

        if (curChar == '\n') {
          index = 0;
          ++line;
          
          advance();

          tokens.push_back(Token("Linebreak", "\n"));
        }

        if (curChar == '#') {
          while (curChar != '\n' && curChar != '\0') {
            advance();
          }
        }

        if (isComment(curChar)) {
          advance(2);

          while (!isComment(curChar, false)) {
            advance();

            if (curChar == '\0') throw SyntaxError("EOF");
          }

          advance(2);
        }

        if (isDelimiter(curChar)) {
          Token tok = Token("Delimiter", curChar);
          tok.index = index;
          tok.line = line;

          advance();

          tokens.push_back(tok);
        }

        if (isNumber(curChar)) {
          std::string type = "Int";
          int ind = index;
          int ln = line;

          std::string val = "";

          if (curChar == '-') {
            val += curChar;
            advance();
          }

          while (isNumber(curChar)) {
            val += curChar;
            advance();

            if (curChar == '.') {
              type = "Float";
              val += ".";

              advance();
            }
          }

          Token tok = Token(type, val.c_str());
          tok.index = ind;
          tok.line = ln;

          tokens.push_back(tok);
        }

        if (isOperator(curChar) > 0) {
          int amt = isOperator(curChar);
          std::string op = std::string(1, curChar);
          int ind = index;
          int ln = line;

          for (int i = 0; i < amt - 1; i++) {
            op += advance();
          }

          advance();

          Token tok = Token("Operator", op.c_str());
          tok.index = ind;
          tok.line = ln;

          tokens.push_back(tok);
        }

        if (isQuote(curChar)) {
          int ind = index;
          int ln = line;

          std::string val = "";
          advance();

          while (!isQuote(curChar)) {
            if (curChar == '\n') throw SyntaxError("\\n");
            val += curChar;
            advance();
          }

          advance();

          Token tok = Token(val);
          tok.index = ind;
          tok.line = ln;

          tokens.push_back(tok);
        }

        if (isAlpha(curChar)) {
          std::string val = "";

          int ind = index;
          int ln = line;

          while (curChar != '\0' && isAlpha(curChar)) {
            val += curChar;
            advance();
          }

          std::string type = keywords.find(val) != keywords.end()
            ? keywords[val]
            : "Identifier";

          Token tok = Token(type, val.c_str());
          tok.index = ind;
          tok.line = ln;

          tokens.push_back(tok);
        }

        if (lastPos == pos) {
          throw SyntaxError(curChar);
        }
      }

      tokens.push_back(Token("EndOfFile", "EOF"));

      return tokens;
    }
  };
};