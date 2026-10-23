#include <iostream>
#include <string>
#include <stack>
#include <stdexcept>
#include <vector>
#include <set>

const char EPSILON = '\0';
const char ANY = 1; 

bool isOperator(char c) {
    return c == '|' || c == '*' || c == '+' || c == '?' || c == '(' || c == ')' || c == '&';
}

bool isQuantifier(char c) {
    return c == '*' || c == '+' || c == '?';
}

bool isLiteral(char c) {
    return !isOperator(c) || c == '.';
}

bool canEndExpression(char c) {
    return isLiteral(c) || c == ')' || c == '*' || c == '+' || c == '?';
}

bool canStartExpression(char c) {
    return isLiteral(c) || c == '(';
}

std::string addConcatenation(const std::string& regex) {
    if (regex.empty()) {
        throw std::runtime_error("Puste wyrazenie regularne");
    }
    if (isQuantifier(regex.front()) || regex.front() == '|' || regex.front() == '&') {
        throw std::runtime_error("Niepoprawny operator na poczatku wyrazenia");
    }
    if (regex.back() == '|' || regex.back() == '&') {
        throw std::runtime_error("Wiszacy operator binarny na koncu wyrazenia");
    }

    std::string result;
    result.reserve(regex.size() * 2);
    
    for (size_t i = 0; i < regex.size(); ++i) {
        char current = regex[i];
        result += current;

        if (i + 1 >= regex.size()) {
            continue;
        }

        char next = regex[i + 1];

            if (current == '(' && next == ')') {
            throw std::runtime_error("Puste nawiasy '()' sa niedozwolone");
        }
        if ((current == '(' || current == '|' || current == '&') && isQuantifier(next)) {
            throw std::runtime_error("Kwantyfikator (*, +, ?) bez poprzedzajacego go wyrazenia");
        }
        if ((current == '(' || current == '|' || current == '&') && (next == '|' || next == '&')) {
            throw std::runtime_error("Pusty czlon operatora binarnego");
        }
        if ((current == '|' || current == '&') && next == ')') {
            throw std::runtime_error("Wiszacy operator binarny przed nawiasem zamykajacym ')'");
        }
        if (isQuantifier(current) && isQuantifier(next)) {
            throw std::runtime_error("Wielokrotny kwantyfikator pod rzad (np. '**', '+*')");
        }

        if (canEndExpression(current) && canStartExpression(next)) {
            result += '&';
        }
    }

    return result;
}

int precedence(char op) {
    if (op == '|') return 1;
    if (op == '&') return 2;
    return 0;
}

std::string toPostfix(const std::string& regex) {
    std::string output = "";
    std::stack<char> operators;

    for (char c : regex) {
        if (isLiteral(c)) {
            output += c;
        }
        else if (isQuantifier(c)) {
            output += c;
        }
        else if (c == '(') {
            operators.push(c);
        }
        else if (c == ')') {
            while (!operators.empty() && operators.top() != '(') {
                output += operators.top();
                operators.pop();
            }
            if (operators.empty()) {
                throw std::runtime_error("Brak nawiasu otwierajacego '('");
            }
            operators.pop();
        }
        else if (c == '&' || c == '|') {
            while (!operators.empty() && operators.top() != '(' &&
                   precedence(operators.top()) >= precedence(c)) {
                output += operators.top();
                operators.pop();
            }
            operators.push(c);
        }
    }

    while (!operators.empty()) {
        if (operators.top() == '(') {
            throw std::runtime_error("Brak nawiasu zamykajacego ')'");
        }
        output += operators.top();
        operators.pop();
    }

    return output;
}

struct Transition {
    char sign;
    int target;
};

struct State {
    std::vector<Transition> transitions;
};

struct Fragment {
    int starter;
    int ender;
};

int newState(std::vector<State>& states) {
    int id = states.size();
    states.push_back(State{});
    return id;
}

Fragment makeLiteral(char c, std::vector<State>& states) {
    int starter = newState(states);
    int ender = newState(states);
    states[starter].transitions.push_back({c, ender});
    return {starter, ender};
}

