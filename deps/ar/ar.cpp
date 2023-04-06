#include "ar.hpp"
#include "opts.hpp"
#include <stdexcept>

using Dijkstra::Graph;
using Dijkstra::Node;
using std::cout;
using std::endl;
using std::string;

int ar::Dictionaries::operator[](std::string &word) {
  for (auto dict : this->dicts) {
    try {
      return dict[word];
    } catch (std::out_of_range e) {
    }
  }
  return -1;
}

bool ar::Dictionaries::check(std::string word) {
  for (auto dict : this->dicts) {
    try {
      auto res = dict[word];
      return true;
    } catch (std::out_of_range e) {
    }
  }
  return false;
}

void ar::Dictionaries::add(Dictionary d) { this->dicts.push_back(d); }

LongForm expand_known_abbr(NonDictWords &nonDictWords, ar::Dictionaries D,
                           NonDictWords *toExpand) {
  LongForm retvec;

  for (auto it = 0; it < nonDictWords.size(); it++) {
    auto each = nonDictWords[it];
    try {
      auto word = D.known_abbr[each];
      if (word.size() == 0) {
        toExpand->push_back(each);
        continue;
      }
      retvec.insert({each, word});
    } catch (std::out_of_range e) {
      toExpand->push_back(each);
    }
  }

  return retvec;
}

bool is_vowels(char character) {
  if (character > 'z' || character < 'a') {
    return false;
  }
  switch (character) {
  case 'a':
  case 'e':
  case 'i':
  case 'o':
  case 'u':
    return true;
  default:
    return false;
  }
}

bool is_consonant(char character) {
  if (character > 'z' || character < 'a') {
    return false;
  }
  return !is_vowels(character);
}

void check_vowels_consonants(string word, int *vowels, int *consonants) {
  for (auto each : word) {
    if (is_vowels(each)) {
      *vowels += 1;
    }
    if (is_consonant(each)) {
      *consonants += 1;
    }
  }
}

LongForm ar::expansion_matching(NonDictWords nonDictWords, Dictionaries D) {
  NonDictWords toExpand;
  LongForm retvec = expand_known_abbr(nonDictWords, D, &toExpand);

  if (toExpand.size() == 0) {
    INFO("Nothing to expand")
    return retvec;
  }

  for (auto word : toExpand) {
    INFO("HAVE TO EXPAND:" << word)
  }

  Phi phi = nullptr;
  Cfunc cost = [&D](std::string &word) { return D[word]; };
  for (auto token : toExpand) {
    for (auto dictionary : D) {
      // INFO(token);
      int vowels = 0;
      int consonants = 0;
      check_vowels_consonants(token, &vowels, &consonants);
      if (vowels >= consonants) {
        // if the token is a prefix
        phi = [&token, &D](string &tok, string &word) {
          if (tok.length() != token.length()) {
            return -1;
          }
          for (auto i = 0; i < tok.length(); i++) {
            if (word[i] != tok[i]) {
              return -1;
            }
          }
          return 1;
        };
      } else {
        phi = [&token, &D](string &tok, string &word) {
          int pos = 0;
          if (tok[pos] != word[pos]) {
            auto n_pos = token.find(tok);
            if(n_pos != string::npos) {
              pos = n_pos;
            } else {
              return -1;
            }
          }
          std::cout << tok << " " << word << std::endl;
          int top_counter = 0;
          int counter = 0;
          // Ensure that the word contains all the needed letters
          for (counter = 0; counter < token.length(); counter++) {
            for (auto j = pos; j < word.length(); j++) {
              if (token[counter] == word[j]) {
                pos = j;
                top_counter++;
                break;
              }
            }
          }
          if (top_counter != counter) {
            return -1;
          }
          return 1;
        };
      }
      Lmatch matches = string_matching(token, dictionary, phi, cost);
      if (matches.size() > 0) {
        for (auto &each : matches) {
          retvec.insert({token, each});
        }
        break;
      }
    }
  }

  return retvec;
}

Lmatch ar::string_matching(std::string token, Dictionary D, Phi phi,
                           Cfunc cost) {
  // Graph G = initializeMatchingGraph(token);
  //
  // int index = 0;
  // for (auto word : D) {
  //
  //   vector<pair<string, string>> matching_seq = BYP(token, word, phi);
  //
  //   cout << " first " << " second " << endl;
  //
  //   // mathing_seq <- BYP(token, word, phi(word))
  //   // for (<ch_i..ch_j>, word) e matching_seq do
  //   for (pair<string, string> match : matching_seq) {
  //     cout << "| " << std::setw(8) << match.first.c_str()
  //          << " | " << std::setw(8) << match.second.c_str() << " |" << endl;
  //     // hello
  //     // hel
  //     int i = token.find(match.first);    // 0
  //     int j = i + match.first.size() - 1; // 0 + 3
  //               // 0, 3, hel,         cost
  //     Node from = {i, j, match.first, token.substr(i, j), cost(match.first)};
  //
  //             // 3, 4, hello,        cost
  //     Node to = {j + 1, j, match.second, token.substr(j + 1, j),
  //     cost(match.second)};
  //
  //     // G(E) <- G(E) U {(i, j), word, c(word)};
  //     G[from.slice].push_back({.from = from, .to = to});
  //   }
  // }
  //
  // Node start = {0, 0, "h", "h", INT32_MIN};
  // Node end = {0, 3, "hell", "hell", (int)token.size()};
  // vector<Node> path = dijkstra(G, token, start, end);
  // for (Node label : path) {
  //   cout << "labels: " << label.word << " ";
  // }
  // return getEdgeLabels(path, G);

  // char, vec<{Node, Node}>
  // G = (V, E) <- initializeMathingGraph(token)
  Graph G = Dijkstra::initializeMatchingGraph(token);
  // h -> e    INT32_MIN
  // e -> l    INT32_MIN
  // l -> l    INT32_MIN
  // l -> '\0' INT32_MIN

  // for each: word E Dict do
  for (auto word : D) {
    // matching_seq <- BYP(token, word, phi(word))
    matches matching_seq = fuzzy::BYP(token, word, phi);
    // for each: (<ch_i..ch_j>, word) E matching_seq do
    for (auto &[first, last] : matching_seq) {
      // INFO(first << " " << last)
      int i = token.find(first);
      int j = i + first.length() - 1;
      auto any = Node{
          .i = i,
          .j = j + 1,
          .word = last,
          .cost = cost(last),
      };
      // std::cout << "HELLO " << s_first << " " << s_second << std::endl;
      // G(E) <- G(E) U {<i, j, word, cost(word)>}
      G[any.i].push_back({any});
    }
  }

  auto best_path = Dijkstra::dijkstra(G, token);
  INFO("DIJKSTRA DONE")

  //    end for
  // end for
  // best_path <- Dijkstra(G)
  // return getEdgeLabels(best_path)
  return getEdgeLabels(best_path, G);
}

