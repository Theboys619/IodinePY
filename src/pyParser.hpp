#pragma once
#include "pyLexer.hpp"

namespace pyInterp {
  std::map<std::string, int> PRECEDENCE = {
    { "=", 1 },

    { "&&", 4 },
    { "||", 5 },

    { "<", 7 }, { ">", 7 }, { "<=", 7 }, { ">=", 7 }, { "==", 7 }, { "!=", 7 },

    { "+", 10 }, { "-", 10 },
    { "*", 20 }, { "/", 20 }, { "%", 20 }
  };

  enum class ExprTypes {
    None,
    While,
    For,
    If,
    Int,
    Float,
    String,
    Variable,
    Identifier,
    Assign,
    Binary,
    Scope,
    FunctionCall,
    Function,
    FunctionDecl,
    Return,
    Datatype
  };

  class Expression {
    public:
    ExprTypes type;
    Token value;

    Expression* left;
    Token op;
    Expression* right;

    bool isArray;
    Expression* then;
    Expression* els;
    Expression* condition;
    Expression* scope;
    std::vector<Expression*> block;
    std::vector<Expression*> args;

    Expression* dotOp;

    Expression(Token value): type(ExprTypes::None), value(value) {};
    Expression(ExprTypes type, Token value): type(type), value(value) {};
  };

  class Parser {
    std::vector<Token> tokens;
    int pos = 0;

    Token curTok;

    Expression* ast;

    public:

    Parser(std::vector<Token> tokens): tokens(tokens) {};

    Token advance(int amt = 1) {
      if (pos + amt >= tokens.size()) return curTok = Token();
      pos += amt;
      
      return curTok = tokens[pos];
    }

    Token peek(int amt = 1) {
      if (pos + amt >= tokens.size()) return Token();

      return tokens[pos + amt];
    }

    bool isType(std::string type, std::string value, Token peeked) {
      return peeked.type == type && peeked.getString() == value;
    }
    bool isType(std::string type, Token peeked) {
      return peeked.type == type;
    }
    bool isType(std::string type, std::string value) {
      return curTok.type == type && curTok.getString() == value;
    }
    bool isType(std::string type) {
      return curTok.type == type;
    }

    bool isIgnore(Token tok) {
      return tok.type == "Delimiter" && tok.getString() == ";";
    }

    bool isEOF() {
      if (curTok.isNull()) return true;

      return curTok.type == "EndOfFile";
    }

    void skipOverVal(std::string val, Token tok) {
      if (tok.getString() != val) throw SyntaxError(tok.getString());

      advance();
    }

    void skipOver(std::string type, std::string val) {
      if (curTok.type != type || curTok.getString() != val) throw SyntaxError(curTok.getString());

      advance();
    }
    void skipOver(std::string type) {
      if (curTok.type != type) throw SyntaxError(curTok.getString());

      advance();
    }

    void skipOver(std::string type, Token tok) {
      if (tok.type != type) throw SyntaxError(tok.getString());

      advance();
    }
    void skipOver(std::string type, std::string val, Token tok) {
      if (tok.type != type || tok.getString() != val) throw SyntaxError(tok.getString());

      advance();
    }

    std::vector<Expression*> pDelimiters(std::string start, std::string end, std::string separator) {
      std::vector<Expression*> values = {};
      bool isFirst = true;

      skipOverVal(start, curTok);
      if (isIgnore(curTok)) advance();

      while (!isEOF()) {
        if (isType("Delimiter", end)) {
          break;
        } else if (isFirst) {
          isFirst = false;
        } else {
          skipOverVal(separator, curTok);
        }

        Expression* val = pExpression();
        values.push_back(val);
      }
      skipOverVal(end, curTok);

      return values;
    }

    std::vector<Expression*> pWhitespace() {
      if (!isType("Linebreak", "\n"))
        return { pExpression() };
      
      advance();

      if (!isType("Whitespace")) throw SyntaxError(curTok.getString());

      std::vector<Expression*> expressions = {};
      std::string indentSpace = curTok.getString();
      int indentSize = indentSpace.length();
      bool isFirst = true;

      advance();

      while (!isEOF()) {
        // std::cout << "Size: " << indentSize << " Value: " << curTok.getString() << " Type: " << curTok.type << std::endl;
        if (isFirst) {
          isFirst = false;
        } else {
          skipOver("Linebreak", "\n");

          if (!isType("Whitespace", indentSpace)) {
            break;
          } else {
            advance();
          }
        }

        // std::cout << "Value: " << curTok.getString() << " Type: " << curTok.type << std::endl;

        expressions.push_back(pExpression());
      }

      return expressions;
    }

    bool skipWhitespace() {
      if (!isType("Whitespace")) return false;

      while (isType("Whitespace"))
        advance();

      return true;
    }

    bool nonCallabes(Expression* expression) {
      return (
        expression->type != ExprTypes::Function
        && expression->type != ExprTypes::If
      );
    }

