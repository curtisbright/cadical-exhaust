#include "exhaustive.hpp"
#include <iostream>
#include <deque>
#include <cassert>

// TODO: add back comments!

ExhaustiveSearch::ExhaustiveSearch(CaDiCaL::Solver * s, int order, bool only_neg, FILE * solfile) : solver(s) {
    if (order == 0)
        n = s->vars(); // No order provided; run exhaustive search on all variables
    else 
        n = order;

    this->only_neg = only_neg;
    this->solfile = solfile;
    
    assignment.assign(n, 0);
    assignments_by_level.push_back({});
    
    solver->connect_external_propagator(this);
    
    std::cout << "c Running exhaustive search on " << n << " variables" << std::endl;
    
    for (int i = 0; i < n; i++)
        solver->add_observed_var(i+1);
}

ExhaustiveSearch::~ExhaustiveSearch () {
    if (n != 0) {
        solver->disconnect_external_propagator ();
        std::cout << "c Number of solutions: " << sol_count << std::endl;
    }
}

void ExhaustiveSearch::notify_assignment(const std::vector<int>& lits) {
    for (int lit : lits) { // Track assignments of observed variables
        int idx = abs(lit) - 1;
        if (assignment[idx] == 0) { // Variable not yet assigned
            assignment[idx] = lit; // Store the signed literal
            assigned_count++; // Increment total assigned variables
            assignments_by_level.back().push_back(lit); // Store when this literal was assigned
        }
    }    
    
    if (assigned_count == n && !partial_solution_found) { // Found all assignments and no solution found
        partial_solution_found = true;
        block_partial_solution();
    }
}

void ExhaustiveSearch::notify_new_decision_level () {
    assignments_by_level.push_back({}); // Add new vector to track history
}

void ExhaustiveSearch::notify_backtrack (size_t new_level) {
    while (assignments_by_level.size() > new_level + 1) {
        std::vector<int>& lits_to_undo = assignments_by_level.back(); // History to wipe
        for (int lit : lits_to_undo) {
            int idx = abs(lit) - 1;
            assignment[idx] = 0; 
            assigned_count--;
        }
        assignments_by_level.pop_back(); // Remove history from list
    }
    
    if (assigned_count < n) // No longer have all assignments
        partial_solution_found = false;
}

void ExhaustiveSearch::block_partial_solution() {
    sol_count++;

#ifdef VERBOSE
    if (!solfile)
      std::cout << "c New solution: ";
#endif

    std::vector<int> clause;
    clause.reserve(n + 1);
    
    for (int i = 0; i < n; i++) {
        int lit = assignment[i];

        assert(lit != 0 && "Variable is unassigned when blocking!");
        
#ifdef VERBOSE
        if (lit > 0) {
            if(!solfile)
              std::cout << (i+1) << " ";
            else
              fprintf(solfile, "%d ", (i+1));
        }
#endif
        if (lit > 0 || !only_neg)
            clause.push_back(-lit);
    }
    
#ifdef VERBOSE
    if(!solfile) 
        std::cout << "0" << std::endl;
    else 
        fprintf(solfile, "0\n");
#endif

    new_clauses.push_back(clause);
    solver->add_trusted_clause(clause);
}

bool ExhaustiveSearch::cb_check_found_model (const std::vector<int> & model) {
    (void)model;
    return false;
}

bool ExhaustiveSearch::cb_has_external_clause (bool& is_forgettable) {
    (void)is_forgettable;
    return !new_clauses.empty();
}

int ExhaustiveSearch::cb_add_external_clause_lit () {
    if (new_clauses.empty()) 
        return 0;
    else {
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
}
