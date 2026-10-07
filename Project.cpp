// ===== STANDARD C++ HEADER FILES =====
// iostream  -> console input/output (cout, getline, etc.)
// string    -> stores program text and other text values
// vector    -> dynamic list used to store DFA-generated tokens
// stack     -> LIFO stack used by the PDA for opening brackets
// cctype    -> character tests such as isalpha() and isdigit()
// sstream   -> string-stream utilities (included as a utility header)
// algorithm -> general C++ utility algorithms (not central to the models)
#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <cctype>
#include <sstream>
#include <algorithm>

using namespace std;

/*
    MINI PROGRAMMING LANGUAGE SYNTAX CHECKER
    ----------------------------------------
    One-file C++ implementation of the project.

    Models of Computation:
    1. DFA -> lexical analysis / token recognition
    2. CFG -> statement and expression structure
    3. PDA -> matching nested brackets using a stack

    Supported examples:
        x = 10;
        x = x + 5;
        if (x > 5) { x = x + 1; }
        if (x > 5) { x = 1; } else { x = 2; }
        while (x > 0) { x = x - 1; }

    This is an educational mini-language, not a full compiler.
*/

// ===== TOKEN TYPES =====
// The DFA classifies pieces of the input into these categories.
// The CFG parser later uses these token labels.
enum class TokenType {
    IDENTIFIER,
    NUMBER,
    KEYWORD,
    OPERATOR,
    BRACKET,
    SEMICOLON,
    INVALID,
    END
};

// ===== TOKEN DATA =====
// Each token stores its type, actual text (lexeme), and position.
struct Token {
    TokenType type;
    string lexeme;
    int line;
    int column;
};

string typeName(TokenType type) {
    switch (type) {
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::KEYWORD: return "KEYWORD";
        case TokenType::OPERATOR: return "OPERATOR";
        case TokenType::BRACKET: return "BRACKET";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::INVALID: return "INVALID";
        default: return "END";
    }
}

bool isKeyword(const string& s) {
    return s == "if" || s == "else" || s == "while";
}

bool isOpening(char c) {
    return c == '(' || c == '{' || c == '[';
}

bool isClosing(char c) {
    return c == ')' || c == '}' || c == ']';
}

bool matches(char open, char close) {
    return (open == '(' && close == ')') ||
           (open == '{' && close == '}') ||
           (open == '[' && close == ']');
}

/* =========================================================
   DFA MODULE
   =========================================================
   DFA = Deterministic Finite Automaton.
   Purpose: lexical analysis / token recognition.

   q0 = start
   q1 = identifier / keyword
   q2 = number
   q3 = operator
   q4 = bracket
   ========================================================= */

