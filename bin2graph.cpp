#include "docopt.cpp/docopt.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
using namespace std;

static const char USAGE[] = R"(
Print x -> y pair from .bin file.

Usage:
  bin2graph [options] <inputbin>

Options:
  -h --help       Show this screen.
  -v --version    Show version.
  -o <FILE>       Output filename: <FILE>.out [default: <inputbin.name>]
)";

struct iiItem {
  int x;
  int rlen;
  int pos;
  int nblock;
};

bool operator<(iiItem a, iiItem b) {
  if (a.rlen == b.rlen) {
    if (a.pos == b.pos) {
      return a.x < b.x;
    } else {
      return a.pos < b.pos;
    }
  } else {
    return a.rlen < b.rlen;
  }
}

vector<vector<iiItem>> I; // inverted index

string INPUTBIN, SIMFUNC, OUTPUT, MODE;

int tokenNum;                // toekn number
int n;                       // Records number
vector<int> vector_id;       // record id
vector<int> len;             // length of each record
vector<vector<int>> Records; // all data

void readTokenBinary(string filename) {
  int totalLen = 0;
  int id, length, word;
  ifstream file(filename, ios::in | ios::binary);
  while (file.read((char *)&id, sizeof(int))) {
    vector_id.push_back(id);
    file.read((char *)&length, sizeof(int));
    len.push_back(length);
    totalLen += length;
    vector<int> record;
    for (int i = 0; i < length; i++) {
      file.read((char *)&word, sizeof(int));
      record.push_back(word);
      cout<<id+1<<" "<<word+1<<endl;
      if (word > tokenNum)
        tokenNum = word;
    }
    Records.push_back(record);
  }
  file.close();
  tokenNum += 1;
  n = Records.size();
  cerr << "# Strings: " << n << endl;
  cerr << "# Peak Size: " << len[n - 1] << endl;
  cerr << "# Average Size: " << setiosflags(ios::fixed) << setprecision(3)
       << double(totalLen) / n << endl;
}

void Initialize() {
  for (int i = 0; i < tokenNum; i++) {
    vector<iiItem> vi;
    I.push_back(vi);
  }
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < Records[i].size(); j++) {
      iiItem tmp = {i, len[i], j, 0};
      I[Records[i][j]].push_back(tmp);
    }
  }
}



int main(int argc, const char **argv) {
  map<string, docopt::value> args =
      docopt::docopt(USAGE, {argv + 1, argv + argc}, true, "bin2graph 1.0");
  INPUTBIN = args["<inputbin>"].asString();
  OUTPUT = args["-o"].asString();
  if (OUTPUT == "<inputbin.name>") {
    OUTPUT = INPUTBIN;
    OUTPUT.resize(OUTPUT.size() - 4);
  }
  cerr << "inputbin = " << INPUTBIN << "OUTPUT = " << OUTPUT << endl;
  readTokenBinary(INPUTBIN);

  return 0;
}
