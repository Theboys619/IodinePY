#pragma once
#include "pyParser.hpp"

// I needed inspiration to do this so some credit goes to @CSharpIsGud because I looked at his IodineJS / JS Interpreter
// I didn't copy and paste and somethings I changed to work for PY but yea Credit to him for a bit of the interpreter

namespace pyInterp {
  enum class PyTypes {
    Dictionary,
    Int,
    Double,
    String,
    Function,
    Bool,
    None
  };

  class PyScope;
  class PyValue { // Main Dynamic Value class
    public:

    PyTypes type;

    void* value;
    std::map<std::string, PyValue*> properties;
    PyScope* Scope;

    bool isReturnValue = false;

    virtual std::string getType() {
      return "value";
    }

    virtual bool HasProp(std::string propName) {
      return properties.find(propName) != properties.end();
    }

    virtual std::string getPropString(std::string prop) {
      if (!HasProp(prop)) return "";

      return properties[prop]->toString();
    }

    virtual int getPropInt(std::string prop) {
      if (!HasProp(prop)) return -1;

      return properties[prop]->toInt();
    }

    int toInt() {
      // No floating point yet

      if (type == PyTypes::String)
        return std::stoi(cast<std::string>());

      if (type == PyTypes::None)
        return 0;

      return cast<int>();
    }

    double toDouble() {
      if (type == PyTypes::String)
        return std::stod(cast<std::string>());

      if (type == PyTypes::None)
        return 0;

      return cast<double>();
    }

    virtual std::string toString() {
      switch (type) {
        case PyTypes::Int:
          return std::to_string(toInt());
        
        case PyTypes::None:
          return "None";

        default:
          return cast<std::string>();
      }
    }

    bool toBool() {
      if (type == PyTypes::Int)
        return toInt() == 1;

      if (type != PyTypes::Bool)
        return type != PyTypes::None;

      return cast<bool>();
    }

    template<typename T>
    T cast() {
      return T(*(T*)value);
    }

    PyValue() {
      type = PyTypes::None;
      value = nullptr;
    }
    PyValue(std::string val) {
      type = PyTypes::String;
      value = new std::string(val);
    }
    PyValue(int val) {
      type = PyTypes::Int;
      value = new int(val);
    }
    PyValue(double val) {
      type = PyTypes::Double;
      value = new double(val);
    }
    PyValue(bool val) {
      type = PyTypes::Int;
      value = new int(val);
    }

    PyValue* operator==(PyValue& x) {
      if (type == PyTypes::Int) {
        return new PyValue(toInt() == x.toInt());
      } else if (type == PyTypes::Double) {
        return new PyValue(toDouble() == x.toDouble());
      } else if (type == PyTypes::String) {
        return new PyValue(toString() == x.toString());
      }

      return new PyValue(toBool() == x.toBool());
    }

    PyValue* operator+(PyValue& x) {
      if (type == PyTypes::Int) {
          return new PyValue(toInt() + x.toInt());
      } else if (type == PyTypes::String) {
        return new PyValue(toString() + x.toString());
      }

      return new PyValue(toBool() + x.toBool());
    }

    PyValue* operator-(PyValue& x) {
      if (type == PyTypes::Int) {
        return new PyValue(toInt() - x.toInt());
      } else if (type == PyTypes::Double) {
        return new PyValue(toDouble() - x.toDouble());
      } else if (type == PyTypes::String) {
        throw TypeError("unsupported operand type(s) for -: 'str'");
      }

      return new PyValue(toBool() - x.toBool());
    }

    PyValue* operator*(PyValue& x) {
      if (type == PyTypes::Int) {
        if (x.type == PyTypes::String) {
          std::string str = "";

          for (int i = 0; i < toInt(); i++)
            str += x.toString();

          return new PyValue(str);
        }

        return new PyValue(toInt() * x.toInt());
      } else if (type == PyTypes::Double) {
        return new PyValue(toDouble() * toDouble());
      } else if (type == PyTypes::String) {
        if (x.type == PyTypes::Int) {
          std::string str = "";

          for (int i = 0; i < x.toInt(); i++)
            str += toString();

          return new PyValue(str);
        }
        throw TypeError("can't multiply sequence by non-int of type 'str'");
      }

      return new PyValue(toBool() * x.toBool());
    }

