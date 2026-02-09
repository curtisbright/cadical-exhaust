#include "internal.hpp"
#include <deque>
#include <vector>

#define l_False 0
#define l_True 1
#define l_Undef 2

class ExhaustiveSearch : CaDiCaL::ExternalPropagator {
    CaDiCaL::Solver * solver;
    std::deque<std::vector<int>> clause_queue;  // Queue for blocking clauses
    std::vector<int> cur_clause;                 // Current clause being added
    size_t cur_pos = 0;                          // Position in current clause
    
    // Track assignments for observed variables
    std::vector<int> assignment;                 // Current assignment: signed literal or 0 for unassigned
    std::vector<int> assigned_at_level;          // Level at which each variable was assigned (-1 if unassigned)
    std::deque<int> assigned_count_per_level;    // Count of assigned observed variables at each level

    int assigned_count = 0;                      // Number of assigned observed variables
    bool partial_solution_found = false;         // Flag to prevent duplicate blocking
    
    int n = 0;
    bool only_neg = false;
    long sol_count = 0;
    
    FILE * solfile;
    
public:
    ExhaustiveSearch(CaDiCaL::Solver * s, int order, bool only_neg, FILE * solfile);
    ~ExhaustiveSearch ();
    void notify_assignment(const std::vector<int>& lits);
    void notify_new_decision_level ();
    void notify_backtrack (size_t new_level);
    bool cb_check_found_model (const std::vector<int> & model);
    bool cb_has_external_clause (bool& is_forgettable);
    int cb_add_external_clause_lit ();
    int cb_decide ();
    int cb_propagate ();
    int cb_add_reason_clause_lit (int plit);
    
private:
    void block_partial_solution();  // Block the current partial assignment
};
