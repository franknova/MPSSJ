#include "docopt.cpp/docopt.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sys/time.h>
using namespace std;

static const char USAGE[] = R"(
Make a .bil(Block Inverted List) file from .bin file.

Usage:
  bin2bil [options] <inputbin>

Options:
  -h --help       Show this screen.
  -v --version    Show version.
  -t <T>          LowerBound of threshold to insert into Inverted List. [default: 0.0]
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

int tokenNum; // toekn number
int n;        // Records number
double T;
vector<int> vector_id;       // record id
vector<int> len;             // length of each record
vector<vector<int>> Records; // all data
timeval timeStart, timeEnd;

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
      // cout<<id+1<<" "<<word+1<<endl;
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
    int lenx = Records[i].size();
    int prefix = int(lenx * (1 - T) + 1e-8) + 1;
    for (int j = 0; j < lenx; j++) {
      if (j > prefix)
        break;
      iiItem tmp = {i, len[i], j, 0};
      I[Records[i][j]].push_back(tmp);
    }
  }
}

void sortII() {
  Initialize();
  for (int i = 0; i < tokenNum; i++) {
    int isize = I[i].size();
    if (isize == 0)
      continue;
    sort(I[i].begin(), I[i].end());
    int lastLen = I[i][isize - 1].rlen;
    int lastHead = isize;
    for (int j = isize - 1; j >= 0; j--) {
      if (I[i][j].rlen != lastLen) {
        lastHead = j + 1;
        lastLen = I[i][j].rlen;
        I[i][j].nblock = lastHead;
      } else {
        I[i][j].nblock = lastHead;
      }
    }
  }
}

void outPut() {
  string bilname = OUTPUT + ".bil";
  ofstream fout(bilname, ofstream::binary);

  for (int i = 0; i < I.size(); i++) {
    int length = I[i].size();
    fout.write((char *)&i, sizeof(i));
    fout.write((char *)&length, sizeof(length));
    // cout << "Token" << (char)('J' - i) << " : ";
    for (int j = 0; j < I[i].size(); j++) {
      int rlen = I[i][j].rlen;
      int rid = I[i][j].x;
      int pos = I[i][j].pos;
      int nblock = I[i][j].nblock;
      fout.write((char *)&rid, sizeof(rid));
      fout.write((char *)&pos, sizeof(pos));
      fout.write((char *)&nblock, sizeof(nblock));
      // cout << "{" << rlen << "," << pos << "," << (char)(rid + 'u') << ","
      //      << nblock << "}" << endl;
    }
    // cout << endl;
  }
  fout.close();
}

void printTime(string str) {
  cerr << str << setiosflags(ios::fixed) << setprecision(3)
       << double(timeEnd.tv_sec - timeStart.tv_sec) +
              double(timeEnd.tv_usec - timeStart.tv_usec) / 1e6
       << endl
       << endl;
}

int main(int argc, const char **argv) {
  map<string, docopt::value> args =
      docopt::docopt(USAGE, {argv + 1, argv + argc}, true, "bin2bil 1.0");
  INPUTBIN = args["<inputbin>"].asString();
  OUTPUT = args["-o"].asString();
  string str = args["-t"].asString();
  istringstream iss(str);
  iss >> T;
  if (OUTPUT == "<inputbin.name>") {
    OUTPUT = INPUTBIN;
    OUTPUT.resize(OUTPUT.size() - 4);
  }
  cerr << "inputbin = " << INPUTBIN << " OUTPUT = " << OUTPUT << " T = " << T
       << endl;
  readTokenBinary(INPUTBIN);
  gettimeofday(&timeStart, NULL);
  sortII();
  gettimeofday(&timeEnd, NULL);
  outPut();
  printTime("Run time : ");

  return 0;
}
