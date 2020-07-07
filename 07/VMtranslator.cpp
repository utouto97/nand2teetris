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
  ofstream ofs;
  int label = 0;
  Codegen(const string& outfile, vector<Statement>& statements) : ofs(outfile) {
    for (auto s : statements) {
      if (s.command == "push")
        load(s.arg1, s.arg2), push();
      else if (s.command == "pop")
        pop();
      else if (s.command == "add" or s.command == "sub" or s.command == "and" or
               s.command == "or")
        bin_op(s.command);
      else if (s.command == "eq" or s.command == "gt" or s.command == "lt")
        cond_op(s.command);
      else
        unary_op(s.command);
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

  void load(string segment, string index) {
    ofs << "@" << index << endl;
    ofs << "D=A" << endl;
  }

  void store(string segment, string index) {}

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
};

void codegen_incsp(ofstream& ofs) { ofs << "D=M" << endl; }

void codegen_push(ofstream& ofs, string segment, string index) {
  // at first, consider that only segment == "constant"
  ofs << "@" << index << endl;
  ofs << "D=A" << endl;

  // store data to stack
  ofs << "@SP" << endl;
  ofs << "A=M" << endl;
  ofs << "M=D" << endl;

  // SP = SP + 1
}

void codegen_pop(ofstream& ofs, string segment, string index) {}

void codegen_arith(ofstream& ofs, Statement s) {}

void codegen(const string& outfile, vector<Statement>& statements) {
  ofstream ofs(outfile);
  for (auto s : statements) {
    if (s.command == "push")
      codegen_push(ofs, s.arg1, s.arg2);
    else if (s.command == "pop")
      codegen_pop(ofs, s.arg1, s.arg2);
  }
}

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