#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

struct Token {
  string word, type;

  Token(string word, string type) : word(word), type(type) {}
};

struct Tokenizer {
  string s;
  vector<Token> tokens;

  Tokenizer(string inputfile) {
    ifstream ifs(inputfile);
    string t;
    while (getline(ifs, t)) {
      s += erase_comment(t);
    }
  }

  string erase_comment(string s) {
    int n = s.size();
    string ret = "";
    bool comment = false;
    for (int i = 0; i < n; i++) {
      if (comment) {
        if (i + 1 < n and s.substr(i, 2) == "*/") i++, comment = false;
      } else {
        if (i + 1 < n and s.substr(i, 2) == "//") break;
        if (i + 1 < n and s.substr(i, 2) == "/*") {
          comment = true;
          continue;
        }
        ret += s[i];
      }
    }
    return ret;
  }

  vector<Token> analyze() {
    string word = "", type = "None";
    const string symbols = "{}()[].,;+-*/&|<>=_";
    const string keywords[] = {
        "class", "constructor", "function", "method",  "field", "static",
        "var",   "int",         "char",     "boolean", "void",  "true",
        "false", "null",        "this",     "let",     "do",    "if",
        "else",  "while",       "return"};
    vector<Token> tokens;
    for (auto c : s) {
      if (type == "stringConstant") {
        if (c == '\"') {
          tokens.emplace_back(word, type);
          word = "";
          type = "";
        } else
          word += c;
      } else if (is_idint_chars(c)) {
        if (type.empty()) {
          type = is_digit(c) ? "integerConstant" : "identifier";
        }
        word += c;
      } else {
        if (!word.empty()) {
          // keyword
          for (auto keyword : keywords) {
            if (word == keyword) {
              type = "keyword";
              break;
            }
          }

          tokens.emplace_back(word, type);
        }

        word = "";
        type = "";

        if (is_space(c)) continue;

        // stringconstant
        if (c == '\"') {
          type = "stringConstant";
          continue;
        }

        // symbol
        for (auto symbol : symbols) {
          if (c == symbol) {
            tokens.emplace_back(string(1, c), "symbol");
          }
        }
      }
    }
    return tokens;
  }

  bool is_idint_chars(char c) {
    if ('a' <= c and c <= 'z') return true;
    if ('A' <= c and c <= 'Z') return true;
    return is_digit(c);
  }

  bool is_digit(char c) { return '0' <= c and c <= '9'; }

  bool is_space(char c) { return c == ' ' or c == '\t'; }
};

int main(int argc, char* argv[]) {
  string inputfile = argv[1];
  for (const auto& entry : std::filesystem::directory_iterator(inputfile)) {
    string p = entry.path();
    if (p.substr(p.size() - 5, 5) != ".jack") continue;
    string outfile = p.substr(0, p.size() - 5) + "_.xml";
    ofstream ofs(outfile);
    Tokenizer tokenizer(p);
    auto tokens = tokenizer.analyze();
    ofs << "<tokens>" << endl;
    for (auto t : tokens) {
      if (t.word == "<") t.word = "&lt;";
      if (t.word == ">") t.word = "&gt;";
      if (t.word == "\"") t.word = "&quot;";
      if (t.word == "&") t.word = "&amp;";
      ofs << "<" << t.type << "> " << t.word << " </" << t.type << ">" << endl;
    }
    ofs << "</tokens>" << endl;
  }

  return 0;
}