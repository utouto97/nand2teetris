#include "Tokenizer.h"

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