    PyValue* operator/(PyValue& x) {
      if (type == PyTypes::Int) {
        if (x.type == PyTypes::String) {
          throw TypeError("unsupported operand type(s) for /: 'int' and 'str'");
        }

        return new PyValue((double)toInt() / (double)x.toInt());
      } else if (type == PyTypes::Double) {
        return new PyValue(toDouble() / x.toDouble());
      } else if (type == PyTypes::String) {
        if (x.type == PyTypes::Int) {
          throw TypeError("unsupported operand type(s) for /: 'str' and 'int'");
        }
        throw TypeError("unsupported operand type(s) for /: 'str' and 'str'");
      }

      return new PyValue(toBool() / x.toBool());
    }
  };
  
  class PyScope {
    public:

    PyScope* parent = nullptr;
    std::map<std::string, PyValue*> properties = std::map<std::string, PyValue*>();

    PyScope() {};
    PyScope(PyScope* parent): parent(parent) {};

    PyScope* Lookup(std::string name) {
      if (HasProp(name)) {
        return this;
      }

      if (parent != nullptr)
        return parent->Lookup(name);
      
      return nullptr;
    }

    PyValue* Set(std::string name, PyValue* prop) {
      PyScope* scope = Lookup(name);
      if (scope != nullptr)
        return scope->Define(name, prop);
      else
        return Define(name, prop);
    }

    PyValue* Define(std::string name, PyValue* prop) {
      properties[name] = prop;
      return prop;
    }
    void Define(std::string name) {
      properties[name] = nullptr;
    }

    bool HasProp(std::string name) {
      if (properties.size() < 1) return false;
      return properties.find(name) != properties.end();
    }

    PyValue* Get(std::string name) {
      if (HasProp(name))
        return properties[name];

      PyScope* scope = Lookup(name);
      if (scope != nullptr)
        return scope->Get(name);

      throw UndeclaredError(name);
    }

    PyScope* Extend() {
      return new PyScope(this);
    }
  };

  class Interpreter;

  typedef PyValue* (*NativeFunction)(std::vector<PyValue*> args);

  class PyFunction : public PyValue {
    public:

    Interpreter* interpreter;

    NativeFunction Func = nullptr;
    std::vector<Expression*> block = std::vector<Expression*>();
    std::vector<std::string> paramNames = std::vector<std::string>();

    PyFunction(Interpreter* interpreter, NativeFunction Func): PyValue(), interpreter(interpreter), Func(Func) {
      type = PyTypes::Function;
    }

    PyFunction(Interpreter* interpreter): PyValue(), interpreter(interpreter) {
      type = PyTypes::Function;
    }

    PyValue* Call(std::vector<Expression*> args, PyScope* scope);

    virtual std::string getType() {
      return "function";
    }

    virtual std::string toString() { // Overwritable - Is that the write word? Writable?
      std::stringstream str;
      str << "<function " << properties["__name__"]->toString() << " at 0x" << static_cast<const void*>(this) << ">";
      return str.str();
    }
  };

  class Interpreter {
    public:

    Expression* ast;
    PyScope* Scope;

    Interpreter(Expression* ast): ast(ast) { // What is this?
      Scope = new PyScope();
    }

    void SetGlobals(PyScope* scope) { // TBH Could just let them pass one in the Interpret Method
      Scope = scope;
    }

    PyValue* iReturn(Expression* exp, PyScope* scope) { // Anything prefixed with "i" just means interpret
      PyValue* returnStmt = Evaluate(exp->scope, scope);
      returnStmt->isReturnValue = true;

      return returnStmt;
    }

    PyValue* iScope(Expression* exp, PyScope* scope) {
      bool scopeReturned = false;
      PyValue* val;

      // Lol sExpression | It is actually scope Expression
      for (Expression* sExp : exp->block) {
        val = Evaluate(sExp, scope);

        if (val != nullptr && val->isReturnValue)
          return val;
      }

      return val;
    }

    PyValue* iIdentifier(Expression* exp, PyScope* scope) {
      return scope->Get(exp->value.getString());
    }

    PyValue* iAssignment(Expression* exp, PyScope* scope) {
      std::string identifier = exp->left->value.getString();
      PyValue* value = scope->Define(identifier, Evaluate(exp->right, scope));

      return value;
    }

