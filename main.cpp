#include <iostream>
#include <vector>
#include <map>
#include <numeric>
#include <algorithm>
#include <queue>
#include <functional>
#include <string>
#include <chrono>
#include <cctype>
#include <cmath>

using namespace std;

/******************** constants ***************************************/

vector<char> nums = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};  // column indices
vector<char> chars = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I'}; // row indices

// which optimizations to use
bool use_ac3 = true;
bool use_mrv = true;
bool use_lcv = true;
bool use_fc = true;

/******************** overloaded print functions **********************/

void print(vector<string> &v)
{
    for (string a : v)
        cout << a << " ";
    cout << '\n';
}

void print(vector<pair<string, string>> v)
{
    for (auto p : v)
    {
        cout << "(" << p.first << "," << p.second << ")"
             << " , ";
    }
    cout << '\n';
}

template <typename T>
void print(map<string, vector<T>> &m)
{
    for (auto itr = m.begin(); itr != m.end(); itr++)
    {
        cout << itr->first << " - ";
        for (int i = 0; i < itr->second.size(); i++)
            cout << itr->second[i] << ",";
        cout << '\n';
    }
}

void print(map<string, int> m)
{
    for (auto x : m)
    {
        cout << x.first << " : " << x.second << '\n';
    }
}

void print(string &puzzle)
{
    for (int i = 0; i < 31; i++)
        cout << "-";
    cout << '\n';

    for (int i = 0; i < puzzle.size(); i++)
    {
        if (i % 9 == 0)
            cout << "| ";

        cout << puzzle[i];

        if (i % 9 == 8)
        {
            cout << " |\n";
            if (i % 27 == 26)
            {
                for (int i = 0; i < 31; i++)
                    cout << "-";
                cout << '\n';
            }
        }
        else if (i % 3 == 2)
            cout << " | ";
        else
            cout << "  ";
    }
}

/*********************** utility functions ********************************/

void join(vector<char> &a, vector<char> &b, vector<string> &joined)
{
    for (char x : a)
    {
        string s1 = string(1, x);
        for (char y : b)
        {
            joined.push_back(s1 + y);
        }
    }
}

/**************  SUDOKU GAME PARAMETERS INITIALIZATION ************************/

// creates variables A1,A2,A3,....,I7,I8,I9 denoting each cell of sudoku grid
void create_variables(vector<char> &chars, vector<char> &nums, vector<string> &variables)
{
    join(chars, nums, variables);
}

// gets possible integers that can be assigned to each variable
void get_domains(string &puzzle, vector<string> &variables, map<string, vector<int>> &domains)
{
    for (int i = 0; i < 81; i++)
    {
        if (puzzle[i] == '0')
        {
            domains[variables[i]] = vector<int>(9);
            iota(domains[variables[i]].begin(), domains[variables[i]].end(), 1); // fill vector with no's 1 to 9
        }
        else
        {
            domains[variables[i]] = vector<int>(1, puzzle[i] - '0');
        }
    }
}

// 'constraints' is a list of pairs (A,B) which denotes the constraint A!=B
void get_constraints(vector<char> &chars, vector<char> &nums, vector<pair<string, string>> &constraints)
{
    vector<vector<string>> rows, columns, boxes;

    for (char c : chars) // create list of rows
    {
        vector<char> t1 = {c};
        vector<string> t2;
        join(t1, nums, t2);
        // print(t2);
        rows.push_back(t2);
    }

    for (char n : nums) // create list of columns
    {
        vector<char> t1 = {n};
        vector<string> t2;
        join(chars, t1, t2);
        columns.push_back(t2);
    }

    vector<string> t1 = {"ABC", "DEF", "GHI"};
    vector<string> t2 = {"123", "456", "789"};

    for (string s1 : t1) // create box-wise lists
    {
        vector<char> v1(s1.begin(), s1.end());
        for (string s2 : t2)
        {
            vector<char> v2(s2.begin(), s2.end());
            vector<string> v3;

            join(v1, v2, v3);
            boxes.push_back(v3);
        }
    }

    vector<vector<string>> all_lists;
    all_lists.insert(all_lists.end(), rows.begin(), rows.end());
    all_lists.insert(all_lists.end(), columns.begin(), columns.end());
    all_lists.insert(all_lists.end(), boxes.begin(), boxes.end());

    for (vector<string> &v : all_lists)
    {
        for (int i = 0; i < v.size(); i++)
        {
            for (int j = 0; j < v.size(); j++)
            {
                if (j == i)
                    continue;
                pair<string, string> pr = make_pair(v[i], v[j]);
                if (find(constraints.begin(), constraints.end(), pr) == constraints.end())
                {
                    constraints.push_back(pr);
                }
            }
        }
    }
}

