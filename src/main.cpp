#include <iostream>
#include "pyInterpreter.hpp"

using namespace pyInterp;

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
  globals->Set("print", new PyFunction(interpreter, [](std::vector<PyValue*> args) -> PyValue* {
    for (auto arg : args) {
      std::cout << arg->toString() << std::endl;
    }

    return nullptr;
  }));

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