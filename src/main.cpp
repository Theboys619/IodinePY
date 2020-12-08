#include <iostream>
#include "pyInterpreter.hpp"

using namespace pyInterp;

PyValue* pystr(std::vector<PyValue*> args) {
  if (args.size() > 0)
    return new PyValue(args[0]->toString());
  else
    return nullptr;
}

PyValue* pyint(std::vector<PyValue*> args) {
  if (args.size() > 0) {
    if (args[0]->type == PyTypes::String)
      return new PyValue(args[0]->toInt());
    else if (args[0]->type == PyTypes::Int) return new PyValue(args[0]->toInt());
    else return new PyValue(0);
  } else
    return new PyValue(0);
}

PyValue* input(std::vector<PyValue*> args) {
  if (args.size() > 0) {
    std::string msg = args[0]->toString();
    std::cout << msg;
  }

  std::string x;

  std::getline(std::cin, x);

  return new PyValue(x);
}

PyValue* print(std::vector<PyValue*> args) {
  for (auto arg : args) {
    std::cout << arg->toString();
  }

  std::cout << std::endl;

  return nullptr;
}

int main(void) {
  std::string data = readFile("./tests/main.py");

  Lexer lexer = Lexer(data);
  std::vector<Token> tokens = lexer.tokenize();

  // for (auto token : tokens) {
  //   token.debugPrint();
  // }

  Parser parser = Parser(tokens);
  Expression* ast = parser.parse();
  // std::cout << ast->block[0]->right->value.getString() << std::endl;

  // std::cout << "OK" << std::endl;

  Interpreter* interpreter = new Interpreter(ast);

  PyScope* globals = new PyScope();
  globals->Set("print", new PyFunction(interpreter, print));
  globals->Set("input", new PyFunction(interpreter, input));
  globals->Set("str", new PyFunction(interpreter, pystr));
  globals->Set("int", new PyFunction(interpreter, pyint));


  interpreter->SetGlobals(globals);
  try {
    interpreter->Interpret();    
  } catch (std::exception &e) {
    std::cerr << "\033[31m";
    std::cerr << "Traceback (most recent call last):\n  File \"unknown\", line ?, in <module>\n";
    std::cerr << e.what();
    std::cerr << "\033[0m" << std::endl;
    return 0;
  }
}