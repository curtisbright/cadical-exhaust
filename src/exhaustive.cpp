#include "exhaustive.hpp"
#include <iostream>

ExhaustiveSearch::ExhaustiveSearch(CaDiCaL::Solver * s, int order, bool only_neg) : solver(s) {
    if (order == 0) {
        // No order provided; run exhaustive search on all variables
        n = s->vars();
    } else {
        n = order;
    }
    this->only_neg = only_neg;
    //assign = new int[n];
    solver->connect_external_propagator(this);
    //for (int i = 0; i < n; i++) {
    //    assign[i] = l_Undef;
    //}
    std::cout << "c Running exhaustive search on " << n << " variables" << std::endl;
    // The root-level of the trail is always there
    //current_trail.push_back(std::vector<int>());
    // Observe the variables used for exhaustive generation
    for (int i = 0; i < n; i++) {
        solver->add_observed_var(i+1);
    }
}

ExhaustiveSearch::~ExhaustiveSearch () {
    if (n != 0) {
        solver->disconnect_external_propagator ();
        //delete [] assign;
        printf("Number of solutions   : %ld\n", sol_count);
    }
}

void ExhaustiveSearch::notify_assignment(const std::vector<int>& lits) {
    //for(int lit : lits) {
    //    assign[abs(lit)-1] = (lit > 0 ? l_True : l_False);
    //    current_trail.back().push_back(lit);
    //}
    (void)lits;
}

void ExhaustiveSearch::notify_new_decision_level () {
    //current_trail.push_back(std::vector<int>());
}

void ExhaustiveSearch::notify_backtrack (size_t new_level) {
    //while (current_trail.size() > new_level + 1) {
    //    for (const auto& lit: current_trail.back()) {
    //        const int x = abs(lit) - 1;
    //        assign[x] = l_Undef;
    //    }
    //    current_trail.pop_back();
    //}
    (void)new_level;
}

bool ExhaustiveSearch::cb_check_found_model (const std::vector<int> & model) {
    sol_count += 1;

#ifdef VERBOSE
    std::cout << "c New solution: ";
#endif
    std::vector<int> clause;
    for (const auto& lit: model) {
#ifdef VERBOSE
        if (lit > 0) {
            std::cout << lit << " ";
        }
#endif
        if (lit > 0 || !only_neg) {
            clause.push_back(-lit);
        }
    }
#ifdef VERBOSE
    std::cout << "0" << std::endl;
#endif
#ifdef PRINT_PROCESS_TIME
    std::cout << "c Process time: " << CaDiCaL::absolute_process_time() << " s" << std::endl;
#endif
    new_clauses.push_back(clause);
    solver->add_trusted_clause(clause);

    return false;
}

bool ExhaustiveSearch::cb_has_external_clause (bool& is_forgettable) {
    (void)is_forgettable;
    // No programmatic clause generated
    return !new_clauses.empty();
}

int ExhaustiveSearch::cb_add_external_clause_lit () {
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
}

int ExhaustiveSearch::cb_decide () { return 0; }
int ExhaustiveSearch::cb_propagate () { return 0; }
int ExhaustiveSearch::cb_add_reason_clause_lit (int plit) {
    (void)plit;
    return 0;
};
