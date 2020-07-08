#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

string formatLine(const string& line) {
  int n = line.size();
  string ret = "";
  for (int i = 0; i < n; i++) {
    if (i + 1 < n and line.substr(i, 2) == "//") break;
    if (i + 1 < n and line.substr(i, 2) == "  ") continue;
    if (line[i] == '\t') continue;
    if (line[i] == '\r') continue;
    ret += line[i];
  }
  return ret;
}

struct Statement {
  string command, arg1, arg2;
  Statement() : command(""), arg1(""), arg2("") {}
  Statement(string command, string arg1, string arg2)
      : command(command), arg1(arg1), arg2(arg2) {}
};

vector<Statement> parse(vector<string>& program) {
  vector<Statement> ret;
  for (string line : program) {
    int n = line.size();
    int phase = 0;  // 0: command, 1: arg1, 2: arg2
    Statement s;
    for (char c : line) {
      if (c == ' ') {
        phase++;
        continue;
      }

      if (phase == 0)
        s.command += c;
      else if (phase == 1)
        s.arg1 += c;
      else
        s.arg2 += c;
    }

    if (s.arg2.size() == 0) swap(s.arg1, s.arg2);
    ret.emplace_back(s);
  }
  return ret;
}

struct Codegen {
  string symbolname;
  ofstream ofs;
  int label = 0;
  Codegen(const string& outfile, vector<Statement>& statements) : ofs(outfile) {
    int i;
    for (i = outfile.size() - 1; i >= 0; i--)
      if (outfile[i] == '/') {
        i++;
        break;
      }
    symbolname = outfile.substr(i, outfile.size() - i - 3);

    for (auto s : statements) {
      if (s.command == "label")
        ofs << "(l" << s.arg2 << ")" << endl;
      else if (s.command == "push") {
        load(s.arg1, s.arg2);
        push();
      } else if (s.command == "pop") {
        pop();
        store(s.arg1, s.arg2);
      } else if (s.command == "add" or s.command == "sub" or
                 s.command == "and" or s.command == "or")
        bin_op(s.command);
      else if (s.command == "eq" or s.command == "gt" or s.command == "lt")
        cond_op(s.command);
      else if (s.command == "not" or s.command == "neg")
        unary_op(s.command);
      else if (s.command == "goto") {
        ofs << "@l" << s.arg2 << endl;
        ofs << "0; JMP" << endl;
      } else if (s.command == "if-goto")
        if_goto(s.arg2);
      else if (s.command == "call")
        call(s.arg1, s.arg2);
      else if (s.command == "function")
        func(s.arg1, s.arg2);
      else if (s.command == "return")
        ret();
    }
  }

  void unary_op(string command) {
    pop();
    if (command == "neg")
      ofs << "D=-D" << endl;
    else if (command == "not")
      ofs << "D=!D" << endl;
    push();
  }

  void bin_op(string command, bool push_after = true) {
    // pop first arg (D <= 1st arg's value)
    pop();

    // dec
    ofs << "@SP" << endl;
    ofs << "M=M-1" << endl;

    // add
    ofs << "@SP" << endl;
    ofs << "A=M" << endl;

    if (command == "add")
      ofs << "D=M+D" << endl;
    else if (command == "sub")
      ofs << "D=M-D" << endl;
    else if (command == "and")
      ofs << "D=M&D" << endl;
    else if (command == "or")
      ofs << "D=M|D" << endl;

    // push result
    if (push_after) push();
  }

  void cond_op(string command) {
    bin_op("sub", false);

    ofs << "@b" << label << endl;
    if (command == "eq")
      ofs << "D; JEQ" << endl;
    else if (command == "gt")
      ofs << "D; JGT" << endl;
    else if (command == "lt")
      ofs << "D; JLT" << endl;

    ofs << "@0" << endl;
    ofs << "D=A" << endl;
    push();
    ofs << "@b" << label + 1 << endl;
    ofs << "0; JMP" << endl;

    ofs << "(b" << label << ")" << endl;
    ofs << "@0" << endl;
    ofs << "D=A" << endl;
    ofs << "D=D-1" << endl;
    push();

    ofs << "(b" << label + 1 << ")" << endl;

    label += 2;
  }

  void addr(string segment, string index) {
    if (segment == "imm") {
      ofs << "@" << index << endl;
      return;
    }

    if (segment == "temp") {
      ofs << "@R5" << endl;
      ofs << "D=A" << endl;
    } else if (segment == "pointer") {
      ofs << "@R3" << endl;
      ofs << "D=A" << endl;
    } else if (segment == "static") {
      ofs << "@" << symbolname << index << endl;
      ofs << "D=A" << endl;
    } else {
      if (segment == "local")
        ofs << "@LCL" << endl;
      else if (segment == "argument")
        ofs << "@ARG" << endl;
      else if (segment == "this")
        ofs << "@THIS" << endl;
      else if (segment == "that")
        ofs << "@THAT" << endl;

      ofs << "D=M" << endl;
    }
    ofs << "@" << index << endl;
    ofs << "A=D+A" << endl;
  }