Fragment makeAny(std::vector<State>& states) {
    int starter = newState(states);
    int ender = newState(states);
    states[starter].transitions.push_back({ANY, ender});
    return {starter, ender};
}

Fragment concatenate(Fragment a, Fragment b, std::vector<State>& states) {
    states[a.ender].transitions.push_back({EPSILON, b.starter});
    return {a.starter, b.ender};
}

Fragment alternative(Fragment a, Fragment b, std::vector<State>& states) {
    int starter = newState(states);
    int ender = newState(states);
    states[starter].transitions.push_back({EPSILON, a.starter});
    states[starter].transitions.push_back({EPSILON, b.starter});
    states[a.ender].transitions.push_back({EPSILON, ender});
    states[b.ender].transitions.push_back({EPSILON, ender});
    return {starter, ender};
}

Fragment kleenStar(Fragment a, std::vector<State>& states) {
    int starter = newState(states);
    int ender = newState(states);
    states[starter].transitions.push_back({EPSILON, a.starter});
    states[starter].transitions.push_back({EPSILON, ender});
    states[a.ender].transitions.push_back({EPSILON, a.starter});
    states[a.ender].transitions.push_back({EPSILON, ender});
    return {starter, ender};
}

Fragment plus(Fragment a, std::vector<State>& states) {
    int starter = newState(states);
    int ender = newState(states);
    states[starter].transitions.push_back({EPSILON, a.starter});
    states[a.ender].transitions.push_back({EPSILON, a.starter});
    states[a.ender].transitions.push_back({EPSILON, ender});
    return {starter, ender};
}

Fragment optional(Fragment a, std::vector<State>& states) {
    int starter = newState(states);
    int ender = newState(states);
    states[starter].transitions.push_back({EPSILON, a.starter});
    states[starter].transitions.push_back({EPSILON, ender});
    states[a.ender].transitions.push_back({EPSILON, ender});
    return {starter, ender};
}

Fragment buildNFA(const std::string& postfix, std::vector<State>& states) {
    std::stack<Fragment> stack;

    for (char c : postfix) {
        if (c == '.') {
            stack.push(makeAny(states));
        } else if (isLiteral(c)) {
            stack.push(makeLiteral(c, states));
        } else if (c == '&') {
            if (stack.size() < 2) throw std::runtime_error("Za malo argumentow dla operatora '&'");
            auto b = stack.top(); stack.pop();
            auto a = stack.top(); stack.pop();
            stack.push(concatenate(a, b, states));
        } else if (c == '|') {
            if (stack.size() < 2) throw std::runtime_error("Za malo argumentow dla operatora '|'");
            auto b = stack.top(); stack.pop();
            auto a = stack.top(); stack.pop();
            stack.push(alternative(a, b, states));
        } else if (c == '*') {
            if (stack.empty()) throw std::runtime_error("Brak argumentu dla operatora '*'");
            auto a = stack.top(); stack.pop();
            stack.push(kleenStar(a, states));
        } else if (c == '+') {
            if (stack.empty()) throw std::runtime_error("Brak argumentu dla operatora '+'");
            auto a = stack.top(); stack.pop();
            stack.push(plus(a, states));
        } else if (c == '?') {
            if (stack.empty()) throw std::runtime_error("Brak argumentu dla operatora '?'");
            auto a = stack.top(); stack.pop();
            stack.push(optional(a, states));
        }
    }

    if (stack.size() != 1) {
        throw std::runtime_error("Niepoprawne wyrazenie regularne (bledna liczba fragmentow na stosie)");
    }

    return stack.top();
}

