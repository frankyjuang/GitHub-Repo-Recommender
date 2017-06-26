/*
 * Copyright 2017 Juang and Liang.
 * file:   main.cpp
 * repo:   NetSci/final/src
 */

#include <omp.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "myUtil.h"

#define USAGE   "final inputfile"
#define _n  1936247

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::ios;
using std::set;
using std::stod;
using std::unordered_map;

class Vertex;
typedef vector<Vertex*> Vertices;

double findIntersect(const Vertex*, const set<Vertex*>&, set<Vertex*>&);
//double calcAdamic(const vector<int>&, vector<int>*[]);

class Vertex {
 public:
    explicit Vertex(int id) : id_(id), score_(0.0) {}
    void add_neighbor(Vertex* v) {
        if (find(neighbors_.begin(), neighbors_.end(), v) == neighbors_.end()) {
            neighbors_.push_back(v);
        }
    }
    const Vertices& get_neighbors() const { return neighbors_; }
    size_t get_degree() const { return neighbors_.size(); }
    int get_id() const { return id_; }
    void inc_score(double s) { score_ += s; }
    void reset_score() { score_ = 0.0; }
    double get_score() const { return score_; }

 private:
    const int id_;
    double score_;
    Vertices neighbors_;
};

bool compare_vertices(const Vertex* v1, const Vertex* v2) {
    return v1->get_score() > v2->get_score();
}

