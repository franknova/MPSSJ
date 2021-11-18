#include <sys/time.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <queue>
#include <unordered_set>
using namespace std;

struct resultItem {
  int x, y;
  double sim;
};

struct iiItem {
  int x;
  int pos;
  // double tub;
  int nblock; // next block index, if nblock > i, the item is the head of the
              // block
};

struct eventItem {
  int x;      // record index
  int ei;     // element index
  int pos;    // element position
  double tub; // similarity upper bound
};

struct yItem {
  int cnt;
  int posx;
  int posy;
};

bool operator<(resultItem a, resultItem b) { return a.sim > b.sim; }
bool operator<(eventItem a, eventItem b) { return a.tub < b.tub; }

// Param of arg
int K, MAXDEPTH, COIN;
string INPUTBIN, INPUTBII, SIMFUNC, OUTPUT, MODE;
bool OVERLAP = false, JACCARD = false, COSINE = false;
const double eps = 1e-8;
const double dmax = 1e10;
int NNN; // the number of thread;

timeval timeStart, timeEnd;

int tokenNum;                     // token number
int n;                            // Records number
double T;                         // the threshold of ppjoin
double tk;                        // simularity of the kth result in topk
vector<int> vector_id;            // record id
vector<int> len;                  // length of each record
vector<int> prefixLen;            // prefixLen of each record
vector<vector<int>> Records;      // all data
vector<vector<iiItem>> I;         // inverted index
priority_queue<resultItem> R;     // results
vector<resultItem> resultsVector; // for sort the result

// int candidateNum = 0;
// int countNum = 0;

inline double SimUpperBound(int x, int pos, bool isNew = false) {
  if (OVERLAP) { // overlap
    return len[x] - pos;
  } else if (JACCARD) { // jaccard
    if (isNew)
      return (len[x] - pos) * 1.0 / (len[x] + pos);
    else
      return (len[x] - pos) * 1.0 / len[x];
  } else { // cosine
    if (isNew)
      return sqrt((len[x] - pos) * 1.0 / (len[x] + pos));
    else
      return sqrt((len[x] - pos) * 1.0 / len[x]);
  }
}

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

void readII(string filename) {
  int totalLen = 0, length, peakLen = 0;
  int eid, x, pos, nblock;
  ifstream file(filename, ios::in | ios::binary);
  while (file.read((char *)&eid, sizeof(int))) {
    file.read((char *)&length, sizeof(int));
    totalLen += length;
    if (length > peakLen)
      peakLen = length;
    vector<iiItem> element;
    for (int i = 0; i < length; i++) {
      file.read((char *)&x, sizeof(int));
      file.read((char *)&pos, sizeof(int));
      file.read((char *)&nblock, sizeof(int));
      iiItem it = {x, pos, nblock};
      element.push_back(it);
    }
    I.push_back(element);
  }
  file.close();
  cerr << endl << "# Element: " << I.size() << endl;
  cerr << "# Peak Element Size: " << peakLen << endl;
  cerr << "# Average Element Size: " << setiosflags(ios::fixed)
       << setprecision(3) << totalLen * 1.0 / I.size() << endl;
}

int Ibsearch(vector<iiItem> &list, int keylen) {
  int mid = 0, l = 0;
  int r = list.size() - 1;
  while (l < r) {
    mid = (l + r) / 2;
    if (len[list[mid].x] >= keylen)
      r = mid - 1;
    else {
      int nb = list[mid].nblock;
      if (nb < mid)
        l = list[nb].nblock;
      else
        l = nb;
    }
  }
  if (len[list[l].x] < keylen)
    l++;
  return l;
}

int biSearch(int value, vector<int> &array, int start, int end) {
  int mid;
  while (start < end) {
    mid = (start + end) / 2;
    if (array[mid] < value)
      start = mid + 1;
    else
      end = mid;
  }
  return start;
}

