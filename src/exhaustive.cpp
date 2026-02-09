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
    
    // The root level trail
    trail.push_back(std::vector<int>());
    
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
    // Track assignments of observed variables
    for (int lit : lits) {
        int var = abs(lit);
        if (var <= n) {  // Only track observed variables (1 to n)
            int idx = var - 1;
            if (assignment[idx] == 0) {  // Not yet assigned
                // Store the signed literal
                assignment[idx] = lit;
                assigned_count++;
                
                // Track this variable at the current level
                trail.back().push_back(var);
            }
        }
    }
    
    // Check if all observed variables are assigned
    if (assigned_count == n && !partial_solution_found) {
        partial_solution_found = true;
        block_partial_solution();
    }
}

void ExhaustiveSearch::notify_new_decision_level () {
    trail.push_back(std::vector<int>());
}

void ExhaustiveSearch::notify_backtrack (size_t new_level) {
    // Unassign variables from backtracked levels
    while (trail.size() > new_level + 1) {
        // Unassign all variables from this level
        for (int var : trail.back()) {
            int idx = var - 1;
            assignment[idx] = 0;  // Mark as unassigned
            assigned_count--;
        }
        trail.pop_back();
    }
    
    // Reset partial solution flag if we're no longer complete
    if (assigned_count < n) {
        partial_solution_found = false;
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
        
        // Check that variable is assigned
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
    
    clause.push_back(0);
    clause_queue.push_back(std::move(clause));
}

bool ExhaustiveSearch::cb_check_found_model (const std::vector<int> & model) {
    // When using notify_assignment for blocking, we can just return false
    // to continue searching, as blocking is already handled
    (void)model;
    return false;
}

bool ExhaustiveSearch::cb_has_external_clause (bool& is_forgettable) {
    if (clause_queue.empty()) {
        return false;
    }
    
    is_forgettable = false;
    cur_clause = std::move(clause_queue.front());
    clause_queue.pop_front();
    cur_pos = 0;
    
    return true;
}

int ExhaustiveSearch::cb_add_external_clause_lit () {
    if (cur_pos >= cur_clause.size()) {
        return 0;
    }
    return cur_clause[cur_pos++];
}

int ExhaustiveSearch::cb_decide () { return 0; }
int ExhaustiveSearch::cb_propagate () { return 0; }
int ExhaustiveSearch::cb_add_reason_clause_lit (int plit) {
    (void)plit;
    return 0;
};
