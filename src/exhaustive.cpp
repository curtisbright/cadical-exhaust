#include "exhaustive.hpp"
#include <iostream>
#include <deque>

ExhaustiveSearch::ExhaustiveSearch(CaDiCaL::Solver * s, int order, bool only_neg, FILE * solfile) : solver(s) {
    if (order == 0) {
        // No order provided; run exhaustive search on all variables
        n = s->vars();
    } else {
        n = order;
    }
    this->only_neg = only_neg;
    this->solfile = solfile;
    
    // Initialize assignment tracking (0 means unassigned)
    assignment.assign(n, 0);
    assigned_at_level.assign(n, -1); 
    
    // The root level
    assigned_count_per_level.push_back(0);
    
    solver->connect_external_propagator(this);
    
    std::cout << "c Running exhaustive search on " << n << " variables" << std::endl;
    
    // Observe the variables used for exhaustive generation
    for (int i = 0; i < n; i++) {
        solver->add_observed_var(i+1);
    }
}

ExhaustiveSearch::~ExhaustiveSearch () {
    if (n != 0) {
        solver->disconnect_external_propagator ();
        std::cout << "c Number of solutions: " << sol_count << std::endl;
    }
}

void ExhaustiveSearch::notify_assignment(const std::vector<int>& lits) {
    int current_level = assigned_count_per_level.size() - 1;

    // Track assignments of observed variables
    for (int lit : lits) {
        int idx = abs(lit) - 1;
        if (assignment[idx] == 0) {  // Not yet assigned
            // Store the signed literal
            assignment[idx] = lit;
            assigned_at_level[idx] = current_level;
            assigned_count++;
            
            // Update count at current level
            assigned_count_per_level[current_level]++;
        }
    }
    
    // Check if all observed variables are assigned
    if (assigned_count == n && !partial_solution_found) {
        partial_solution_found = true;
        block_partial_solution();
    }
}

void ExhaustiveSearch::notify_new_decision_level () {
    assigned_count_per_level.push_back(0);
}

void ExhaustiveSearch::notify_backtrack (size_t new_level) {
    // Remove levels above new_level
    while (assigned_count_per_level.size() > new_level + 1) {
        // Mark variables assigned at this level as unassigned
        int count_at_level = assigned_count_per_level.back();
        assigned_count_per_level.pop_back();
        assigned_count -= count_at_level;
    }
    
    // Reset partial solution flag if we're no longer complete
    if (assigned_count < n) {
        partial_solution_found = false;
        
        // Unassign variables that were assigned above the new level
        for (int i = 0; i < n; i++) {
            if (assigned_at_level[i] > (int)new_level) {
                assignment[i] = 0;
                assigned_at_level[i] = -1;
            }
        }
    }
}

void ExhaustiveSearch::block_partial_solution() {
    sol_count++;

#ifdef VERBOSE
    if (!solfile)
      std::cout << "c New solution: ";
#endif
    
    std::vector<int> clause;
    clause.reserve(n + 1);
    
    // Create blocking clause from tracked assignments
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
        
        // Block this assignment by negating the literal
        if (lit > 0 || !only_neg) {
            clause.push_back(-lit);
        }
    }
    
#ifdef VERBOSE
    if(!solfile)
      std::cout << "0" << std::endl;
    else
      fprintf(solfile, "0\n");
#endif
#ifdef PRINT_PROCESS_TIME
    std::cout << "c Process time: " << CaDiCaL::absolute_process_time() << " s" << std::endl;
#endif
    
    new_clauses.push_back(clause);
    solver->add_trusted_clause(clause);
}

bool ExhaustiveSearch::cb_check_found_model (const std::vector<int> & model) {
    // When using notify_assignment for blocking, we can just return false
    // to continue searching, as blocking is already handled
    (void)model;
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