class DFA {
public:
    // Main DFA function: scans the source from left to right
    // and converts raw characters into labelled tokens.
    vector<Token> tokenize(const string& source, bool showSteps = false) {
        vector<Token> tokens;

        int line = 1;
        int column = 1;
        size_t i = 0;

        if (showSteps) {
            cout << "\n------------ DFA / LEXICAL ANALYSIS ------------\n";
            cout << "State q0 = start state\n";
            cout << "q1 = identifier, q2 = number, q3 = operator, q4 = bracket\n\n";
        }

        while (i < source.size()) {
            char c = source[i];

            if (c == '\n') {
                i++;
                line++;
                column = 1;
                continue;
            }

            if (isspace(static_cast<unsigned char>(c))) {
                i++;
                column++;
                continue;
            }

            int startColumn = column;

            // ----- DFA: q0 -> q1 -----
            // A letter or underscore starts an identifier.
            // Letters/digits/underscore can then continue the word.
            // Finally, the word is classified as a keyword or identifier.
            if (isalpha(static_cast<unsigned char>(c)) || c == '_') {
                string word;
                while (i < source.size() &&
                       (isalnum(static_cast<unsigned char>(source[i])) ||
                        source[i] == '_')) {
                    word += source[i];
                    i++;
                    column++;
                }

                TokenType type = isKeyword(word)
                                 ? TokenType::KEYWORD
                                 : TokenType::IDENTIFIER;

                tokens.push_back({type, word, line, startColumn});

                if (showSteps)
                    cout << "q0 -> q1 : " << word << " -> "
                         << typeName(type) << "\n";

                continue;
            }

            // ----- DFA: q0 -> q2 -----
            // A digit starts a number and more digits may follow.
            // A pattern like 12x is rejected as an invalid token in our language.
            if (isdigit(static_cast<unsigned char>(c))) {
                string number;

                while (i < source.size() &&
                       isdigit(static_cast<unsigned char>(source[i]))) {
                    number += source[i];
                    i++;
                    column++;
                }

                // Example: 12x is rejected lexically.
                if (i < source.size() &&
                    (isalpha(static_cast<unsigned char>(source[i])) ||
                     source[i] == '_')) {

                    while (i < source.size() &&
                           (isalnum(static_cast<unsigned char>(source[i])) ||
                            source[i] == '_')) {
                        number += source[i];
                        i++;
                        column++;
                    }

                    tokens.push_back(
                        {TokenType::INVALID, number, line, startColumn});

                    if (showSteps)
                        cout << "q0 -> q2 -> INVALID : "
                             << number << "\n";
                } else {
                    tokens.push_back(
                        {TokenType::NUMBER, number, line, startColumn});

                    if (showSteps)
                        cout << "q0 -> q2 : " << number
                             << " -> NUMBER\n";
                }

                continue;
            }

            // ----- DFA: q0 -> q3 -----
            // Recognize arithmetic, assignment, and comparison operators.
            // Single-character: + - * / = < >
            // Two-character: == != <= >=
            if (c == '+' || c == '-' || c == '*' || c == '/' ||
                c == '=' || c == '<' || c == '>') {

                string op(1, c);

                if (i + 1 < source.size()) {
                    string two = source.substr(i, 2);
                    if (two == "==" || two == "!=" ||
                        two == "<=" || two == ">=") {
                        op = two;
                        i += 2;
                        column += 2;
                        tokens.push_back(
                            {TokenType::OPERATOR, op, line, startColumn});

                        if (showSteps)
                            cout << "q0 -> q3 : " << op
                                 << " -> OPERATOR\n";
                        continue;
                    }
                }

                i++;
                column++;

                tokens.push_back(
                    {TokenType::OPERATOR, op, line, startColumn});

                if (showSteps)
                    cout << "q0 -> q3 : " << op
                         << " -> OPERATOR\n";

                continue;
            }

            // ----- DFA: q0 -> q4 -----
            // Recognize the bracket characters: ( ) { } [ ].
            // DFA only identifies them as bracket tokens; PDA checks matching.
            if (isOpening(c) || isClosing(c)) {
                string bracket(1, c);
                tokens.push_back(
                    {TokenType::BRACKET, bracket, line, startColumn});

                if (showSteps)
                    cout << "q0 -> q4 : " << bracket
                         << " -> BRACKET\n";

                i++;
                column++;
                continue;
            }

            // ----- SEMICOLON -----
            // Semicolon terminates an assignment statement in our mini-language.
            if (c == ';') {
                tokens.push_back(
                    {TokenType::SEMICOLON, ";", line, startColumn});

                if (showSteps)
                    cout << "q0 -> ACCEPT : ; -> SEMICOLON\n";

                i++;
                column++;
                continue;
            }

            // ----- INVALID SYMBOL -----
            // Any unsupported character is marked INVALID.
            // Example: # is invalid because # is not in our language rules.
            string invalid(1, c);
            tokens.push_back(
                {TokenType::INVALID, invalid, line, startColumn});

            if (showSteps)
                cout << "q0 -> INVALID : " << invalid << "\n";

            i++;
            column++;
        }

        return tokens;
    }

    // Checks whether tokenization produced any INVALID token.
    // An INVALID token means the DFA rejects the input lexically.
    bool validTokens(const vector<Token>& tokens) {
        for (const auto& t : tokens)
            if (t.type == TokenType::INVALID)
                return false;
        return true;
    }

    void displayTokens(const vector<Token>& tokens) {
        cout << "\nToken Table\n";
        cout << "---------------------------------------------\n";
        cout << "Lexeme\t\tType\t\tPosition\n";
        cout << "---------------------------------------------\n";

        for (const auto& t : tokens) {
            cout << t.lexeme << "\t\t"
                 << typeName(t.type) << "\t\t"
                 << t.line << ":" << t.column << "\n";
        }

        cout << "---------------------------------------------\n";
    }
};

