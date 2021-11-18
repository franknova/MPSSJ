#include "common.cpp"
#include "docopt.cpp/docopt.h"
using namespace std;

static const char USAGE[] = R"(
Find top-k similarity pairs from .bin file Gradually.

Usage:
    atopk [options] <inputbin>

Options:
    -h --help     Show this screen.
    -v --version  Show version.
    -k <K>        The parameter integer k for top-k join [default: 1000]
    -l <L>        The step size of l-ssjoin [default: 1]
    -d <DEPTH>    The max depth for Suffix Filtering [default: 6]
    -f <FUNC>     The name of sim function in ["jaccard", "cosine", "overlap"] [default: jaccard]
    -o <FILE>     Output filename: <FILE>.out [default: <inputbin.name>]
)";

// Param of arg

int l;                        // step size of l-ssjoin
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

inline int updateL(int l, double tub, double tk, int lenx) {
  int delta;
  if (OVERLAP) {
    delta = tub - tk;
  } else {
    delta = lenx * (tub - tk);
  }
  if (l < delta)
    return l + 1;
  else if (delta < 1)
    return 1;
  return delta;
}

void topkJoin() {
  while (!PQ.empty()) {
    eventItem et = PQ.top();
    PQ.pop();
    int x = et.x, ei = et.ei, pos = et.pos;
    double tub = et.tub;
    int lenx = len[x];
    double tk = R.size() < K ? 0.0 : R.top().sim;
    if (tub <= tk)
      break;
    l = updateL(l, tub, tk, lenx);
    for (int j = 0; j < l; j++) {
      if (pos + j >= lenx)
        break;
      if (SimUpperBound(x, pos + j) <= tk)
        break;
      int jei = Records[x][pos + j];
      double lbsize = sizeLowerBound(tk, lenx);
      double ubsize = sizeUpperBound(tk, lenx);
      for (int i = 0; i < I[jei].size(); i++) {
        int y = I[jei][i].x, posy = I[jei][i].pos;
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
      iiItem it = {x, pos + j};
      I[jei].push_back(it);
    }
    if (pos + l < lenx) {
      double tub2 = SimUpperBound(x, pos + l);
      if (tub2 > tk) {
        eventItem newe = {x, Records[x][pos + l], pos + l, tub2};
        PQ.push(newe);
      }
    }
  }
}

int main(int argc, const char **argv) {
  map<string, docopt::value> args =
      docopt::docopt(USAGE, {argv + 1, argv + argc}, true, "atopk 1.0");
  INPUTBIN = args["<inputbin>"].asString();
  K = args["-k"].asLong();
  l = args["-l"].asLong();
  SIMFUNC = args["-f"].asString();
  OUTPUT = args["-o"].asString();
  MAXDEPTH = args["-d"].asLong();
  if (OUTPUT == "<inputbin.name>") {
    OUTPUT = INPUTBIN;
    OUTPUT.resize(OUTPUT.size() - 4);
  }
  checkSIMFUNC();
  cerr << "atopk, K=" << K << ", inputbin = " << INPUTBIN
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