    Expression* isCall(Expression* expression) {
      return isType("Delimiter", "(", peek()) && nonCallabes(expression) ? pCall(expression) : expression;
    }

    Expression* pCall(Expression* expr) {
      std::string varName = expr->value.getString();
      advance();

      Expression* funcCall = new Expression(ExprTypes::FunctionCall, expr->value);
      funcCall->args = pDelimiters("(", ")", ",");

      return funcCall;
    }

    // Parse expressions like x = 5 or 5 + 5 or x = 5 + 5
    Expression* pBinary(Expression* left, int prec) {
      skipWhitespace();
      Token op = curTok;

      if (isType("Operator")) {
        std::string opvalue = op.getString();
        int newPrec = PRECEDENCE[opvalue];

        if (newPrec > prec) {
          advance();
          skipWhitespace();

          ExprTypes type = opvalue == "=" ? ExprTypes::Assign : ExprTypes::Binary;
          Expression* expr = new Expression(type, op);
          expr->left = left;
          expr->op = op;
          expr->right = pBinary(isCall(pAll()), newPrec);

          return pBinary(expr, prec);
        }
      }

      return left;
    }

    Expression* pIdentifier(Expression* oldTok) {
      oldTok->type = ExprTypes::Identifier;

      if (!isType("Delimiter", "(", peek()))
        advance();

      if (isType("Delimiter", ".")) {
        advance();
        oldTok->dotOp = pExpression();
      }

      skipWhitespace();

      return oldTok;
    }

    Expression* pFunction() {
      advance();
      skipWhitespace();
      Token identifier = curTok; // Name (identifier) of function
      advance();

      Expression* func = new Expression(ExprTypes::Function, identifier);
      std::vector<Expression*> args = pDelimiters("(", ")", ",");

      // std::cout << "Function (" << identifier.getString() << ") -> " << args.size() << std::endl;

      if (!isType("Delimiter", ":")) throw SyntaxError(curTok.getString());
      else advance();

      func->args = args;
      func->scope = new Expression(ExprTypes::Scope, identifier);
      func->scope->block = pWhitespace();

      return func;
    }

    Expression* pReturn() {
      advance(); // Skip over return token

      skipWhitespace();

      Expression* returnValue = pExpression();
      Expression* expr = new Expression(ExprTypes::Return, curTok);
      expr->scope = returnValue;

      return expr;
    }

    Expression* pIf() {
      advance();
      skipWhitespace();


      Expression* ifStmt = new Expression(ExprTypes::If, Token("If", true));
      Expression* expr = pExpression();

      skipOver("Delimiter", ":");

      ifStmt->condition = expr;
      ifStmt->els = nullptr;

      Expression* then = new Expression(ExprTypes::Scope, Token("Then", true));
      then->block = pWhitespace();

      ifStmt->then = then;
      skipWhitespace();

      if (isType("Keyword", "elif")) {
        ifStmt->els = pIf();
      } else if (isType("Keyword", "else")) {
        advance();
        skipOver("Delimiter", ":");
        Expression* els = new Expression(ExprTypes::Scope, Token("Else", true));
        els->block = pWhitespace();
        ifStmt->els = els;
      }


      skipWhitespace();

      return ifStmt;
    }

    Expression* pAll() {

      if (isType("Delimiter", "(")) {
        skipOver("Delimiter", "(");
        Expression* expr = pExpression();
        skipOver("Delimiter", ")");

        return expr;
      }

      if (isType("Keyword", "def"))
        return pFunction();

      if (isType("Keyword", "return"))
        return pReturn();

      if (isType("Keyword", "if"))
        return pIf();

      if (isType("Whitespace"))
        throw SyntaxError(" ");

      Expression* oldTok = new Expression(curTok);

      if (isType("String")) {
        oldTok->type = ExprTypes::String;
        advance();

        return oldTok;
      } else if (isType("Int")) {
        oldTok->type = ExprTypes::Int;
        advance();

        return oldTok;
      } else if (isType("Float")) {
        oldTok->type = ExprTypes::Float;
        advance();

        return oldTok;
      }

      if (isType("Identifier"))
        return pIdentifier(oldTok);

      if ((isIgnore(curTok) || isType("Linebreak", "\n")))
        return oldTok;

      return new Expression(Token());
    }
    
    Expression* pExpression() {
      return isCall(pBinary(isCall(pAll()), 0));
    }

    Expression* parse() {
      ast = new Expression(ExprTypes::Scope, Token("_TOP_", true));
      curTok = tokens[0];

      while (isType("Linebreak", "\n") || isType("Whitespace")) {
        advance();
      }

      while (!curTok.isNull() && !isEOF()) {
        Expression* expr = pExpression();
        ast->block.push_back(expr);

        if (!isEOF() && (isIgnore(curTok) || isType("Linebreak", "\n"))) {
          // std::cout << curTok.getString() << std::endl;
          while (isIgnore(curTok) || isType("Linebreak", "\n")) {
            advance();
          }
        }
      }
      // std::cout << "Hi" << std::endl;

      return ast;
    }
  };
}