/* =========================================================
   PDA MODULE
   =========================================================
   PDA = Pushdown Automaton.
   Purpose: match and correctly nest brackets.
   The PDA uses a STACK as its extra memory.
   Opening bracket -> PUSH
   Matching closing bracket -> POP
   Mismatch / unfinished stack -> REJECT
   Empty stack at the end -> ACCEPT
   ========================================================= */

class PDA {
public:
    // Main PDA function. The stack remembers opening brackets.
    bool checkBrackets(const string& source, bool showSteps = false) {
        stack<char> st; // PDA stack
        int line = 1;

        if (showSteps)
            cout << "\n------------ PDA / BRACKET ANALYSIS ------------\n";

        for (char c : source) {
            if (c == '\n') {
                line++;
                continue;
            }

            // Opening bracket -> PUSH it onto the PDA stack.
            if (isOpening(c)) {
                st.push(c);

                if (showSteps)
                    cout << "Line " << line
                         << ": PUSH '" << c << "'\n";
            }
            // Closing bracket -> compare it with the top of the stack.
            // If it matches, POP the opening bracket.
            else if (isClosing(c)) {
                if (st.empty()) {
                    cout << "PDA ERROR: Unexpected closing bracket '"
                         << c << "' on line " << line << ".\n";
                    return false;
                }

                char top = st.top();

                if (!matches(top, c)) {
                    cout << "PDA ERROR: '" << c
                         << "' does not match stack top '"
                         << top << "' on line " << line << ".\n";
                    return false;
                }

                st.pop();

                if (showSteps)
                    cout << "Line " << line
                         << ": POP '" << top
                         << "' using '" << c << "'\n";
            }
        }

        // A non-empty stack means an opening bracket was never closed.
        if (!st.empty()) {
            cout << "PDA ERROR: Unmatched opening bracket '"
                 << st.top() << "'.\n";
            return false;
        }

        if (showSteps)
            cout << "PDA RESULT: ACCEPTED - Stack is empty.\n";

        return true;
    }
};

/* =========================================================
   CFG MODULE
   =========================================================
   CFG = Context-Free Grammar.
   Purpose: check statement and expression structure.
   In this C++ program the CFG is implemented using parsing
   functions such as parseExpression(), parseAssignment(),
   parseConditional(), and parseWhile().
   ========================================================= */

class CFGParser {
private:
    vector<Token> tokens;
    size_t pos = 0;
    string errorMessage;

    bool isLexeme(const string& s) const {
        return pos < tokens.size() && tokens[pos].lexeme == s;
    }

    bool isType(TokenType type) const {
        return pos < tokens.size() && tokens[pos].type == type;
    }

    bool consume(const string& s) {
        if (isLexeme(s)) {
            pos++;
            return true;
        }

        if (pos < tokens.size())
            errorMessage = "Expected '" + s +
                           "' but found '" + tokens[pos].lexeme + "'.";
        else
            errorMessage = "Expected '" + s + "' but reached end of input.";

        return false;
    }

    bool parseIdentifier() {
        if (isType(TokenType::IDENTIFIER)) {
            pos++;
            return true;
        }

        if (pos < tokens.size())
            errorMessage = "Expected an identifier, found '" +
                           tokens[pos].lexeme + "'.";
        else
            errorMessage = "Expected an identifier.";

        return false;
    }

    bool parseNumberOrIdentifier() {
        if (isType(TokenType::IDENTIFIER) ||
            isType(TokenType::NUMBER)) {
            pos++;
            return true;
        }

        errorMessage = "Expected an identifier or number.";
        return false;
    }

    // CFG Expression rule: an expression can start with an
    // identifier/number and can continue with supported operators.
    bool parseExpression() {
        if (!parseNumberOrIdentifier())
            return false;

        while (pos < tokens.size()) {
            string op = tokens[pos].lexeme;

            if (op == "+" || op == "-" || op == "*" || op == "/" ||
                op == ">" || op == "<" || op == ">=" || op == "<=" ||
                op == "==" || op == "!=") {

                pos++;

                if (!parseNumberOrIdentifier())
                    return false;
            } else {
                break;
            }
        }

        return true;
    }

    // ===== CFG PRODUCTION =====
    // <Assignment> -> <Identifier> = <Expression> ;
    bool parseAssignment(bool requireSemicolon = true) {
        if (!parseIdentifier())
            return false;

        if (!consume("="))
            return false;

        if (!parseExpression())
            return false;

        if (requireSemicolon && !consume(";"))
            return false;

        return true;
    }