inline int dcProbe(int x, int y, int sx, int ex, int sy, int ey, int hd,
                   int dep) {
  if (ex <= sx || ey <= sy)
    return abs((ex - sx) - (ey - sy));
  int lx = ex - sx, ly = ey - sy;
  int left, right, mid, pos, tok, offset;
  int hdLeft, hdRight, hdLeftBound, hdRightBound;

  mid = sx + lx / 2, tok = Records[x][mid];
  if (lx >= ly) {
    offset = (hd - (lx - ly)) / 2 + (lx - ly), left = sy + lx / 2 - offset;
    offset = (hd - (lx - ly)) / 2, right = sy + lx / 2 + offset;
  } else {
    offset = (hd - (ly - lx)) / 2, left = sy + lx / 2 - offset;
    offset = (hd - (ly - lx)) / 2 + (ly - lx), right = sy + lx / 2 + offset;
  }
  if (left >= sy && Records[y][left] > tok ||
      right < ey && Records[y][right] < tok)
    return hd + 1;
  pos = biSearch(tok, Records[y], left >= sy ? left : sy,
                 right + 1 < ey ? right + 1 : ey);
  if (pos < ey && Records[y][pos] == tok) {
    hdLeft = hdLeftBound = abs((mid - sx) - (pos - sy));
    hdRight = hdRightBound = abs((ex - mid - 1) - (ey - pos - 1));
    if (hdLeftBound + hdRightBound > hd)
      return hdLeftBound + hdRightBound;
    if (dep < MAXDEPTH) {
      hdLeft = dcProbe(x, y, sx, mid, sy, pos, hd - hdRightBound, dep + 1);
      if (hdLeft + hdRightBound > hd)
        return hdLeft + hdRightBound;
      hdRight = dcProbe(x, y, mid + 1, ex, pos + 1, ey, hd - hdLeft, dep + 1);
    }
    if (hdLeft + hdRight > hd)
      return hdLeft + hdRight;
    return hdLeft + hdRight;
  } else {
    hdLeft = hdLeftBound = abs((mid - sx) - (pos - sy));
    hdRight = hdRightBound = abs((ex - mid - 1) - (ey - pos));
    if (hdLeftBound + hdRightBound + 1 > hd)
      return hdLeftBound + hdRightBound + 1;
    if (dep < MAXDEPTH) {
      hdLeft = dcProbe(x, y, sx, mid, sy, pos, hd - hdRightBound - 1, dep + 1);
      if (hdLeft + hdRightBound + 1 > hd)
        return hdLeft + hdRightBound + 1;
      hdRight = dcProbe(x, y, mid + 1, ex, pos, ey, hd - hdLeft - 1, dep + 1);
    }
    if (hdLeft + hdRight + 1 > hd)
      return hdLeft + hdRight + 1;
    return hdLeft + hdRight + 1;
  }
  return 0;
}

inline int getPrefxLen(int lenx, double t, bool isNew = false) {
  if (OVERLAP) { // overlap
    return lenx - int(t + eps) + 1;
  } else if (JACCARD) {
    if (!isNew)
      return int(lenx * (1 - t) + eps) + 1; // old jaccard
    // int p = len[x] - ceil(T * len[x]) + 1;
    else
      return int(lenx * (1 - t) / (1 + t) + eps) + 1; // new jaccard, Ep4
  } else {
    if (!isNew)
      return int(lenx * (1 - t * t) + eps) + 1; // old cosine
    else
      return int(lenx * (1 - t) + eps) + 1; // new cosine, Ep4
  }
}

inline int getIndexPrefixLen(int lenx, double t) {
  if (OVERLAP) { // overlap
    return lenx - int(t + eps) + 1;
  } else if (JACCARD) {
    return int(lenx - 2 * t * lenx / (1 + t) + eps) + 1;
  } else {
    return int(lenx - lenx * t + eps) + 1; // old cosine
  }
}

inline int getAlapha(int a, int b, double t) {
  int alpha;
  // candidateNum++;
  if (OVERLAP) { // overlap
    alpha = int(t + eps);
  } else if (JACCARD) { // jaccard
    alpha = ceil(t * (len[a] + len[b]) / (1.0 + t));
  } else { // cosine
    alpha = ceil(sqrt(len[a]) * sqrt(len[b]) * t);
  }
  return alpha;
}

inline double getSim(int a, int b, int overlap) {
  double sim;
  if (OVERLAP) {
    sim = overlap;
  } else if (JACCARD) {
    sim = overlap * 1.0 / (len[a] + len[b] - overlap);
  } else {
    sim = overlap * 1.0 / (sqrt(len[a]) * sqrt(len[b]));
  }
  return sim;
}

inline int getOverlap(int a, int posa, int b, int posb, int initOverlap = 0) {
  int i = posa;
  int j = posb;
  int overlap = initOverlap;
  // countNum++;
  while (i < len[a] && j < len[b]) {
    if (Records[a][i] == Records[b][j]) {
      overlap++;
      i++;
      j++;
    } else if (Records[a][i] > Records[b][j]) {
      j++;
    } else {
      i++;
    }
  }
  return overlap;
}