void printNFA(const std::vector<State>& states, Fragment nfa) {
    std::cout << "\n=== NFA (Start: " << nfa.starter << ", Akceptujacy: " << nfa.ender << ") ===\n";
    for (size_t i = 0; i < states.size(); ++i) {
        std::cout << "Stan " << i << ":\n";
        if (states[i].transitions.empty()) {
            std::cout << "  (brak wyjsc)\n";
        }
        for (const auto& t : states[i].transitions) {
            std::string label;
            if (t.sign == EPSILON) label = "eps";
            else if (t.sign == ANY) label = "ANY";
            else label = std::string(1, t.sign);

            std::cout << "  --" << label << "--> Stan " << t.target << "\n";
        }
    }
}

std::set<int> epsilonClosure(const std::set<int>& input, const std::vector<State>& states) {
    std::set<int> result = input;
    std::stack<int> st;

    for (int s : input) {
        st.push(s);
    }

    while (!st.empty()) {
        int s = st.top();
        st.pop();

        for (const auto& tr : states[s].transitions) {
            if (tr.sign == EPSILON && result.count(tr.target) == 0) {
                result.insert(tr.target);
                st.push(tr.target);
            }
        }
    }
    return result;
}

std::set<int> move(const std::set<int>& current, char c, const std::vector<State>& states) {
    std::set<int> nextStates;

    for (int s : current) {
        for (const auto& tr : states[s].transitions) {
            if (tr.sign == c || tr.sign == ANY) {
                nextStates.insert(tr.target);
            }
        }
    }
    return epsilonClosure(nextStates, states);
}

bool matches(const std::string& text, const Fragment& nfa, const std::vector<State>& states) {
    std::set<int> current = epsilonClosure({nfa.starter}, states);

    for (char c : text) {
        current = move(current, c, states);
        if (current.empty()) {
            return false;
        }
    }

    return current.count(nfa.ender) > 0;
}

struct Match {
    int start;
    int end;
    std::string text;
};

int longestMatchFrom(const std::string& text, int start, const Fragment& nfa, const std::vector<State>& states) {
    std::set<int> current = epsilonClosure({nfa.starter}, states);
    int lastAccept = -1;

    if (current.count(nfa.ender)) {
        lastAccept = start - 1;
    }

    for (size_t i = start; i < text.size(); ++i) {
        current = move(current, text[i], states);
        if (current.empty()) {
            break;
        }
        if (current.count(nfa.ender)) {
            lastAccept = i;
        }
    }
    return lastAccept;
}

std::vector<Match> search(const std::string& text, const Fragment& nfa, const std::vector<State>& states) {
    std::vector<Match> results;
    int start = 0;

    while (start < static_cast<int>(text.size())) {
        int end = longestMatchFrom(text, start, nfa, states);
        if (end >= start) {
            results.push_back({
                start,
                end,
                text.substr(start, end - start + 1)
            });
            start = end + 1;
        } else {
            start++;
        }
    }
    return results;
}

void runMatchTest(const std::string& pattern, const std::string& text, bool expected) {
    std::string prepared = addConcatenation(pattern);
    std::string postfix = toPostfix(prepared);
    std::vector<State> states;
    Fragment nfa = buildNFA(postfix, states);
    bool actual = matches(text, nfa, states);

    std::cout << "Wzorzec: " << pattern << " | Tekst: \"" << text << "\" -> "
              << (actual ? "PASUJE" : "BRAK")
              << " [" << (actual == expected ? "OK" : "BLAD") << "]\n";
}

void runInvalidRegexTest(const std::string& badPattern, const std::string& description) {
    try {
        std::string prepared = addConcatenation(badPattern);
        std::string postfix = toPostfix(prepared);
        std::vector<State> states;
        Fragment nfa = buildNFA(postfix, states);
        std::cout << "[BLAD TDD] Wzorzec \"" << badPattern << "\" (" << description 
                  << ") przeszedl bez wyjatku!\n";
    } catch (const std::exception& e) {
        std::cout << "[OK - ZLAPANO] Wzorzec \"" << badPattern << "\" (" << description 
                  << ") -> Wyjatek: " << e.what() << "\n";
    }
}