    // Parses a block such as { x = 1; }.
    // Blocks may contain assignments, if-statements, and while-statements.
    bool parseBlock() {
        if (!consume("{"))
            return false;

        while (pos < tokens.size() && !isLexeme("}")) {
            if (isLexeme("if")) {
                if (!parseConditional())
                    return false;
            }
            else if (isLexeme("while")) {
                if (!parseWhile())
                    return false;
            }
            else {
                if (!parseAssignment())
                    return false;
            }
        }

        if (!consume("}"))
            return false;

        return true;
    }

    // ===== CFG PRODUCTION =====
    // <Conditional> -> if ( <Expression> ) { <Statement> }
    // The implementation also supports an optional else block.
    bool parseConditional() {
        if (!consume("if"))
            return false;

        if (!consume("("))
            return false;

        if (!parseExpression())
            return false;

        if (!consume(")"))
            return false;

        if (!parseBlock())
            return false;

        // Optional else
        if (pos < tokens.size() && isLexeme("else")) {
            pos++;

            if (!parseBlock())
                return false;
        }

        return true;
    }

    // ===== CFG PRODUCTION =====
    // <While> -> while ( <Expression> ) { <Statement> }
    bool parseWhile() {
        if (!consume("while"))
            return false;

        if (!consume("("))
            return false;

        if (!parseExpression())
            return false;

        if (!consume(")"))
            return false;

        if (!parseBlock())
            return false;

        return true;
    }

public:
    // Main CFG entry point. It receives the DFA token list
    // and checks whether the tokens follow the grammar.
    bool parse(const vector<Token>& inputTokens, bool showSteps = false) {
        tokens = inputTokens;
        pos = 0;
        errorMessage.clear();

        if (tokens.empty()) {
            errorMessage = "No program was entered.";
            return false;
        }

        if (showSteps)
            cout << "\n------------ CFG / SYNTAX ANALYSIS ------------\n";

        // Parse one or more top-level statements.
        while (pos < tokens.size()) {
            bool success = false;

            if (isLexeme("if"))
                success = parseConditional();
            else if (isLexeme("while"))
                success = parseWhile();
            else
                success = parseAssignment();

            if (!success) {
                if (showSteps)
                    cout << "CFG RESULT: REJECTED\n";
                return false;
            }
        }

        if (showSteps) {
            cout << "CFG RESULT: ACCEPTED\n";
            cout << "Grammar matched successfully.\n";
        }

        return true;
    }

    string getError() const {
        return errorMessage;
    }
};

/* =========================================================
   SYSTEM / USER INTERFACE
   =========================================================
   These functions are not separate automata. They handle
   user input, testing, language rules, and program control.
   ========================================================= */

string readProgram() {
    cout << "\nEnter your program.\n";
    cout << "Type END on a separate line when finished.\n\n";

    string program;
    string line;

    while (true) {
        getline(cin, line);

        if (line == "END")
            break;

        program += line + "\n";
    }

    return program;
}

void printBanner() {
    cout << "\n===============================================\n";
    cout << "     MINI PROGRAMMING LANGUAGE SYNTAX CHECKER\n";
    cout << "===============================================\n";
    cout << " Models of Computation: DFA + CFG + PDA\n";
    cout << "===============================================\n";
}

// ===== INTEGRATION =====
// This function connects the three models in this implementation:
// 1. DFA -> token / lexical validation
// 2. PDA -> bracket matching
// 3. CFG -> statement / syntax validation
// Only when all required checks pass do we report VALID SYNTAX.
// ===== QUICK VIVA MAP =====
// DFA  -> class DFA / tokenize()
// PDA  -> class PDA / checkBrackets() / stack<char>
// CFG  -> class CFGParser / parse(), parseExpression(),
//         parseAssignment(), parseConditional(), parseWhile()
// Integration -> completeCheck()
// User interface -> readProgram(), runTestCases(), main()
//
// Easy memory trick:
// DFA = identifies WHAT each piece is.
// CFG = checks if the pieces are in the RIGHT STRUCTURE.
// PDA = checks if brackets are PROPERLY NESTED.
// ================================================================