  void load(string segment, string index) {
    if (segment == "constant") {
      ofs << "@" << index << endl;
      ofs << "D=A" << endl;
    } else {
      addr(segment, index);
      ofs << "D=M" << endl;
    }
  }

  void store(string segment, string index) {
    ofs << "@R13" << endl;
    ofs << "M=D" << endl;

    addr(segment, index);
    ofs << "D=A" << endl;
    ofs << "@R14" << endl;
    ofs << "M=D" << endl;

    ofs << "@R13" << endl;
    ofs << "D=M" << endl;

    ofs << "@R14" << endl;
    ofs << "A=M" << endl;
    ofs << "M=D" << endl;
  }

  void push() {
    // store to stack
    ofs << "@SP" << endl;
    ofs << "A=M" << endl;
    ofs << "M=D" << endl;

    // inc
    ofs << "D=A" << endl;
    ofs << "@SP" << endl;
    ofs << "M=D+1" << endl;
  }

  void pop() {
    // dec
    ofs << "@SP" << endl;
    ofs << "M=M-1" << endl;

    // load from stack
    ofs << "@SP" << endl;
    ofs << "A=M" << endl;
    ofs << "D=M" << endl;
  }

  void if_goto(string label) {
    pop();

    ofs << "@l" << label << endl;
    ofs << "D;  JNE" << endl;
  }

  void call(string f, string n) {
    // push ret addr
    ofs << "@r" << f << endl;
    ofs << "D=A" << endl;
    push();

    // push LCL, ARG, THIS, THAT
    load("imm", "LCL");
    push();
    load("imm", "ARG");
    push();
    load("imm", "THIS");
    push();
    load("imm", "THAT");
    push();

    // reposition arg, lcl
    load("imm", "SP");
    store("imm", "LCL");
    ofs << "@" << 5 + stoi(n) << endl;
    ofs << "D=D-M" << endl;
    store("imm", "ARG");

    // jump to f and label to return back
    ofs << "@f" << f << endl;
    ofs << "0; JMP" << endl;
    ofs << "(r" << f << ")" << endl;
  }

  void func(string f, string k) {
    ofs << "(f" << f << ")" << endl;

    ofs << "@" << k << endl;
    ofs << "D=A" << endl;
    ofs << "(ils" << f << ")" << endl;  // for inner loop
    ofs << "@ile" << f << endl;
    ofs << "D; JEQ" << endl;
    store("imm", "R15");
    ofs << "D=0" << endl;
    push();
    load("imm", "R15");
    ofs << "D=D-1" << endl;
    ofs << "@ils" << f << endl;
    ofs << "0; JMP" << endl;
    ofs << "(ile" << f << ")" << endl;
  }

  void ret() {
    // FRAME
    load("imm", "LCL");
    store("imm", "R15");

    // set ret value
    pop();
    store("argument", "0");

    // restore sp, that, this, arg, lcl
    load("imm", "ARG");
    ofs << "D=D+1" << endl;
    store("imm", "SP");

    load("imm", "R15");
    ofs << "A=D-1" << endl;
    ofs << "D=M" << endl;
    store("imm", "THAT");

    load("imm", "R15");
    ofs << "@2" << endl;
    ofs << "A=D-A" << endl;
    ofs << "D=M" << endl;
    store("imm", "THIS");

    load("imm", "R15");
    ofs << "@3" << endl;
    ofs << "A=D-A" << endl;
    ofs << "D=M" << endl;
    store("imm", "ARG");

    load("imm", "R15");
    ofs << "@4" << endl;
    ofs << "A=D-A" << endl;
    ofs << "D=M" << endl;
    store("imm", "LCL");

    // jump to return
    load("imm", "R15");
    ofs << "@5" << endl;
    ofs << "A=D-A" << endl;
    ofs << "0; JMP" << endl;
  }
};

int main(int argc, char* argv[]) {
  if (argc != 2) {
    cerr << "Arg error" << endl;
    return -1;
  }

  string inputfile = argv[1];
  ifstream ifs(inputfile);
  vector<string> program;
  string line;
  while (getline(ifs, line)) {
    line = formatLine(line);
    if (line == "") continue;
    program.emplace_back(line);
  }

  auto statements = parse(program);

  string outfile = inputfile.substr(0, inputfile.size() - 3) + ".asm";
  Codegen codegen(outfile, statements);
  /*
  for (auto s : statements)
    cout << s.command << ":" << s.arg1 << " " << s.arg2 << endl;
  */

  return 0;
}