// 'neighbours' is a mapping from each variable to a list of variables such that the key variable is constrained by each of its value variables
void get_neighbours(vector<string> &variables, vector<pair<string, string>> &constraints, map<string, vector<string>> &neighbours)
{
    for (string var : variables)
    {
        for (auto p : constraints)
        {
            if (var == p.first)
                neighbours[var].push_back(p.second);
        }
    }
}

// have we found a solution to the sudoku?
bool is_solved(map<string, vector<int>> m)
{
    for (auto itr = m.begin(); itr != m.end(); itr++)
    {
        if (itr->second.size() > 1)
            return false;
    }
    return true;
}

// 'assignments' is a mapping from each variable to a single value that has been asssigned to it
void get_assignments(map<string, vector<int>> &domains, map<string, int> &assignments) // check if some variables can be assigned values
{
    for (auto x : domains)
    {
        if (x.second.size() == 1) // if domain has only value that must be assigned to that variabel
        {
            assignments[x.first] = x.second[0];
        }
    }
}

/**************     AC-3 IMPLEMENTATION **************************/

bool check_constraint(string a, string b, map<string, vector<int>> &domains)
{
    bool domain_for_a_has_changed = false;
    for (int xi : domains[a])
    {
        bool xj_exists = false; // does xj exists such that xi!=xj is true
        for (int xj : domains[b])
        {
            if (xi != xj)
            {
                xj_exists = true;
                break;
            }
        }
        if (!xj_exists)
        {
            domain_for_a_has_changed = true;
            domains[a].erase(remove(domains[a].begin(), domains[a].end(), xi), domains[a].end()); // remove xi from domains[a]
        }
    }

    return domain_for_a_has_changed;
}

void ac3(vector<pair<string, string>> &constraints, map<string, vector<int>> &domains, map<string, vector<string>> &neighbours, bool &solution_exists)
{
    deque<pair<string, string>> agenda(deque<pair<string, string>>(constraints.begin(), constraints.end())); // create agenda by copying all constraints initially

    string a, b;

    while (!agenda.empty())
    {
        a = agenda.front().first, b = agenda.front().second;
        agenda.pop_front();

        bool domain_for_a_has_changed = check_constraint(a, b, domains); // pruning domains using constraint a!=b

        if (domain_for_a_has_changed)
        {
            if (domains[a].empty())
            {
                solution_exists = false;
                return;
            }
            for (string c : neighbours[a])
            {
                if (find(agenda.begin(), agenda.end(), make_pair(c, a)) == agenda.end())
                {
                    agenda.push_back(make_pair(c, a));
                }
            }
        }
    }
    solution_exists = true;
}

/**************************** BACKTRACKING *********************************/

string select_unassigned_variable(map<string, vector<int>> &domains, map<string, int> &assignments, map<string, vector<string>> &neighbours) // return variable with Minimum Remaining Values (MRV)
{
    string req;
    int remaining_values = INT16_MAX;

    for (auto x : domains)
    {
        if (assignments.find(x.first) != assignments.end())
            continue; // ignore if already assigned

        if (!use_mrv)
            return x.first;

        if (x.second.size() < remaining_values)
            req = x.first, remaining_values = x.second.size(); // MRV
    }
    return req;
}

static inline int get_conflicts(int &val, string &var, map<string, vector<int>> &domains, map<string, vector<string>> &neighbours)
{
    int count = 0;
    for (string xk : neighbours[var])
    {
        if (find(domains[xk].begin(), domains[xk].end(), val) != domains[xk].end())
            count += 1;
    }
    return count;
}

static inline bool least_constraining_order(int i1, int i2, string &var, map<string, vector<int>> &domains, map<string, vector<string>> &neighbours)
{
    return get_conflicts(i1, var, domains, neighbours) < get_conflicts(i2, var, domains, neighbours);
}

// checks if neighbour of to-be-assigned var, already has been assigned that variable...in that case we can't assign var this value
bool can_be_assigned(int &val, string &var, map<string, int> &assignments, map<string, vector<string>> &neighbours)
{
    for (auto x : assignments)
    {
        if (x.second == val && find(neighbours[var].begin(), neighbours[var].end(), x.first) != neighbours[var].end())
            return false;
    }
    return true;
}

void forward_check(int &val, string &var, map<string, vector<int>> &domains, map<string, int> &assignments, map<string, vector<string>> &neighbours, map<string, vector<pair<string, int>>> &pruned)
{
    for (auto x : neighbours[var])
    {
        if (find(domains[x].begin(), domains[x].end(), val) != domains[x].end()) // if neighbour also has same value 'val' then remove it from its domain
        {
            domains[x].erase(remove(domains[x].begin(), domains[x].end(), val), domains[x].end()); // remove val from domains[x]
            pruned[var].push_back(make_pair(x, val));
        }
    }
}