void completeCheck(const string& program) {
    DFA dfa;
    PDA pda;
    CFGParser cfg;

    vector<Token> tokens = dfa.tokenize(program, true);

    cout << "\n";

    // ----- STAGE 1: DFA -----
    // Convert raw source text into labelled tokens and reject invalid tokens.
    if (!dfa.validTokens(tokens)) {
        dfa.displayTokens(tokens);
        cout << "\n===============================================\n";
        cout << "FINAL RESULT: LEXICAL ERROR\n";
        cout << "The DFA rejected an invalid token.\n";
        cout << "===============================================\n";
        return;
    }

    cout << "DFA RESULT: ACCEPTED - All tokens are valid.\n";

    // ----- STAGE 2: PDA -----
    // Use the stack to check matching and nesting of brackets.
    if (!pda.checkBrackets(program, true)) {
        cout << "\n===============================================\n";
        cout << "FINAL RESULT: BRACKET ERROR\n";
        cout << "The PDA rejected the bracket structure.\n";
        cout << "===============================================\n";
        return;
    }

    // ----- STAGE 3: CFG -----
    // Check whether the token sequence follows the grammar.
    if (!cfg.parse(tokens, true)) {
        cout << "\nCFG ERROR: " << cfg.getError() << "\n";
        cout << "\n===============================================\n";
        cout << "FINAL RESULT: SYNTAX ERROR\n";
        cout << "The CFG rejected the statement structure.\n";
        cout << "===============================================\n";
        return;
    }

    cout << "\n===============================================\n";
    cout << "FINAL RESULT: VALID SYNTAX\n";
    cout << "DFA + PDA + CFG all accepted the program.\n";
    cout << "===============================================\n";
}

// Runs the predefined valid and invalid examples from the project.
void runTestCases() {
    vector<pair<string, string>> tests = {
        {"x = 10;", "Valid"},
        {"x = x + 5;", "Valid"},
        {"if (x > 5) { x = x + 1; }", "Valid"},
        {"x = ;", "Syntax Error"},
        {"if (x > 5 { x = x + 1; }", "Bracket Error"},
        {"12x = 5;", "Lexical Error"}
    };

    cout << "\n================ TEST CASES ================\n";

    for (size_t i = 0; i < tests.size(); i++) {
        cout << "\nTest " << i + 1 << ": " << tests[i].first << "\n";
        cout << "Expected: " << tests[i].second << "\n";
        completeCheck(tests[i].first);
    }
}

// Displays supported language features and the main CFG rules.
void showLanguageRules() {
    cout << "\n=============== LANGUAGE RULES ===============\n";
    cout << "Keywords: if, else, while\n";
    cout << "Identifiers: letter/underscore followed by letters, digits, or _\n";
    cout << "Numbers: one or more digits\n";
    cout << "Operators: + - * / = > < == != >= <=\n";
    cout << "Brackets: ( ) { } [ ]\n";
    cout << "Statements end with: ;\n\n";

    cout << "CFG examples:\n";
    cout << "<Statement> -> <Assignment> | <Conditional> | <While>\n";
    cout << "<Assignment> -> <Identifier> = <Expression> ;\n";
    cout << "<Conditional> -> if ( <Expression> ) { <Statement> }\n";
    cout << "<While> -> while ( <Expression> ) { <Statement> }\n";
    cout << "<Expression> -> <Identifier> | <Number> |\n";
    cout << "              <Expression> <Operator> <Expression>\n";
}

void showMenu() {
    cout << "\n==================== MENU ====================\n";
    cout << "1. Enter and check a program\n";
    cout << "2. Run project test cases\n";
    cout << "3. Show language rules and CFG\n";
    cout << "4. Exit\n";
    cout << "===============================================\n";
    cout << "Enter choice: ";
}

// ===== PROGRAM START =====
// main() controls the menu and calls the appropriate functions.
// It lets the user enter code, run tests, view rules, or exit.
int main() {
    while (true) {
        printBanner();
        showMenu();

        string choice;
        getline(cin, choice);

        if (choice == "1") {
            string program = readProgram();

            if (program.empty()) {
                cout << "\nNo program entered.\n";
                continue;
            }

            completeCheck(program);
        }
        else if (choice == "2") {
            runTestCases();
        }
        else if (choice == "3") {
            showLanguageRules();
        }
        else if (choice == "4") {
            cout << "\nThank you for using the Mini Programming Language Syntax Checker.\n";
            break;
        }
        else {
            cout << "\nInvalid choice. Please select 1-4.\n";
        }
    }

    return 0;
}