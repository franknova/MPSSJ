#include <cstring>
#include <mutex>
#include <thread>

#include "common.cpp"
#include "docopt.cpp/docopt.h"
#define P 0.5
#define MAX_LEVEL 32

using namespace std;

mutex Rmutex;

static const char USAGE[] = R"(
Find top-k similarity pairs from .bin and .bii file.

Usage:
    btopk [options] <inputbin> <inputbii>

Options:
    -h --help     Show this screen.
    -v --version  Show version.
    -n <NNN>      The thread number [default: 4]
    -k <K>        The parameter integer k for top-k join [default: 1000]
    -m <MODE>     The mode in ["stop","decide","continue"], means the action when 0 change happen in this turn [default: continue]
    -d <DEPTH>    The max depth for Suffix Filtering [default: 6]
    -f <FUNC>     The name of sim function in ["jaccard", "cosine"] [default: jaccard]
    -o <FILE>     Output filename: <FILE>.out [default: <inputbin.name>]
)";

struct Node {
  resultItem key;
  Node **forward;
  Node(resultItem key = {-1, -1, -1}, int level = MAX_LEVEL) {
    this->key = key;
    forward = new Node *[level];
    memset(forward, 0, level * sizeof(Node *));
  }
};

class SkipList {
 private:
  Node *header;
  int level;
  int size;
  int random_level() {
    int level = 1;
    while ((rand() & 0xffff) < (P * 0xffff) && level < MAX_LEVEL) {
      level++;
    }
    return level;
  }

 public:
  SkipList() {
    header = new Node;
    level = 0;
  }

  ~SkipList() {
    Node *cur = header;
    Node *next = nullptr;
    while (cur) {
      next = cur->forward[0];
      delete cur;
      cur = next;
    }
    header = nullptr;
  }

  Node *getHeader() { return header; }

  int getSize() { return size; }

  void push(resultItem key) {
    Node *node = header;
    Node *update[MAX_LEVEL];
    memset(update, 0, MAX_LEVEL * sizeof(Node *));

    Rmutex.lock();
    for (int i = level - 1; i >= 0; i--) {
      while (node->forward[i] && node->forward[i]->key.sim < key.sim) {
        node = node->forward[i];
      }
      update[i] = node;
    }
    node = node->forward[0];
    int new_level = random_level();
    if (new_level > level) {
      for (int i = level; i < new_level; i++) {
        update[i] = header;
      }
      level = new_level;
    }
    Node *new_node = new Node(key, new_level);
    for (int i = 0; i < new_level; i++) {
      new_node->forward[i] = update[i]->forward[i];
      update[i]->forward[i] = new_node;
    }
    size++;

    Rmutex.unlock();
  }

  resultItem top() {
    if (header->forward[0]) {
      return header->forward[0]->key;
    }
    return {-1, -1, -1};
  }

  void push_pop(resultItem key) {
    Node *node = header;
    Node *update[MAX_LEVEL];
    memset(update, 0, MAX_LEVEL * sizeof(Node *));

    Rmutex.lock();
    for (int i = level - 1; i >= 0; i--) {
      while (node->forward[i] && node->forward[i]->key.sim < key.sim) {
        node = node->forward[i];
      }
      update[i] = node;
    }
    node = node->forward[0];
    int new_level = random_level();
    if (new_level > level) {
      for (int i = level; i < new_level; i++) {
        update[i] = header;
      }
      level = new_level;
    }
    Node *new_node = new Node(key, new_level);
    for (int i = 0; i < new_level; i++) {
      new_node->forward[i] = update[i]->forward[i];
      update[i]->forward[i] = new_node;
    }

    // pop
    node = header->forward[0];
    for (int i = 0; i < level; i++) {
      if (header->forward[i] == node) {
        header->forward[i] = node->forward[i];
      }
    }

    delete node;
    for (int i = level - 1; i >= 0; i--) {
      if (header->forward[i] == nullptr)
        level--;
      else
        break;
    }
    Rmutex.unlock();
  }
};

SkipList RR;

inline void updateR(int x, int y, double sim) {
  if (sim < 0.0) return;
  int a = x < y ? x : y;
  int b = x < y ? y : x;
  int l = RR.getSize();
  if (l < K) {
    resultItem r = {a, b, sim};
    RR.push(r);
    // if (l + 1 == K)
    //   tk = RR.top().sim;
    // } else if (sim > RR.top().sim) {
  } else if (sim > tk) {
    resultItem r = {a, b, sim};
    RR.push_pop(r);
    tk = RR.top().sim;
  }
}