int main() {
    std::cout << "========================================\n";
    std::cout << "   TESTY JEDNOSTKOWE DOPASOWANIA (MATCH)\n";
    std::cout << "========================================\n";

    runMatchTest("a(b|c)*d", "ad", true);
    runMatchTest("a(b|c)*d", "abd", true);
    runMatchTest("a(b|c)*d", "abccbd", true);
    runMatchTest("a(b|c)*d", "ab", false);

    runMatchTest("a+b", "ab", true);
    runMatchTest("a+b", "aaaab", true);
    runMatchTest("a+b", "b", false);

    runMatchTest("a?b", "b", true);
    runMatchTest("a?b", "ab", true);
    runMatchTest("a?b", "aab", false);

    runMatchTest("a.c", "abc", true);
    runMatchTest("a.c", "a7c", true);
    runMatchTest("a.c", "ac", false);

    runMatchTest("(ab|cd)+", "abcd", true);
    runMatchTest("(ab|cd)+", "ababcd", true);
    runMatchTest("(ab|cd)+", "a", false);

    std::cout << "\n========================================\n";
    std::cout << "   TEST WYSZUKIWANIA W TEKSCIE (SEARCH)\n";
    std::cout << "========================================\n";

    std::string searchPattern = "ab+c";
    std::string searchText = "xxabbbczz_abc_ac_abbc";
    
    std::string prepared = addConcatenation(searchPattern);
    std::string postfix = toPostfix(prepared);
    std::vector<State> states;
    Fragment nfa = buildNFA(postfix, states);

    auto matchesFound = search(searchText, nfa, states);

    std::cout << "Szukany wzorzec: " << searchPattern << "\n";
    std::cout << "Przeszukiwany tekst: \"" << searchText << "\"\n";
    std::cout << "Znalezione wystapienia:\n";
    for (const auto& m : matchesFound) {
        std::cout << "  Pozycja [" << m.start << ".." << m.end << "]: \"" << m.text << "\"\n";
    }

    std::cout << "\n========================================\n";
    std::cout << "   TESTY: BLEDNE WYRAZENIA (NEGATIVE)\n";
    std::cout << "========================================\n";

    runInvalidRegexTest("", "Pusty wzorzec");
    runInvalidRegexTest("()", "Puste nawiasy");
    runInvalidRegexTest("a()b", "Puste nawiasy w srodku");
    runInvalidRegexTest("*a", "Gwiazdka na poczatku");
    runInvalidRegexTest("+a", "Plus na poczatku");
    runInvalidRegexTest("?a", "Znak zapytania na poczatku");
    runInvalidRegexTest("a(*b)", "Kwantyfikator zaraz po '('");
    runInvalidRegexTest("a|*b", "Kwantyfikator zaraz po '|'");
    runInvalidRegexTest("a|", "Wiszaca alternatywa na koncu");
    runInvalidRegexTest("|a", "Wiszaca alternatywa na poczatku");
    runInvalidRegexTest("a||b", "Podwojna alternatywa");
    runInvalidRegexTest("(a|b", "Brak nawiasu zamykajacego");
    runInvalidRegexTest("a|b)", "Brak nawiasu otwierajacego");
    runInvalidRegexTest("a**", "Podwojny kwantyfikator");
    runMatchTest("a.*b", "ab", true);       // 0 dowolnych znakow miedzy a i b
    runMatchTest("a.*b", "axxxb", true);    // wiele dowolnych znakow
    runMatchTest("a.+b", "ab", false);      // .+ wymaga min. 1 znaku
    runMatchTest("a.+b", "axb", true);      // 1 dowolny znak
    runMatchTest("a.?b", "ab", true);       // 0 znakow
    runMatchTest("a.?b", "axb", true);      // 1 znak
    runMatchTest("a.?b", "axxb", false);    // 2 znaki -> za duzo dla ?
    runMatchTest("((.*|A*)|A)*", "AAAAAAAAAAAAAAAAAAAAAAAAAAAA", true);    // 2 znaki -> za duzo dla ?
    runMatchTest("a*", "", true);
    runMatchTest("a?", "", true);
    runMatchTest("a+", "", false);

    return 0;
}   