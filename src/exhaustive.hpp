#include "internal.hpp"

#define l_False 0
#define l_True 1
#define l_Undef 2

class ExhaustiveSearch : CaDiCaL::ExternalPropagator {
    CaDiCaL::Solver * solver;
    std::vector<std::vector<int>> new_clauses;
    std::vector<std::vector<int>> current_trail;
    int num_assign = 0;
    char * assign;
    //bool * fixed;
    int n = 0;
    bool only_neg = false;
    long sol_count = 0;
    FILE * solfile;
#ifdef PRINT_CALLBACK_TIME
    double callback_time = 0;
#endif
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
};