void backtrack(map<string, vector<int>> &domains, map<string, int> &assignments, map<string, vector<string>> &neighbours, bool &solution_exists, map<string, vector<pair<string, int>>> &pruned)
{
    if (assignments.size() == domains.size())
    {
        solution_exists = true;
        return;
    }

    string var = select_unassigned_variable(domains, assignments, neighbours); // uses MRV and DH

    // sort in order of least constraining values
    if(use_lcv)
        sort(domains[var].begin(), domains[var].end(), bind(least_constraining_order, placeholders::_1, placeholders::_2, var, domains, neighbours));

   
    for (int val : domains[var])
    {
        if (can_be_assigned(val, var, assignments, neighbours))
        {
            assignments[var] = val;

            if (use_fc)
                forward_check(val, var, domains, assignments, neighbours, pruned);

            backtrack(domains, assignments, neighbours, solution_exists, pruned);

            if (solution_exists)
                return;

            assignments.erase(var);
            if (use_fc)
            {
                for (auto x : pruned[var])
                {
                    domains[x.first].push_back(x.second); // add back values to neighbours domains that were pruned when 'var' was assigned 'val'
                }
                pruned[var].clear();
            }
        }
    }
    solution_exists = false;
}

/********************** DRIVER ********************************/

int main()
{
    // row-wise input, 0 represents blank
    string easy_puzzle = "034 870 000 002 340 158 180 200 000 900 030 506 010 000 400 053 760 002 020 603 000 708 005 003 301 407 865";
    string medium_puzzle = "170 000 006 040 106 000 000 005 208 400 078 000 006 000 005 000 001 300 020 904 500 000 000 000 810 000 649";
    string hard_puzzle = "006 000 030 000 200 785 200 000 000 701 000 000 098 010 200 060 805 090 000 009 060 900 000 813 637 008 009";

    string puzzle;
    int choice;
    cout<<"Please choose\n0 : custom sudoku\n1 : example sudoku\n";
    cin>>choice;
    if(choice == 0)
    {
        cout<<"enter initial sudoku values as a single string (row-wise with 0 representing blank cell)\n";
        cout<<"example input - 034 870 000 002 340 158 180 200 000 900 030 506 010 000 400 053 760 002 020 603 000 708 005 003 301 407 865\n";
        getline(std::cin >> std::ws,puzzle);
    }
    else puzzle = easy_puzzle;

    auto start = chrono::high_resolution_clock::now();
    
    vector<string> variables;
    map<string, int> assignments;
    map<string, vector<int>> domains;
    map<string, vector<string>> neighbours;
    vector<pair<string, string>> constraints;
    map<string, vector<pair<string, int>>> pruned; // used in forward checking..key is a 'var' and value is a list of pairs (a,b) where 'a' is a variable that was pruned cause 'var' was assigned 'b'
    bool solution_exists;

    // remove whitespaces
    puzzle.erase(remove_if(puzzle.begin(), puzzle.end(), ::isspace), puzzle.end());
    if(puzzle.size()!=81)
    {
        cout<<"input error! Please make sure input has a total of 81 digits.\n";
        exit(0);
    }

    create_variables(chars, nums, variables);

    get_domains(puzzle, variables, domains);

    get_constraints(chars, nums, constraints);

    get_neighbours(variables, constraints, neighbours);

    auto stop = chrono::high_resolution_clock::now();

    auto duration = chrono::duration_cast<chrono::milliseconds>(stop - start);

    cout << "SUDOKU PARAMETRIZATION TIME : " << duration.count() << " ms" << '\n';

    /******** print original puzzle ****************/
    cout << "******** ORIGINAL ********* " << '\n';
    string puzzle_mod = puzzle;
    for(char &c:puzzle_mod) if(c=='0') c=' ';
    print(puzzle_mod);
    cout << '\n';
    /***********************************************/

    cout << "SOLVING SUDOKU....\n";

    start = chrono::high_resolution_clock::now();

    if (use_ac3)
    {
        ac3(constraints, domains, neighbours, solution_exists);
        if(!solution_exists)
        {
            cout<<"No solution!!\n";
            return 0;
        }
    }

    backtrack(domains, assignments, neighbours, solution_exists, pruned);

    stop = chrono::high_resolution_clock::now();

    duration = chrono::duration_cast<chrono::milliseconds>(stop - start);

    cout << "SUDOKU RUNNING TIME : " << duration.count() << " ms" << '\n';

    if (!solution_exists)
    {
        cout << "NO SOLUTION!!" << '\n';
        return 0;
    }

    for (auto x : assignments)
    {
        string var = x.first;

        int row = var[0] - 'A';
        int col = var[1] - '1';

        int idx = 9 * row + col;

        puzzle[idx] = '0' + x.second;
    }

    cout << "********* SOLVED **********" << '\n';
    print(puzzle);
    cout << '\n';

    // print(assignments);

    // print<string>(neighbours);

    // print(constraints);

    // print(variables);

    // print(puzzle);

}