Lmatch ar::split_matching(string ident, Dictionaries D, Lmatch *matches) {

  Phi phi = [&ident, &D](std::string &token, std::string &word) {
    if (token == word) {
      return 0;
    }
    return -1;
  };

  Cfunc cost = [&D](std::string &word) { return D[word]; };

  for (auto dict : D.dicts) {
    for (std::string each : string_matching(ident, dict, phi, cost)) {
      matches->push_back(each);
    };

    if (matches->size() != 0) {
      INFO("SPLIT_MATCHING DONE")
      break;
    }
  }
  Lmatch retvec;
  for (auto each : *matches) {
    auto find = ident.find(each);
    auto split = ident.substr(0, find);
    ident.erase(0, find + each.size());
    retvec.push_back(split);
  }

  if (ident != "\0" || ident != "" || ident != "\n" || ident.size() > 0) {
    retvec.push_back(ident);
  }

  retvec.erase(
      std::remove_if(retvec.begin(), retvec.end(),
                     [](const std::string &str) { return str.size() == 0; }),
      retvec.end());
  return retvec;
}

#include <filesystem>
#include <fstream>
#include <math.h>
#include <ostream>

std::unordered_map<std::string, std::string> get_abbr(string file) {
  std::unordered_map<std::string, std::string> retvec;

  std::fstream ifile;
  ifile.open(file);

  if (!ifile.is_open()) {
    std::cout << "Could not open file " << file << std::endl;
    exit(1);
  }

  string line;
  string delim = ";";
  while (std::getline(ifile, line)) {
    int first = line.find(delim);
    string word = line.substr(0, first);
    line.erase(0, first + delim.length());
    string other = line;
    retvec.insert({word, other});
  }

  return retvec;
}

ar::Dictionary get_dict(string file) {
  ar::Dictionary retvec;

  std::fstream ifile;
  ifile.open(file);

  if (!ifile.is_open()) {
    std::cout << "Could not open file " << file << std::endl;
    exit(1);
  }
  int file_size = std::count(std::istreambuf_iterator<char>(ifile),
                             std::istreambuf_iterator<char>(), '\n');

  ifile.clear();
  ifile.seekg(0);

  string line;
  string delim = ";";
  while (std::getline(ifile, line)) {
    int first = line.find(delim);
    string word = line.substr(0, first);
    line.erase(0, first + delim.length());
    int other = -1;
    try {
      other = std::stoi(line.substr(0, first));
    } catch (std::invalid_argument e) {
      other = (word.length() / std::log(word.length()));
    }
    retvec.insert({word, other});
  }

  return retvec;
}

ar::Dictionaries get_true() {
  ar::Dictionaries D;
  auto eng = get_dict("test.txt");
  auto it = get_dict("List_of_computing_and_IT_abbreviations");
  D.add(eng);
  D.add(it);
  D.known_abbr = get_abbr("known_abbr.txt");
  return D;
}

auto D = get_true();
std::string ar::do_ar(std::string token) {
  RESULT("STARTED: " << token)

  Lmatch matches;
  auto labels = ar::split_matching(token, D, &matches);

  auto thing = ar::expansion_matching(labels, D);

  // Replace the words in the identifier
  for (auto each : matches) {
    size_t index = 0;
    while (true) {
      /* Locate the substring to replace. */
      index = token.find(each, index);
      if (index == std::string::npos)
        break;

      /* Make the replacement. */
      token.replace(index, each.size(), each + "_");

      /* Advance index forward so the next iteration doesn't pick it up as
       * well.
       */
      index += each.size();
    }
  }

  for (auto &[each, word] : thing) {
    size_t index = 0;
    while (true) {
      /* Locate the substring to replace. */
      index = token.find(each, index);
      if (index == std::string::npos)
        break;

      /* Make the replacement. */
      token.replace(index, each.size(), word + "_");

      /* Advance index forward so the next iteration doesn't pick it up as
       * well.
       */
      index += word.size();
    }
  }

  RESULT(token)

  return token;
}