    PyValue* iFunction(Expression* exp, PyScope* scope) {
      std::string funcName = exp->value.getString();
      PyFunction* function = new PyFunction(this);
      function->Scope = scope;
      function->block = exp->scope->block;

      function->properties["__name__"] = new PyValue(funcName);
      
      for (Expression* arg : exp->args) {
        function->paramNames.push_back(arg->value.getString());
      }

      if (scope->Lookup(funcName) != nullptr) {
        throw Error("Function already defined");
      }

      scope->Define(funcName, function);

      return function;
    }

    PyValue* iFunctionCall(Expression* exp, PyScope* scope) {
      std::string funcName = exp->value.getString();
      PyValue* function = scope->Get(funcName);

      if (function->getType() == "function") {
        return ((PyFunction*)function)->Call(exp->args, scope->Extend());
      } else {
        throw Error("Not a callable function");
      }
    }

    PyValue* iOperation(std::string op, PyValue* a, PyValue* b) {
      if (op == "==") {
        return *a == *b;
      } else if (op == "+") {
        return *a + *b;
      } else if (op == "-") {
        return *a - *b;
      } else if (op == "*") {
        return *a * (*b);
      } else if (op == "/") {
        return *a / *b;
      }

      return new PyValue(false);
    }

    PyValue* iBinary(Expression* exp, PyScope* scope) {
      PyValue* left = Evaluate(exp->left, scope);
      PyValue* right = Evaluate(exp->right, scope);
      std::string op = exp->op.getString();

      return iOperation(op, left, right);
    }

    PyValue* iIf(Expression* exp, PyScope* scope) {
      PyValue* condition = Evaluate(exp->condition, scope);

      if (condition->toBool() || condition->toInt()) {
        return Evaluate(exp->then, scope->Extend());
      }

      if (exp->els != nullptr)
        return Evaluate(exp->els, scope->Extend());
      
      return nullptr;
    }

    PyValue* Interpret() {
      return Evaluate(ast, Scope);
    }

    PyValue* Evaluate(Expression* exp, PyScope* scope) { // Main evaluation using capital letter becasuse it looks nicer when doing interpreter->
      switch (exp->type) {
        case ExprTypes::Scope:
          return iScope(exp, scope);
        case ExprTypes::String:
          return new PyValue(exp->value.getString());
        case ExprTypes::Int:
          return new PyValue(exp->value.getInt(true));

        case ExprTypes::If:
          return iIf(exp, scope);

        case ExprTypes::Identifier:
          return iIdentifier(exp, scope);
        case ExprTypes::Assign:
          return iAssignment(exp, scope);
        case ExprTypes::Binary:
          return iBinary(exp, scope);

        case ExprTypes::Function:
          return iFunction(exp, scope);

        case ExprTypes::FunctionCall:
          return iFunctionCall(exp, scope);

        case ExprTypes::Return:
          return iReturn(exp, scope);

        default:
          return new PyValue();
      }
    }
  };

  PyValue* PyFunction::Call(std::vector<Expression*> args, PyScope* scope) { // Going to Clean this Up eventually and make something a lot nicer and easier to use
    std::vector<PyValue*> pyValues = {};

    if (this->Func != nullptr) { // WHy?
      for (Expression* arg : args) {
        if (arg->type == ExprTypes::Assign) {
          PyValue* argValue = interpreter->Evaluate(arg->right, scope);
          argValue->properties["__ARGNAME__"] = new PyValue(arg->left->value.getString());
          pyValues.insert(pyValues.begin(), argValue);
          continue;          
        } else {
          PyValue* argValue = interpreter->Evaluate(arg, scope);
          argValue->properties["__ARGNAME__"] = new PyValue(0);
          pyValues.push_back(argValue);
        }
      }
      
      PyValue* returnVal = this->Func(pyValues);

      return returnVal;
    }
    PyValue* returnVal = new PyValue();

    for (int i = 0; i < paramNames.size(); i++) {
      if (args[i]->type == ExprTypes::Assign) { // Yea these if statements need to go or I need to change something this is quite long
        interpreter->Evaluate(args[i], scope);
      } else {
        scope->Define(paramNames[i], interpreter->Evaluate(args[i], scope));
      }
    }

    for (Expression* expr : block) {
      returnVal = interpreter->Evaluate(expr, scope);

      if (returnVal != nullptr && returnVal->isReturnValue) return returnVal;
    }

    return returnVal;
  };
};