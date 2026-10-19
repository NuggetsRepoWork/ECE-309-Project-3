// src/tools/calculator_eval.cpp — YOU IMPLEMENT (spec §4.6).
//
// calc_evaluate: evaluate a well-formed token list with two stacks
// (one for numbers, one for operators and '('). See spec §4.6 and the
// worked trace in Appendix C. std::stack or std::vector are both fine.

#include "tools/calculator.h"
#include <stack>

namespace {
    using Kind = CalcToken::Kind;

    int precedence(Kind kind) {
        switch (kind) {
        case Kind::Plus:
        case Kind::Minus:
            return 1;
        case Kind::Times:
        case Kind::Divide:
            return 2;
        case Kind::Number:
        case Kind::LParen:
        case Kind::RParen:
            return 0;
        }
        return 0;
    }

    bool is_operator(Kind kind) {
        return kind == Kind::Plus || kind == Kind::Minus ||
            kind == Kind::Times || kind == Kind::Divide;
    }

    bool fail(std::string* error, const char* message) {
        if (error != nullptr) {
            *error = message;
        }
        return false;
    }

    bool apply_top(std::stack<double>& values, std::stack<Kind>& operators,
        std::string* error) {
        if (operators.empty() || !is_operator(operators.top())) {
            return fail(error, "invalid expression");
        }
        if (values.size() < 2) {
            return fail(error, "invalid expression");
        }

        const Kind op = operators.top();
        operators.pop();

        const double right = values.top();
        values.pop();
        const double left = values.top();
        values.pop();

        double result = 0.0;
        switch (op) {
        case Kind::Plus:
            result = left + right;
            break;
        case Kind::Minus:
            result = left - right;
            break;
        case Kind::Times:
            result = left * right;
            break;
        case Kind::Divide:
            if (right == 0.0) {
                return fail(error, "division by zero");
            }
            result = left / right;
            break;
        case Kind::Number:
        case Kind::LParen:
        case Kind::RParen:
            return fail(error, "invalid expression");
        }

        values.push(result);
        return true;
    }
}  // namespace

bool calc_evaluate(const std::vector<CalcToken>& tokens, double* out,
    std::string* error) {
    if (error != nullptr) {
        error->clear();
    }
    if (out == nullptr) {
        return fail(error, "invalid output pointer");
    }
    if (tokens.empty()) {
        return fail(error, "empty expression");
    }

    std::stack<double> values;
    std::stack<Kind> operators;

    for (const CalcToken& token : tokens) {
        switch (token.kind) {
        case Kind::Number:
            values.push(token.value);
            break;

        case Kind::LParen:
            operators.push(Kind::LParen);
            break;

        case Kind::RParen:
            while (!operators.empty() && operators.top() != Kind::LParen) {
                if (!apply_top(values, operators, error)) {
                    return false;
                }
            }
            if (operators.empty()) {
                return fail(error, "invalid expression");
            }
            operators.pop();  // discard the matching '('
            break;

        case Kind::Plus:
        case Kind::Minus:
        case Kind::Times:
        case Kind::Divide:
            while (!operators.empty() && operators.top() != Kind::LParen &&
                precedence(operators.top()) >= precedence(token.kind)) {
                if (!apply_top(values, operators, error)) {
                    return false;
                }
            }
            operators.push(token.kind);
            break;
        }
    }

    while (!operators.empty()) {
        if (operators.top() == Kind::LParen || operators.top() == Kind::RParen) {
            return fail(error, "invalid expression");
        }
        if (!apply_top(values, operators, error)) {
            return false;
        }
    }

    if (values.size() != 1) {
        return fail(error, "invalid expression");
    }

    *out = values.top();
    return true;
}