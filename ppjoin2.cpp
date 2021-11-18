#include <map>
#include <mutex>
#include <sstream>
#include <thread>

#include "common.cpp"
#include "docopt.cpp/docopt.h"
using namespace std;

static const char USAGE[] = R"(
Find similarity pairs greater or equal than T from .bin file.

Usage:
    ppjoin [options] <inputbin>

Options:
    -h --help     Show this screen.
    -v --version  Show version.
    -t <T>        The parameter t for threshold [default: 0.90]
    -d <DEPTH>    The max depth for Suffix Filtering [default: 6]
    -f <FUNC>     The name of sim function in ["jaccard", "cosine"] [default: jaccard]
    -o <FILE>     Output filename: <FILE>.out [default: <inputbin.name>]
)";


void ppjoin() {
  I.resize(tokenNum);
  vector<int> start;
  start.resize(tokenNum, 0);

  for (int x = Records.size() - 1; x >= 0; x--) {
    map<int, yItem> A;
    int p = getPrefxLen(len[x], T);
    int newp = getPrefxLen(len[x], T, true);
    for (int i = 0; i < p; i++) {
      int w = Records[x][i];
      if (i < newp) {
        int j = start[w];
        double newubsize = sizeUpperBound(T, len[x], true, i);
        double ubsize = sizeUpperBound(T, len[x]);
        while (j < I[w].size() && len[I[w][j].x] > ubsize)
          j++;
        start[w] = j; // zip inverted list
        for (; j < I[w].size(); j++) {
          int y = I[w][j].x;
          if (len[y] <= newubsize) {
            auto it = A.find(y);
            if (it == A.end()) {
              yItem tmp = {1, i, I[w][j].pos};
              A[y] = tmp;
            } else {
              it->second.cnt++;
              it->second.posx = i;
              it->second.posy = I[w][j].pos;
            }
          }
        }
      }
      iiItem newI = {x, i};
      I[w].push_back(newI);
    }
    Verify(x, A, resultsVector);
  }
}

int main(int argc, const char **argv) {
  map<string, docopt::value> args =
      docopt::docopt(USAGE, {argv + 1, argv + argc}, true, "ppjoin 1.0");
  INPUTBIN = args["<inputbin>"].asString();
  string str = args["-t"].asString();
  istringstream iss(str);
  iss >> T;
  SIMFUNC = args["-f"].asString();
  OUTPUT = args["-o"].asString();
  MAXDEPTH = args["-d"].asLong();
  if (OUTPUT == "<inputbin.name>") {
    OUTPUT = INPUTBIN;
    OUTPUT.resize(OUTPUT.size() - 4);
  }
  checkSIMFUNC();
  cerr << "ppjoin, T = " << T << ", inputbin = " << INPUTBIN
       << ", simfunc = " << SIMFUNC << ", output = " << OUTPUT << endl;

  readTokenBinary(INPUTBIN);

  gettimeofday(&timeStart, NULL);
  ppjoin();
  gettimeofday(&timeEnd, NULL);

  outputResults();

  printTime("Run time : ");
  cerr << endl;
  return 0;
}