// for ppjoin
inline void Verify(int x, map<int, yItem> &A, vector<resultItem> &tR) {
  int a = x;
  for (auto y : A) {
    int b = y.first;
    int posa = y.second.posx;
    int posb = y.second.posy;

    // position filter
    int alpha = getAlapha(a, b, T) - y.second.cnt;
    int ubound = min(len[a] - posa, len[b] - posb);
    if (ubound < alpha)
      continue;

    // suffix filter
    int hd = len[a] + len[b] - 2 * alpha - (posa + posb);
    if (dcProbe(b, a, posb + 1, len[b], posa + 1, len[a], hd, 1) > hd)
      continue;

    // count it
    int overlap = getOverlap(a, posa + 1, b, posb + 1, y.second.cnt);
    double t = getSim(a, b, overlap);
    if (t > T - eps) {
      resultItem tmp = {a, b, t};
      tR.push_back(tmp);
    }
  }
}

// for all topk join
double Verification(int a, int posa, int b, int posb, double tk) {
  int alpha = getAlapha(a, b, tk);
  int hd = len[a] + len[b] - 2 * alpha - (posa + posb);

  // position filter
  int ubound = min(len[a] - posa, len[b] - posb);
  if (ubound < alpha)
    return -1.0;

  // suffix filter
  if (dcProbe(b, a, posb + 1, len[b], posa + 1, len[a], hd, 1) > hd)
    return -1.0;

  // count it
  int overlap = getOverlap(a, posa, b, posb);
  double t = getSim(a, b, overlap);
  return t;
}

double Verification2(int a, int posa, int b, int posb, double tk, int alpha) {
  int hd = len[a] + len[b] - 2 * alpha - (posa + posb);

  // suffix filter
  if (dcProbe(b, a, posb + 1, len[b], posa + 1, len[a], hd, 1) > hd)
    return -1.0;

  // count it
  int overlap = getOverlap(a, posa, b, posb);
  double t = getSim(a, b, overlap);
  if (t < tk)
    t = -1.0;
  return t;
}

inline double sizeLowerBound(double tk, int lenx) {
  if (OVERLAP) {
    return tk;
  } else if (JACCARD) {
    return lenx * tk;
  } else { // cosine
    return lenx * tk * tk;
  }
}

inline double sizeUpperBound(double tk, int lenx, bool isNew = false,
                             int i = 0) {
  if (OVERLAP) {
    return dmax;
  } else if (JACCARD) {
    if (!isNew)
      return lenx / tk;
    else
      return (lenx - i - i * tk) / tk + eps;
  } else { // cosine
    if (!isNew)
      return lenx / tk / tk;
    else
      return 1.0 * (lenx - i) * (lenx - i) / (lenx * tk * tk) + eps;
  }
}

void printTime(string str) {
  cerr << str << setiosflags(ios::fixed) << setprecision(3)
       << double(timeEnd.tv_sec - timeStart.tv_sec) +
              double(timeEnd.tv_usec - timeStart.tv_usec) / 1e6
       << endl
       << endl;
}

bool outputSort(const resultItem &a, const resultItem &b) {
  if (a.sim == b.sim) {
    if (vector_id[a.x] == vector_id[b.x]) {
      return vector_id[a.y] < vector_id[b.y];
    } else {
      return vector_id[a.x] < vector_id[b.x];
    }
  } else {
    return a.sim < b.sim;
  }
}

void outputResults() {
  int i = 0;
  resultItem r;
  double lowsim;

  if (!R.empty()) {
    r = R.top();
    lowsim = r.sim;
    while (!R.empty()) {
      r = R.top();
      resultsVector.push_back(r);
      R.pop();
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
  } else {
    sort(resultsVector.begin(), resultsVector.end(), outputSort);
    for (i = 0; i < resultsVector.size(); i++) {
      r = resultsVector[i];
      printf("%d %d %.3f\n", vector_id[r.x], vector_id[r.y], r.sim);
    }
    cerr << "# Tokens: " << tokenNum << endl;
    // cerr << "# CandidateNum:  " << candidateNum << endl;
    // cerr << "# CountNum:  " << countNum << endl;
    cerr << "# Results: " << resultsVector.size() << endl;
  }
}

void checkSIMFUNC() {
  if (SIMFUNC[0] == 'o') {
    OVERLAP = true;
  } else if (SIMFUNC[0] == 'j') {
    JACCARD = true;
  } else if (SIMFUNC[0] == 'c') {
    COSINE = true;
  }
}
