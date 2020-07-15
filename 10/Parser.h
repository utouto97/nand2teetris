#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

#include "Tokenizer.h"

struct Node {
  string val;
  bool terminal;
  vector<Node*> children;

  Node(string val, bool terminal = false) : val(val), terminal(terminal) {}

  void add_children(Node* c) { children.emplace_back(c); }

  void print(ofstream& ofs, int k = 0) {
    if (terminal) {
      for (int i = 0; i < k; i++) ofs << "  ";
      ofs << val << endl;
    } else {
      for (int i = 0; i < k; i++) ofs << "  ";
      ofs << "<" << val << ">" << endl;
      for (auto c : children) {
        c->print(ofs, k + 1);
      }
      for (int i = 0; i < k; i++) ofs << "  ";
      ofs << "</" << val << ">" << endl;
    }
  }
};

struct Parser {
  vector<Token> tokens;
  Node* root;
  int idx;

  Parser(vector<Token> tokens) : tokens(tokens), idx(0) {}

  Token next() { return tokens[idx]; }

  bool in(string s, vector<string> list) {
    for (string t : list)
      if (s == t) return true;
    return false;
  }

  Node* terminal() {
    auto t = tokens[idx];
    if (t.word == "<") t.word = "&lt;";
    if (t.word == ">") t.word = "&gt;";
    if (t.word == "\"") t.word = "&quot;";
    if (t.word == "&") t.word = "&amp;";
    string s = "<" + t.type + "> " + t.word + " </" + t.type + ">";
    idx++;
    return new Node(s, true);
  }

  Node* parse_class() {
    if (next().word != "class") return NULL;

    Node* nn;  // next_node
    auto node = new Node("class");
    node->add_children(terminal());  // 'class'
    node->add_children(terminal());  // className
    node->add_children(terminal());  // '{'
    while ((nn = parse_class_var_dec()) != NULL) {
      node->add_children(nn);  // classVarDec
    }
    while ((nn = parse_subroutine_dec()) != NULL) {
      node->add_children(nn);  // subroutineDec
    }
    node->add_children(terminal());  // '}'
    return node;
  }

  Node* parse_class_var_dec() {
    if (!in(next().word, {"static", "field"})) return NULL;

    auto node = new Node("classVarDec");
    node->add_children(terminal());  // 'static' | 'field'
    node->add_children(terminal());  // type
    node->add_children(terminal());  // varName
    while (next().word == ",") {
      node->add_children(terminal());  // ','
      node->add_children(terminal());  // varName
    }
    node->add_children(terminal());  // ';'
    return node;
  }

  Node* parse_subroutine_dec() {
    if (!in(next().word, {"constructor", "function", "method"})) return NULL;

    auto node = new Node("subroutineDec");
    node->add_children(terminal());  // 'constructor' | 'function' | 'method'
    node->add_children(terminal());  // 'void' | type
    node->add_children(terminal());  // subroutineName
    node->add_children(terminal());  // '('
    node->add_children(parse_parameter_list());   // parameterList
    node->add_children(terminal());               // ')'
    node->add_children(parse_subroutine_body());  // subroutineBody
    return node;
  }

  Node* parse_parameter_list() {
    auto node = new Node("parameterList");
    if (in(next().word, {"int", "char", "boolean"}) or
        next().type == "identifier") {
      node->add_children(terminal());  // type
      node->add_children(terminal());  // varName

      while (next().word == ",") {
        node->add_children(terminal());  // ','
        node->add_children(terminal());  // type
        node->add_children(terminal());  // varName
      }
    }
    return node;
  }

  Node* parse_subroutine_body() {
    Node* nn;
    auto node = new Node("subroutineBody");
    node->add_children(terminal());  // '{'
    while ((nn = parse_var_dec()) != NULL) {
      node->add_children(nn);  // varDec
    }
    node->add_children(parse_statements());  // statements
    node->add_children(terminal());          // '}'
    return node;
  }

  Node* parse_var_dec() {
    if (next().word != "var") return NULL;
    auto node = new Node("varDec");
    node->add_children(terminal());  // var
    node->add_children(terminal());  // type
    node->add_children(terminal());  // varName
    while (next().word == ",") {
      node->add_children(terminal());  // ','
      node->add_children(terminal());  // varName
    }
    node->add_children(terminal());  // ';'
    return node;
  }

  Node* parse_statements() {
    auto node = new Node("statements");
    while (1) {
      Node* nn;
      if ((nn = parse_while_statements()) != NULL) {
        node->add_children(nn);
        continue;
      }
      if ((nn = parse_if_statements()) != NULL) {
        node->add_children(nn);
        continue;
      }
      if ((nn = parse_return_statements()) != NULL) {
        node->add_children(nn);
        continue;
      }
      if ((nn = parse_let_statements()) != NULL) {
        node->add_children(nn);
        continue;
      }
      if ((nn = parse_do_statements()) != NULL) {
        node->add_children(nn);
        continue;
      }
      break;
    }
    return node;
  }

