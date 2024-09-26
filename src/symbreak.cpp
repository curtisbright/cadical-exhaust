#include "symbreak.hpp"
#include <iostream>

SymmetryBreaker::SymmetryBreaker(CaDiCaL::Solver * s, int order) : solver(s) {
    if (order == 0) {
        // No order provided; run exhaustive search on all variables
        n = s->vars();
    } else {
        n = order;
    }
    assign = new int[n];
    fixed = new bool[n];
    solver->connect_external_propagator(this);
    for (int i = 0; i < n; i++) {
        assign[i] = l_Undef;
        fixed[i] = false;
    }
    std::cout << "c Running exhaustive search on " << n << " variables" << std::endl;
    // The root-level of the trail is always there
    current_trail.push_back(std::vector<int>());
    // Observe the edge variables for orderly generation
    for (int i = 0; i < n; i++) {
        solver->add_observed_var(i+1);
    }
}

SymmetryBreaker::~SymmetryBreaker () {
    if (n != 0) {
        solver->disconnect_external_propagator ();
        delete [] assign;
        delete [] fixed;
        printf("Number of solutions   : %ld\n", sol_count);
    }
}

void SymmetryBreaker::notify_assignment(int lit, bool is_fixed) {
    assign[abs(lit)-1] = (lit > 0 ? l_True : l_False);
    if (is_fixed) {
        fixed[abs(lit)-1] = true;
    } else {
        current_trail.back().push_back(lit);
    }
}

void SymmetryBreaker::notify_new_decision_level () {
    current_trail.push_back(std::vector<int>());
}

void SymmetryBreaker::notify_backtrack (size_t new_level) {
    while (current_trail.size() > new_level + 1) {
        for (const auto& lit: current_trail.back()) {
            const int x = abs(lit) - 1;
            // Don't remove literals that have been fixed
            if(fixed[x])
                continue;
            assign[x] = l_Undef;
        }
        current_trail.pop_back();
    }
}

bool SymmetryBreaker::cb_check_found_model (const std::vector<int> & model) {
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
        clause.push_back(-lit);
    }
#ifdef VERBOSE
    std::cout << "0" << std::endl;
    std::cout << "c Process time: " << CaDiCaL::absolute_process_time() << " s" << std::endl;
#endif
    new_clauses.push_back(clause);
    solver->add_trusted_clause(clause);

    return false;
}

bool SymmetryBreaker::cb_has_external_clause () {
    // No programmatic clause generated
    return !new_clauses.empty();
}

int SymmetryBreaker::cb_add_external_clause_lit () {
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

int SymmetryBreaker::cb_decide () { return 0; }
int SymmetryBreaker::cb_propagate () { return 0; }
int SymmetryBreaker::cb_add_reason_clause_lit (int plit) {
    (void)plit;
    return 0;
};
