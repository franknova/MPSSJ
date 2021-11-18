#include "common.cpp"
#include "docopt.cpp/docopt.h"
using namespace std;

static const char USAGE[] = R"(
Find top-k similarity pairs from .bin file Gradually.

Usage:
    gptopk [options] <inputbin>

Options:
    -h --help     Show this screen.
    -v --version  Show version.
    -k <K>        The parameter integer k for top-k join [default: 1000]
    -m <MODE>     The mode in ["stop","decide","continue"], means the action when 0 change happen in this turn [default: continue]
    -d <DEPTH>    The max depth for Suffix Filtering [default: 6]
    -f <FUNC>     The name of sim function in ["jaccard", "cosine", "overlap"] [default: jaccard]
    -o <FILE>     Output filename: <FILE>.out [default: <inputbin.name>]
)";

priority_queue<eventItem> PQ; // event priority_queue
vector<unordered_set<int>>
    H; // A hash set to avoid duplicate calculation of (x,y)

void Initialize() {
  H.resize(n);
  for (int i = 0; i < tokenNum; i++) {
    vector<iiItem> vi;
    I.push_back(vi);
  }
  for (int i = 0; i < n; i++) {
    if (OVERLAP) {
      eventItem t = {i, Records[i][0], 0, (double)len[i]};
      PQ.push(t);
    } else {
      eventItem t = {i, Records[i][0], 0, 1.0};
      PQ.push(t);
    }
  }
}

inline void updateR(int a, int b, double sim, double tk) {
  if (sim > tk) { // greate than simk
    resultItem r = {a, b, sim};
    R.push(r);
    if (R.size() > K) {
      R.pop();
    }
  }
}

// see algorithm 1 in <<Adaptive top-k overlap set similarity joins>>
void topkJoin() {
  // Initialize();
  while (!PQ.empty()) {
    eventItem et = PQ.top();
    PQ.pop();
    int x = et.x, ei = et.ei, pos = et.pos;
    double tub = et.tub;
    int lenx = len[x];
    double tk = R.size() < K ? 0.0 : R.top().sim;
    if (tub <= tk)
      break;
    double ubsize = sizeUpperBound(tk, lenx);
    double lbsize = sizeLowerBound(tk, lenx);
    for (int i = 0; i < I[ei].size(); i++) {
      int y = I[ei][i].x, posy = I[ei][i].pos;
      int leny = len[y];
      if (leny < lbsize || leny > ubsize)
        continue;
      int tx = x < y ? x : y;
      int ty = x < y ? y : x;
      if (H[tx].end() == H[tx].find(ty)) {
        double sim = Verification(x, pos, y, posy, tk);
        H[tx].insert(ty);
        updateR(tx, ty, sim, tk);
      }
    }
    iiItem it = {x, pos};
    I[ei].push_back(it);
    if (pos + 1 >= lenx)
      continue;
    double tub2 = SimUpperBound(x, pos + 1);
    eventItem newe = {x, Records[x][pos + 1], pos + 1, tub2};
    PQ.push(newe);
  }
}

int main(int argc, const char **argv) {
  map<string, docopt::value> args =
      docopt::docopt(USAGE, {argv + 1, argv + argc}, true, "gptopk 1.0");
  INPUTBIN = args["<inputbin>"].asString();
  K = args["-k"].asLong();
  SIMFUNC = args["-f"].asString();
  OUTPUT = args["-o"].asString();
  MAXDEPTH = args["-d"].asLong();
  MODE = args["-m"].asString();
  if (OUTPUT == "<inputbin.name>") {
    OUTPUT = INPUTBIN;
    OUTPUT.resize(OUTPUT.size() - 4);
  }
  checkSIMFUNC();
  cerr << "topk, K=" << K << ", inputbin = " << INPUTBIN
       << ", simfunc = " << SIMFUNC << ", output = " << OUTPUT << endl;

  readTokenBinary(INPUTBIN);

  Initialize();
  gettimeofday(&timeStart, NULL);
  topkJoin();
  gettimeofday(&timeEnd, NULL);

  outputResults();

  printTime("Run time : ");
  cerr << endl;
  return 0;
}
