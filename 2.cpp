#include <iostream>
#include <string>
#include <stack>
#include <stdexcept>
#include <vector>

bool isOperator(char c) {
    return c == '|' || c == '*' || c == '+' || c == '?' || c == '(' || c == ')' || c == '&';
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
    std::string result;
    result.reserve(regex.size() * 2);
    
    for (size_t i = 0; i < regex.size(); ++i) {
        char current = regex[i];
        result += current;

        if (i + 1 >= regex.size()) {
            continue;
        }

        char next = regex[i + 1];

        if (canEndExpression(current) && canStartExpression(next)) {
            result += '&';
        }
    }

    return result;
}


int precedence(char op) {
    if (op == '|') return 1;
    if (op == '&') return 2;
    if (op == '*' || op == '+' || op == '?') return 3;
    return 0;
}

std::string toPostfix(const std::string& regex) {
    std::string output = "";
    std::stack<char> operators;

    for (char c : regex) {
        // 1. Literały (wszystko co nie jest operatorem) idą prosto na wyjście
        if (!isOperator(c)) {
            output += c;
        }
        // 2. Nawias otwierający idzie bezwarunkowo na stos jako separator zasięgu
        else if (c == '(') {
            operators.push(c);
        }
        // 3. Nawias zamykający opróżnia stos aż do pasującego '('
        else if (c == ')') {
            while (!operators.empty() && operators.top() != '(') {
                output += operators.top();
                operators.pop();
            }
            if (operators.empty()) throw std::runtime_error("Niedopasowany nawias");
            operators.pop(); // usuń '('
        }
        // 4. Każdy operator (unarny i binarny)
        else {
            while (!operators.empty() && operators.top() != '(' &&
                   precedence(operators.top()) >= precedence(c)) {
                output += operators.top();
                operators.pop();
            }
            operators.push(c);
        }
    }

    while (!operators.empty()) {
        if (operators.top() == '(') throw std::runtime_error("Niedopasowany nawias");
        output += operators.top();
        operators.pop();
    }

    return output;
}

struct Transition
{
    char sign;
    int target;
};

struct State
{
    std::vector<Transition> transitions;
};

struct Fragment {
    int starter;
    int ender;
};

const char EPSILON = '\0';

int newState(std::vector<State>&states) {
    int id = states.size();
    states.push_back(State{});
    return id;
}

Fragment makeLiteral(char c, std::vector<State>states)
{
    int starter = newState(states);
    int ender = newState(states);
    states[starter].transitions.push_back({c,ender});
    return {starter, ender};
}

Fragment concatenate( Fragment a, Fragment b, std::vector<State>states)
{
    states[a.ender].transitions.push_back({EPSILON,b.starter});
    return {a.starter,b.ender};
}

Fragment alternative(Fragment a, Fragment b, std::vector<State>states)
{
    int starter=newState(states);
    int ender=newState(states);
    states[starter].transitions.push_back({EPSILON, a.starter});
    states[starter].transitions.push_back({EPSILON, b.starter});
    states[a.ender].transitions.push_back({EPSILON, ender});
    states[b.ender].transitions.push_back({EPSILON, ender});
    return{starter,ender};
}

Fragment kleenStar(Fragment a, std::vector<State>& states){
    int starter = newState(states);
    int ender = newState(states);
    states[starter].transitions.push_back({EPSILON, a.starter});
    states[starter].transitions.push_back({EPSILON, ender});
    states[a.ender].transitions.push_back({EPSILON, a.starter});
    states[a.ender].transitions.push_back({EPSILON, ender});
    return {starter,ender};
}

int main() {
    std::string testCases[] = {
        "abc",         // -> a&b&c
        "a(b)c",       // -> a&(b)&c
        "a*b",         // -> a*&b
        "a+bc",        // -> a+&b&c
        "a?b",         // -> a?&b
        "(a|b)c",      // -> (a|b)&c
        "a(b|c)*d",    // -> a&(b|c)*&d
        "a.b"          // -> a&.&b
    };
    std::string raw = "a(b|c)*d";
    std::string withConcat = addConcatenation(raw);
    std::string postfix = toPostfix(withConcat);

    std::cout << "Wejscie:   " << raw << std::endl;
    std::cout << "Po '&':    " << withConcat << std::endl;
    std::cout << "Postfix:   " << postfix << std::endl;
    std::cout << "=== Test Kroku 1: Wstawianie konkatenacji (&) ===" << std::endl;
    for (const auto& test : testCases) {
        std::cout << test << "  ==>  " << addConcatenation(test) << std::endl;
    }

    return 0;
}