  Node* parse_while_statements() {
    if (next().word != "while") return NULL;

    auto node = new Node("whileStatement");
    node->add_children(terminal());          // 'while'
    node->add_children(terminal());          // '('
    node->add_children(parse_expression());  // expression
    node->add_children(terminal());          // ')'
    node->add_children(terminal());          // '{'
    node->add_children(parse_statements());  // statements
    node->add_children(terminal());          // '}'
    return node;
  }

  Node* parse_if_statements() {
    if (next().word != "if") return NULL;

    auto node = new Node("ifStatement");
    node->add_children(terminal());          // 'if'
    node->add_children(terminal());          // '('
    node->add_children(parse_expression());  // expression
    node->add_children(terminal());          // ')'
    node->add_children(terminal());          // '{'
    node->add_children(parse_statements());  // statements
    node->add_children(terminal());          // '}'
    if (next().word == "else") {
      node->add_children(terminal());          // 'else'
      node->add_children(terminal());          // '{'
      node->add_children(parse_statements());  // statements
      node->add_children(terminal());          // '}'
    }
    // node->add_children(terminal());          // '}'
    return node;
  }

  Node* parse_return_statements() {
    if (next().word != "return") return NULL;

    Node* nn;
    auto node = new Node("returnStatement");
    node->add_children(terminal());  // 'return'
    if ((nn = parse_expression()) != NULL) {
      node->add_children(nn);  // expression
    }
    node->add_children(terminal());  // ';'
    return node;
  }

  Node* parse_let_statements() {
    if (next().word != "let") return NULL;

    auto node = new Node("letStatement");
    node->add_children(terminal());  // 'let'
    node->add_children(terminal());  // varName
    if (next().word == "[") {
      node->add_children(terminal());          // '['
      node->add_children(parse_expression());  // expression
      node->add_children(terminal());          // ']'
    }
    node->add_children(terminal());          // '='
    node->add_children(parse_expression());  // expression
    node->add_children(terminal());          // ';'
    return node;
  }

  Node* parse_do_statements() {
    if (next().word != "do") return NULL;

    auto node = new Node("doStatement");
    node->add_children(terminal());  // 'do'
    node->add_children(terminal());  // subroutineName | (className | varName)
    if (next().word == ".") {
      node->add_children(terminal());  // '.'
      node->add_children(terminal());  // subroutineName
    }
    node->add_children(terminal());               // '('
    node->add_children(parse_expression_list());  // expressionList
    node->add_children(terminal());               // ')'
    node->add_children(terminal());               // ';'
    return node;
  }

  Node* parse_expression() {
    Node* nn;
    auto node = new Node("expression");
    if ((nn = parse_term()) == NULL) return NULL;
    node->add_children(nn);  // term
    while (in(next().word, {"+", "-", "*", "/", "&", "|", "<", ">", "="})) {
      node->add_children(terminal());    // op
      node->add_children(parse_term());  // term
    }
    return node;
  }

  Node* parse_term() {
    if (next().type != "identifier" and next().type != "stringConstant" and
        next().type != "integerConstant" and
        !in(next().word, {"(", "-", "~", "true", "false", "null", "this"}))
      return NULL;

    auto node = new Node("term");
    if (next().word == "(") {
      node->add_children(terminal());          // '('
      node->add_children(parse_expression());  // expression
      node->add_children(terminal());          // ')'
    } else if (in(next().word, {"-", "~"})) {
      node->add_children(terminal());    // unaryOp
      node->add_children(parse_term());  // term
    } else {
      node->add_children(terminal());  // start is always identifier
      if (in(next().word, {".", "("})) {
        if (next().word == ".") {
          node->add_children(terminal());  // '.'
          node->add_children(terminal());  // subroutineName
        }
        node->add_children(terminal());               // '('
        node->add_children(parse_expression_list());  // expressionList
        node->add_children(terminal());               // ')'
      } else if (next().word == "[") {
        node->add_children(terminal());          // "["
        node->add_children(parse_expression());  // expression
        node->add_children(terminal());          // "]"
      }
    }
    return node;
  }

  Node* parse_expression_list() {
    Node* nn;
    auto node = new Node("expressionList");
    if ((nn = parse_expression()) != NULL) {
      node->add_children(nn);  // expression
      while (next().word == ",") {
        node->add_children(terminal());          // ','
        node->add_children(parse_expression());  // expression
      }
    }
    return node;
  }
};