int main(int argc, char **argv) {
    if (!myCheckArgc(argc, 2, USAGE)) return 1;

    ifstream ifile;
    ifile.open(argv[1], ios::in);
    if (!ifile.is_open()) {
        cerr << "Error: file \'" << argv[1] << "\' failed to open!!" << endl;
        return 1;
    }

    unordered_map<int, Vertex*> users, repos;

    string ln;
    vector<int> indices;
    while (ifile.good()) {
        ln.clear(); indices.clear();
        getline(ifile, ln);
        if (!parse2Int(ln, indices, ' ')) continue;

        Vertex* u;
        Vertex* r;
        auto u_it = users.find(indices[0]);
        auto r_it = repos.find(indices[1]);
        if (u_it == users.end()) {
            u = new Vertex(indices[0]);
            users.insert({indices[0], u});
        } else {
            u = u_it->second;
        }
        if (r_it == repos.end()) {
            r = new Vertex(indices[1]);
            repos.insert({indices[1], r});
        } else {
            r = r_it->second;
        }
        u->add_neighbor(r);
        r->add_neighbor(u);
    }

    size_t max_user_deg = 0, max_repo_deg = 0;
    float avg_user_deg = 0.0, avg_repo_deg = 0.0;

    for (auto& it : users) {
        Vertex& u = *(it.second);
        size_t deg = u.get_degree();
        if (deg > max_user_deg) {
            max_user_deg = deg;
        }
        avg_user_deg += deg;
    }
    avg_user_deg /= users.size();
    for (auto& it : repos) {
        Vertex& r = *(it.second);
        size_t deg = r.get_degree();
        if (deg > max_repo_deg) {
            max_repo_deg = deg;
        }
        avg_repo_deg += deg;
    }
    avg_repo_deg /= repos.size();

    cout << "=== Basic analysis ===" << endl
         << "number of users: " << users.size() << endl
         << "number of repos: " << repos.size() << endl
         << "Avg user degree: " << avg_user_deg << endl
         << "Avg repo degree: " << avg_repo_deg << endl
         << "Max user degree: " << max_user_deg << endl
         << "Max repo degree: " << max_repo_deg << endl;

//http://bisqwit.iki.fi/story/howto/openmp/

    int ids[] = {8425822};
    // 1811303: leomao
    // 6175880: frankyjuang
    // 8425822: sunprinceS
    Vertices target_users;
    for (int id : ids) {
        auto it = users.find(id);
        if (it != users.end()) {
            target_users.push_back(it->second);
            cout << "Target user: " << id << endl;
        } else {
            cerr << "Error: id " << id << " not found." << endl;
        }
    }

    set<Vertex*> target_repos;
    for (auto& tu : target_users) {
        // Reset previous target repos.
        for (auto& tr : target_repos)
            tr->reset_score();
        target_repos.clear();

        const Vertices& orig_repos = tu->get_neighbors();
        for (auto& orig_repo : orig_repos) {
            cout << "Repo " << orig_repo->get_id() << ", score = " << orig_repo->get_score() << endl;
        }
        #pragma omp parallel for default(none) shared(orig_repos, cout, tu, target_repos)
        for (auto or_it = orig_repos.begin(); or_it < orig_repos.end(); ++or_it) {
            if ((*or_it)->get_degree() > 5000) {
                cout << "Repo " << (*or_it)->get_id() << " too famous!" << endl;
                continue;
            }
            const Vertices orig_repo_n = (*or_it)->get_neighbors();
            const set<Vertex*> orig_repo_nset(orig_repo_n.begin(), orig_repo_n.end());
            for (auto orn_it = orig_repo_n.begin(); orn_it < orig_repo_n.end(); ++orn_it) {
                if (*orn_it == tu) {
                    cout << "user back!" << endl;
                    continue;
                }
                Vertices tmp_target_repos;
                for (auto& tr : (*orn_it)->get_neighbors()) {
                    if (find(orig_repos.begin(), orig_repos.end(), tr) != orig_repos.end()) {
                        //cout << "repo back!" << endl;
                        continue;
                    }
                    tmp_target_repos.push_back(tr);
                }
                for (auto& tr : tmp_target_repos) {
                    // Similarity between orig_repo(*or_it) and tr.
                    //double score = 1.0 / tmp_target_repos.size();
                    set<Vertex*> common_neighbors;
                    double score = findIntersect(tr, orig_repo_nset, common_neighbors);
                    tr->inc_score(score);
                }
                copy(tmp_target_repos.begin(), tmp_target_repos.end(), inserter(target_repos, target_repos.end()));
            }
        }
        Vertices tmp_target_repos;
        copy(target_repos.begin(), target_repos.end(), back_inserter(tmp_target_repos));
        sort(tmp_target_repos.begin(), tmp_target_repos.end(), compare_vertices);
        for (size_t i = 0; i < 10; ++i) {
            cout << "Repo " << tmp_target_repos[i]->get_id() << ", score = " << tmp_target_repos[i]->get_score() << endl;
        }
    }

    return 0;
}

    /*
    struct BestEdge {
        double value;
        int i, j;
    };
    BestEdge g_jaccard{0.0, 0, 0}, g_adamic{0.0, 0, 0};

    int n_threads = 30;
    #pragma omp declare reduction(isbetter:BestEdge: \
              omp_out.value<omp_in.value ? omp_out=omp_in : omp_out) \
    initializer(omp_priv = BestEdge{0.0, 0, 0})
    #pragma omp parallel for num_threads(n_threads) default(none) shared(list, cout) reduction(isbetter:g_jaccard, g_adamic)
    for(int i = 0; i < _n; i++) {
        if(list[i]->empty()) continue;

        BestEdge jaccard{0.0, 0, 0}, adamic{0.0, 0, 0};
        for(int j = i + 1; j < _n; j++) {
            if(list[j]->empty()) continue;
            vector<int> intersect;
            double tmp_j = findIntersect((*list[i]), (*list[j]), intersect);
            if(jaccard.value < tmp_j)
                 jaccard = BestEdge{tmp_j, i, j};
            if(g_jaccard.value < tmp_j)
                 g_jaccard = BestEdge{tmp_j, i, j};

            double tmp_a = calcAdamic(intersect, list);
            if(adamic.value < tmp_a)
                 adamic = BestEdge{tmp_a, i, j};
            if(g_adamic.value < tmp_a)
                 g_adamic = BestEdge{tmp_a, i, j};
        }
        if (i % 1000 == 0) {
            cout << "finish processing node " << i << "...\n"
                  << ", J = " << jaccard.value << " at " << jaccard.j
                  << ", A = " << adamic.value << " at " << adamic.j << endl;
        }
    }

    cout << "Largest Jaccard coefficient: " << g_jaccard.value
          << "\nbetween " << g_jaccard.i << " and " << g_jaccard.j << endl;
    cout << "Largest Adamic coefficient: " << g_adamic.value
          << "\nbetween " << g_adamic.i << " and " << g_adamic.j << endl;

    return 0;
}
*/

double
findIntersect(const Vertex* v1, const set<Vertex*>& nset2, set<Vertex*>& c) {
    const Vertices n1 = v1->get_neighbors();
    const set<Vertex*> nset1(n1.begin(), n1.end());
    set_intersection(nset1.begin(), nset1.end(), nset2.begin(), nset2.end(),
            inserter(c, c.end()));
    return 1.0 * c.size() / (nset1.size() + nset2.size() - c.size());
}

//double
//calcAdamic(const vector<int>& z, vector<int>* list[]) {
    //double ada = 0;

    //for(size_t i = 0; i < z.size(); i++) {
        //if(list[z[i]]->size() <= 1) continue;
        //ada += (1 / myLog2(list[z[i]]->size()));
    //}

    //return ada;
//}
