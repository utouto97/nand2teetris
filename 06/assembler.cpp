#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>
using namespace std;

// delete Spaces, tabs, <CR>s, and comments
string formatLine(const string line) {
  int n = line.size();
  string ret = "";
  for (int i = 0; i < n; i++) {
    if (i + 1 < n and line.substr(i, 2) == "//") break;
    if (line[i] == ' ') continue;
    if (line[i] == '\t') continue;
    if (line[i] == '\r') continue;
    ret += line[i];
  }
  return ret;
}

// If dest is "@", it would be A-instruction
// otherwise, C-instruction
struct Statement {
  string dest, comp, jump;
  Statement() : dest(""), comp(""), jump("") {}
  Statement(string dest, string comp, string jump)
      : dest(dest), comp(comp), jump(jump) {}
};

string dec2bin(int x) {
  string ret = "";
  while (x) {
    ret += char(x % 2 + '0');
    x /= 2;
  }
  for (int i = ret.size(); i < 16; i++) ret += "0";
  reverse(ret.begin(), ret.end());
  return ret;
}

struct Symbols {
  int next;
  map<string, int> table;

  Symbols() {
    next = 16;
    for (int i = 0; i < 16; i++) {
      string symbol = "R" + to_string(i);
      table[symbol] = i;
    }

    table["SP"] = 0;
    table["LCL"] = 1;
    table["ARG"] = 2;
    table["THIS"] = 3;
    table["THAT"] = 4;
    table["SCREEN"] = 16384;
    table["KBD"] = 24576;
  }

  void add(string symbol) { table[symbol] = next++; }

  void set(string symbol, int val) { table[symbol] = val; }

  string bin(string symbol) { return dec2bin(table[symbol]); }

  bool exist(string symbol) { return table.count(symbol) > 0; }
};

bool hasChar(string s) {
  for (auto c : s) {
    if ('a' <= c and c <= 'z') return true;
    if ('A' <= c and c <= 'Z') return true;
  }
  return false;
}

vector<Statement> parse(vector<string> &program, Symbols &symbols) {
  vector<Statement> ret;
  for (auto line : program) {
    line = formatLine(line);
    if (line.empty()) continue;

    // If line starts with "@"
    if (line[0] == '@') {
      string symbol = line.substr(1, line.size() - 1);
      if (hasChar(symbol)) {
        ret.emplace_back("@", "symbol", symbol);
      } else {
        // immediate
        int val = stoi(symbol);
        ret.emplace_back("@", "imm", dec2bin(val));
      }
    } else if (line[0] == '(') {
      string symbol = line.substr(1, line.size() - 2);
      symbols.set(symbol, ret.size());
    } else {
      int phase = 0;  // 0:dest, 1:comp, 2:jump
      Statement s;
      for (auto c : line) {
        if (c == '=') {
          phase = 1;
          continue;
        }
        if (c == ';') {
          phase = 2;
          continue;
        }

        if (phase == 0) {
          s.dest += c;
        } else if (phase == 1) {
          s.comp += c;
        } else {
          s.jump += c;
        }
      }
      if (s.comp.empty()) swap(s.dest, s.comp);
      ret.emplace_back(s);
    }
  }
  return ret;
}

string codegen_c(Statement &s) {
  string a = "0", c = "101010", d = "000", j = "000";

  // set a-bit and replace 'M' with 'A'
  for (auto &c : s.comp) {
    if (c == 'M') {
      a = "1";
      c = 'A';
    }
  }

  // set c-bits
  if (s.comp == "0")
    c = "101010";
  else if (s.comp == "1")
    c = "111111";
  else if (s.comp == "-1")
    c = "111010";
  else if (s.comp == "D")
    c = "001100";
  else if (s.comp == "A")
    c = "110000";
  else if (s.comp == "!D")
    c = "001101";
  else if (s.comp == "!A")
    c = "110001";
  else if (s.comp == "-D")
    c = "001111";
  else if (s.comp == "-A")
    c = "110011";
  else if (s.comp == "D+1")
    c = "011111";
  else if (s.comp == "A+1")
    c = "110111";
  else if (s.comp == "D-1")
    c = "001110";
  else if (s.comp == "A-1")
    c = "110010";
  else if (s.comp == "D+A")
    c = "000010";
  else if (s.comp == "D-A")
    c = "010011";
  else if (s.comp == "A-D")
    c = "000111";
  else if (s.comp == "D&A")
    c = "000000";
  else if (s.comp == "D|A")
    c = "010101";

  // set d-bits
  string dlist = "AMD";
  int ilist[] = {0, 2, 1};
  for (int i = 0; i < dlist.size(); i++) {
    if (*s.dest.begin() == dlist[i]) {
      d[ilist[i]] = '1';
      s.dest.erase(s.dest.begin());
    }
  }

  // set j-bits
  if (s.jump == "JGT")
    j = "001";
  else if (s.jump == "JEQ")
    j = "010";
  else if (s.jump == "JGE")
    j = "011";
  else if (s.jump == "JLT")
    j = "100";
  else if (s.jump == "JNE")
    j = "101";
  else if (s.jump == "JLE")
    j = "110";
  else if (s.jump == "JMP")
    j = "111";

  return "111" + a + c + d + j;
}

void codegen(const string outfile, vector<Statement> &statements,
             Symbols &symbols) {
  ofstream ofs(outfile);
  for (auto s : statements) {
    // A-instructions
    if (s.dest == "@") {
      if (s.comp == "imm") {
        ofs << s.jump << endl;
      } else {
        if (!symbols.exist(s.jump)) symbols.add(s.jump);
        ofs << symbols.bin(s.jump) << endl;
      }
    } else {
      // C-instructions
      ofs << codegen_c(s) << endl;
    }
  }
}

int main(int argc, char *args[]) {
  if (argc != 2) {
    cout << "Need filename(.asm)" << endl;
    return -1;
  }

  string filename = args[1];
  ifstream ifs(filename);
  vector<string> program;
  string s;
  while (getline(ifs, s)) {
    program.emplace_back(s);
  }

  Symbols symbols;
  vector<Statement> statements = parse(program, symbols);

  string outfile = filename.substr(0, filename.size() - 4) + ".hack";
  codegen(outfile, statements, symbols);

  // Debug
  /*
  for(auto s : statements){
    cout << s.dest << ", " << s.comp << ", " << s.jump << endl;
  }
  cout << endl;
  for (auto s : symbols.table) {
    cout << s.first << ":" << s.second << endl;
  }
  */

  return 0;
}