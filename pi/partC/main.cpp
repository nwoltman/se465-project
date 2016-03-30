#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

unsigned int t_support = 3;
float t_confidence = 65.0;
bool interProceduralAnalysis = false;

struct NodeData {
  unsigned int support;
  set<string> siblings;
  NodeData() : support(0) {}
};

struct EdgeData {
  unsigned int support;
  float confidenceFirst; // confidence for the first node in the edge (A,B/A)
  float confidenceSecond; // confidence for the second node in the edge (A,B/B)
  EdgeData() : support(0) {}
};

map<string, set<string> > scopes; // maps scopes to the functions called in the scope
map<string, NodeData> nodes; // maps functions to data for nodes
map<pair<string, string>, EdgeData> edges; // maps function pairs to data for edges

pair<string, string> makeFunctionPair(const string& a, const string& b) {
  if (a < b) {
    return pair<string, string>(a, b);
  }
  if (a > b) {
    return pair<string, string>(b, a);
  }

  throw "ERROR: Should not be making a pair of the same function: " + a;
}

void parseCallGraph() {
  for (string line; getline(cin, line);) {
    // Find a call scope
    if (line.size() < 33 || line[30] != '\'') {
      continue;
    }

    unsigned int scopeNameLength = line.find_first_of('\'', 31) - 31;
    string scopeName = line.substr(31, scopeNameLength);
    set<string>* scopeFuncs = &scopes[scopeName];

    // Parse function calls in the scope
    while (getline(cin, line) && !line.empty()) {
      size_t prefixIndex = line.find("calls function '");
      if (prefixIndex == string::npos) {
        continue;
      }

      unsigned int fnNameStartIndex = prefixIndex + 16;
      unsigned int fnNameLength = line.find_first_of('\'', fnNameStartIndex) - fnNameStartIndex;
      string fnName = line.substr(fnNameStartIndex, fnNameLength);

      if (interProceduralAnalysis && scopes.find(fnName) != scopes.end()) {
        // Expand the function
        set<string>* expandedFuncs = &scopes[fnName];
        for (set<string>::iterator it = expandedFuncs->begin(); it != expandedFuncs->end(); ++it) {
          scopeFuncs->insert(*it);
        }
      } else {
        scopeFuncs->insert(fnName);
      }
    }

    // Make scope pairs and record siblings
    vector<string> scopeFuncsVec(scopeFuncs->begin(), scopeFuncs->end());
    for (size_t i = 0; i < scopeFuncsVec.size(); ++i) {
      string fn1 = scopeFuncsVec[i];
      ++nodes[fn1].support;

      for (size_t j = i + 1; j < scopeFuncsVec.size(); ++j) {
        string fn2 = scopeFuncsVec[j];
        nodes[fn1].siblings.insert(fn2);
        nodes[fn2].siblings.insert(fn1);
        pair<string, string> edgeKey = makeFunctionPair(fn1, fn2);
        ++edges[edgeKey].support;
      }
    }
  } // End call scope loop
}

void calculateConfidences() {
  for (map<pair<string, string>, EdgeData>::iterator it = edges.begin(); it != edges.end(); ++it) {
    pair<string, string> fnPair = it->first;
    EdgeData* edgeData = &it->second;
    edgeData->confidenceFirst = (edgeData->support / (float)nodes[fnPair.first].support) * 100;
    edgeData->confidenceSecond = (edgeData->support / (float)nodes[fnPair.second].support) * 100;
  }
}

void findBugs() {
  // For each scope
  for (map<string, set<string> >::iterator it = scopes.begin(); it != scopes.end(); ++it) {
    string scopeName = it->first;
    const set<string>* scopeFuncs = &it->second;

    // For each function in the scope
    for (set<string>::iterator scopeIt = scopeFuncs->begin(); scopeIt != scopeFuncs->end(); ++scopeIt) {
      string scopeFunc = *scopeIt;
      const set<string>* siblings = &nodes[scopeFunc].siblings;

      // For each of the scope function's siblings
      for (set<string>::iterator sibsIt = siblings->begin(); sibsIt != siblings->end(); ++sibsIt) {
        string siblingFunc = *sibsIt;
        pair<string, string> edgeKey = makeFunctionPair(scopeFunc, siblingFunc);
        EdgeData* edgeData = &edges[edgeKey];
        float confidence = scopeFunc < siblingFunc ? edgeData->confidenceFirst : edgeData->confidenceSecond;
        if (
          edgeData->support >= t_support &&
          confidence >= t_confidence &&
          scopeFuncs->find(siblingFunc) == scopeFuncs->end()
        ) {
          cout << "bug: " << scopeFunc << " in " << scopeName
               << ", pair: (" << edgeKey.first << ", " << edgeKey.second
               << "), support: " << edgeData->support
               << ", confidence: " << fixed << setprecision(2) << confidence;
          cout << '%' << endl;
        }
      }
    }
  }
}

int main(int argc, char** argv) {
  if (argc > 1 && strcmp(argv[1], "--ipa") == 0) {
    interProceduralAnalysis = true;
    switch (argc) {
      case 4: t_confidence = atof(argv[3]);
      case 3: t_support = atoi(argv[2]);
    }
  } else {
    switch (argc) {
      case 3: t_confidence = atof(argv[2]);
      case 2: t_support = atoi(argv[1]);
    }
  }

  parseCallGraph();
  calculateConfidences();
  findBugs();

  return 0;
}
