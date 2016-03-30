#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

unsigned int t_support = 3;
float t_confidence = 65.0;

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
  // cout << "input:" << endl << endl; // DEBUGGING
  for (string line; getline(cin, line);) {
    // cout << line << endl; // DEBUGGING
    // Find a call scope
    if (line.size() < 33 || line[30] != '\'') {
      continue;
    }

    unsigned int scopeNameLength = line.find_first_of('\'', 31) - 31;
    string scopeName = line.substr(31, scopeNameLength);
    set<string>* scopeFuncs = &scopes[scopeName];

    // Parse function calls in the scope
    while (getline(cin, line) && !line.empty()) {
      // cout << line << endl; // DEBUGGING
      size_t prefixIndex = line.find("calls function '");
      if (prefixIndex == string::npos) {
        continue;
      }

      unsigned int fnNameStartIndex = prefixIndex + 16;
      unsigned int fnNameLength = line.find_first_of('\'', fnNameStartIndex) - fnNameStartIndex;
      string fnName = line.substr(fnNameStartIndex, fnNameLength);
      scopeFuncs->insert(fnName);
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

  // cout << endl; // DEBUGGING
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
  switch (argc) {
    case 3: t_confidence = atof(argv[2]);
    case 2: t_support = atoi(argv[1]);
  }

  parseCallGraph();
  calculateConfidences();
  findBugs();

  // DEBUGGING
  // cout << "scopes:" << endl;
  // for (auto const &scopeEntry : scopes) {
  //   cout << "  " << scopeEntry.first << ':' << endl;
  //   for (auto const &fn : scopeEntry.second) {
  //     cout << "    - " << fn << endl;
  //   }
  // }
  // cout << endl;

  // cout << "nodes:" << endl;
  // for (auto const &nodeEntry : nodes) {
  //   cout << "  " << nodeEntry.first << ": " << nodeEntry.second.support << "  [";
  //   for (auto const &fn : nodeEntry.second.siblings) {
  //     cout << fn << ", ";
  //   }
  //   cout << "]" << endl;
  // }
  // cout << endl;

  // cout << "edges:" << endl;
  // for (auto const &edgeEntry : edges) {
  //   pair<string, string> key = edgeEntry.first;
  //   EdgeData ed = edgeEntry.second;
  //   cout << "  (" << key.first << ", " << key.second << "): {";
  //   cout << "support: " << ed.support << ", ";
  //   cout << "confidence_" << key.first << ": " << ed.confidenceFirst << ", ";
  //   cout << "confidence_" << key.second << ": " << ed.confidenceSecond << "}" << endl;
  // }

  return 0;
}
