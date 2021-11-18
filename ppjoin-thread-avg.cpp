#include <atomic>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>

#include "common.cpp"
#include "docopt.cpp/docopt.h"
using namespace std;

static const char USAGE[] = R"(
Find similarity pairs greater or equal than T from .bin and .bii file. By multi-threading.

Usage:
    bppjoin-thread [options] <inputbin> <inputbii>

Options:
    -h --help     Show this screen.
    -v --version  Show version.
    -n <NNN>      The thread number [default: 4]
    -t <T>        The parameter t for threshold [default: 0.90]
    -d <DEPTH>    The max depth for Suffix Filtering [default: 6]
    -f <FUNC>     The name of sim function in ["jaccard", "cosine"] [default: jaccard]
    -o <FILE>     Output filename: <FILE>.out [default: <inputbin.name>]
)";

mutex Rmutex;
atomic_int theX(0);

void doit() {
  timeval aaa, bbb;
  gettimeofday(&aaa, NULL);
  vector<resultItem> tR;
  int x;
  while (true) {
    x = --theX; 
    if (x < 0) break;
    map<int, yItem> A;
    int p = getPrefxLen(len[x], T);
    int newp = getPrefxLen(len[x], T, true);
    for (int i = 0; i < newp; i++) {
      int w = Records[x][i];
      int j = Ibsearch(I[w], len[x]);  // binary search
      int newubsize =
          sizeUpperBound(T, len[x], true, i);  // new size filter, Ep5
      while (j < I[w].size()) {
        int y = I[w][j].x;
        if (x < y) {
          if (len[y] > newubsize) break;
          if (I[w][j].pos > p) {
            j = I[w][j].nblock;  // length group jump
            continue;
          }
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
        j++;
      }
    }
    Verify(x, A, tR);
  }
  gettimeofday(&bbb, NULL);
  cerr << double(bbb.tv_sec - aaa.tv_sec) +
              double(bbb.tv_usec - aaa.tv_usec) / 1e6
       << endl;

  Rmutex.lock();
  resultsVector.insert(resultsVector.end(), tR.begin(), tR.end());
  Rmutex.unlock();
}

void ppjoin() {
  thread threads[NNN];
  int number = n / NNN;
  theX.store(n);
  for (int i = 0; i < NNN; i++) {
    threads[i] = thread(doit);
  }
  for (int i = 0; i < NNN; i++) {
    threads[i].join();
  }
}

int main(int argc, const char **argv) {
  map<string, docopt::value> args =
      docopt::docopt(USAGE, {argv + 1, argv + argc}, true, "gptopk 1.0");
  INPUTBIN = args["<inputbin>"].asString();
  INPUTBII = args["<inputbii>"].asString();
  NNN = args["-n"].asLong();
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
  cerr << "bppjoin-thread, T = " << T << ", NNN = " << NNN
       << ", inputbin = " << INPUTBIN << ", simfunc = " << SIMFUNC
       << ", output = " << OUTPUT << endl;

  readTokenBinary(INPUTBIN);
  readII(INPUTBII);

  gettimeofday(&timeStart, NULL);
  ppjoin();
  gettimeofday(&timeEnd, NULL);

  outputResults();

  printTime("Run time : ");
  cerr << endl;
  return 0;
}
