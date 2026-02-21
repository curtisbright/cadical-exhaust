#include "exhaustive.hpp"
#include <iostream>

ExhaustiveSearch::ExhaustiveSearch(CaDiCaL::Solver * s, int order, bool only_neg, FILE * solfile, bool can_forget) : solver(s) {
    if (order == 0) {
        // No order provided; run exhaustive search on all variables
        n = s->vars();
    } else {
        n = order;
    }
    this->only_neg = only_neg;
    this->solfile = solfile;
    this->can_forget = can_forget;
    assign = new char[n];
    solver->connect_external_propagator(this);
    for (int i = 0; i < n; i++) {
        assign[i] = l_Undef;
    }
    std::cout << "c Running exhaustive search on " << n << " variables" << std::endl;
    // The root-level of the trail is always there
    current_trail.push_back(std::vector<int>());
    // Observe the variables used for exhaustive generation
    for (int i = 0; i < n; i++) {
        solver->add_observed_var(i+1);
    }
}

ExhaustiveSearch::~ExhaustiveSearch () {
    if (n != 0) {
        solver->disconnect_external_propagator ();
        delete [] assign;
        std::cout << "c Number of solutions: " << sol_count << std::endl;
#ifdef PRINT_CALLBACK_TIME
        std::cout << "c Time spent in IPASIR-UP callbacks: " << callback_time << " sec" << std::endl;
#endif
    }
}

void ExhaustiveSearch::notify_assignment(const std::vector<int>& lits) {
#ifdef PRINT_CALLBACK_TIME
    clock_t begin = clock();
#endif
    for(int lit : lits) {
        if (assign[abs(lit)-1] == l_Undef) {
            num_assign++;
            assign[abs(lit)-1] = (lit > 0 ? l_True : l_False);
            current_trail.back().push_back(lit);
        }
    }
#ifdef PRINT_CALLBACK_TIME
    callback_time += (double)(clock() - begin) / CLOCKS_PER_SEC;
#endif
}

void ExhaustiveSearch::notify_new_decision_level () {
#ifdef PRINT_CALLBACK_TIME
    clock_t begin = clock();
#endif
    current_trail.push_back(std::vector<int>());
#ifdef PRINT_CALLBACK_TIME
    callback_time += (double)(clock() - begin) / CLOCKS_PER_SEC;
#endif
}

void ExhaustiveSearch::notify_backtrack (size_t new_level) {
#ifdef PRINT_CALLBACK_TIME
    clock_t begin = clock();
#endif
    while (current_trail.size() > new_level + 1) {
        for (const auto& lit: current_trail.back()) {
            const int x = abs(lit) - 1;
            assign[x] = l_Undef;
            num_assign--;
        }
        current_trail.pop_back();
    }
#ifdef PRINT_CALLBACK_TIME
    callback_time += (double)(clock() - begin) / CLOCKS_PER_SEC;
#endif
}

bool ExhaustiveSearch::cb_check_found_model (const std::vector<int> & model) {
    (void)model;
    return false;
}

bool ExhaustiveSearch::cb_has_external_clause (bool& is_forgettable) {
#ifdef PRINT_CALLBACK_TIME
    clock_t begin = clock();
#endif
    is_forgettable = can_forget;
    // If not all observed variables have been assigned then no blocking clause to learn
    if (num_assign < n) return false;
    // If all observed variables have been assigned then learn a blocking clause
    sol_count += 1;
    solver->set_num_sol(sol_count);

#ifdef VERBOSE
    if (!solfile) {
        std::cout << "c New solution: ";
    }
#endif
    std::vector<int> clause;
    for (int i = 0; i < n; i++) {
        const int lit = (i+1) * (assign[i] ? 1 : -1);
#ifdef VERBOSE
        if (lit > 0) {
            if(!solfile) {
                std::cout << lit << " ";
            } else {
                fprintf(solfile, "%d ", lit);
            }
        }
#endif
        if (lit > 0 || !only_neg) {
            clause.push_back(-lit);
        }
    }
#ifdef VERBOSE
    if(!solfile) {
        std::cout << "0" << std::endl;
    } else {
        fprintf(solfile, "0\n");
    }
#endif
#ifdef PRINT_PROCESS_TIME
    std::cout << "c Process time: " << CaDiCaL::absolute_process_time() << " s" << std::endl;
#endif
    new_clauses.push_back(clause);
    solver->add_trusted_clause(clause);
#ifdef PRINT_CALLBACK_TIME
    callback_time += (double)(clock() - begin) / CLOCKS_PER_SEC;
#endif
    return true;
}

int ExhaustiveSearch::cb_add_external_clause_lit () {
#ifdef PRINT_CALLBACK_TIME
    clock_t begin = clock();
#endif
    if (new_clauses.empty()) return 0;
    else {
        assert(!new_clauses.empty());
        size_t clause_idx = new_clauses.size() - 1;
        if (new_clauses[clause_idx].empty()) {
            new_clauses.pop_back();
            return 0;
        }

        int lit = new_clauses[clause_idx].back();
        new_clauses[clause_idx].pop_back();
        return lit;
    }
#ifdef PRINT_CALLBACK_TIME
    callback_time += (double)(clock() - begin) / CLOCKS_PER_SEC;
#endif
}

int ExhaustiveSearch::cb_decide () { return 0; }
int ExhaustiveSearch::cb_propagate () { return 0; }
int ExhaustiveSearch::cb_add_reason_clause_lit (int plit) {
    (void)plit;
    return 0;
};