// inline bool loopit(int i, int x, vector<unordered_set<int>> &H) {
inline bool loopit(int i, int x, vector<vector<int>> &H) {
  int hx = x / NNN;
  if (i >= len[x]) return false;
  double tub = SimUpperBound(x, i, true);  // new bound
  if (tub <= tk) return false;
  int w = Records[x][i];
  int j = Ibsearch(I[w], len[x]);                          // binary search
  double newubsize = sizeUpperBound(tk, len[x], true, i);  // new size bound
  while (j < I[w].size()) {
    int y = I[w][j].x;
    if (x < y) {
      if (len[y] > newubsize) break;
      int alpha = getAlapha(x, y, tk);
      if (len[y] - I[w][j].pos < alpha) {  // position filter
        j = I[w][j].nblock;
        continue;
      }
      bool findy = false;
      for (int value : H[hx]) {
        if (value == y) {
          findy = true;
          break;
        }
      }
      // if (H[hx].end() == H[hx].find(y)) {
      if (!findy) {
        double sim = Verification2(x, i, y, I[w][j].pos, tk, alpha);
        H[hx].push_back(y);
        updateR(x, y, sim);
      }
    }
    j++;
  }
  return true;
}

void doit(int start) {
  timeval aaa, bbb;
  gettimeofday(&aaa, NULL);

  // vector<unordered_set<int>> H; // avoid duplicate calculation of (x,y)
  vector<vector<int>> H;  // avoid duplicate calculation of (x,y)
  H.resize(n / NNN + 1);  // per thread
  if (OVERLAP) {
    for (int x = start; x >= 0; x -= NNN) {  // line split
      for (int i = 0; i < len[start]; i++) {
        if (!loopit(i, x, H)) break;
      }
    }
  } else {
    for (int i = 0; i < len[start]; i++) {
      for (int x = start; x >= 0; x -= NNN) {  // line split
        if (!loopit(i, x, H)) break;
      }
    }
  }
  gettimeofday(&bbb, NULL);
  cerr << double(bbb.tv_sec - aaa.tv_sec) +
              double(bbb.tv_usec - aaa.tv_usec) / 1e6
       << endl;
}

void topkJoin() {
  thread threads[NNN];
  tk = 0.0;
  for (int i = 0; i < NNN; i++) {
    threads[i] = thread(doit, n - 1 - i);
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
  K = args["-k"].asLong();
  NNN = args["-n"].asLong();
  SIMFUNC = args["-f"].asString();
  OUTPUT = args["-o"].asString();
  MAXDEPTH = args["-d"].asLong();
  MODE = args["-m"].asString();
  if (OUTPUT == "<inputbin.name>") {
    OUTPUT = INPUTBIN;
    OUTPUT.resize(OUTPUT.size() - 4);
  }
  checkSIMFUNC();
  cerr << "btopk-thread, K=" << K << ", inputbin = " << INPUTBIN
       << ", inputbii = " << INPUTBII << ", simfunc = " << SIMFUNC
       << ", output = " << OUTPUT << endl;

  readTokenBinary(INPUTBIN);
  readII(INPUTBII);

  gettimeofday(&timeStart, NULL);
  topkJoin();
  gettimeofday(&timeEnd, NULL);

  // outputResults();

  int i = 0;
  resultItem r;
  double lowsim = -1;

  int Rcnt = RR.getSize();

  Node *node = RR.getHeader()->forward[0];
  while (node) {
    if (Rcnt <= K) {
      resultsVector.push_back(node->key);
      if (lowsim < 0) {
        lowsim = node->key.sim;
      }
    }
    Rcnt--;
    node = node->forward[0];
  }

  sort(resultsVector.begin(), resultsVector.end(), outputSort);
  for (i = 0; i < resultsVector.size(); i++) {
    r = resultsVector[i];
    printf("%d %d %.3f\n", vector_id[r.x], vector_id[r.y], r.sim);
  }
  cerr << "# Lowest similarity: " << lowsim << endl << endl;
  cerr << "# Tokens: " << tokenNum << endl;
  // cerr << "# CandidateNum:  " << candidateNum << endl;
  // cerr << "# CountNum:  " << countNum << endl;
  cerr << "# Results: " << resultsVector.size() << endl;

  printTime("Run time : ");
  cerr << endl;
  return 0;
}
