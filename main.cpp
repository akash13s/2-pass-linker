#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <map>
#include <cctype>

using namespace std;

struct Token {
    char *text;
    int lineNumber;
    int lineOffset;

    Token(char *text, int lineNumber, int lineOffset) {
        this->text = text;
        this->lineNumber = lineNumber;
        this->lineOffset = lineOffset;
    }
};

struct Symbol {
    string text;
    int lineNumber;
    int lineOffset;
};

class Tokenizer {
private:
    char *currToken;
    int lineNumber;
    int lineOffset;
    string filename;
    ifstream inputFile;
    string currentLine;
    char *delimiters;

    // starts with 0
    int currentModuleNumber;
    int baseAddrOfCurrentModule;

    // module name, base address, module size
    map<int, pair<int, int>> moduleTable;

    // symbol, absolute address
    unordered_map<string, int> symbolTable;

    // memory location, [opcode, absolute address]
    int currentMemoryLocation;
    map<string, int> memoryMap;

    bool isDigit(char c) {
        return (c == '1' || c == '2' || c == '3' || c == '4' || c == '5' || c == '6' || c == '7' || c == '8' ||
                c == '9' || c == '0');
    }

    bool isValidSymbol(string text) {
        int len = text.length();
        if (len > 0 and len <= 16) {
            if (isalpha(text[0])) {
                for (int i = 1; i < len; i++) {
                    if (!isalnum(text[i])) {
                        return false;
                    }
                }
                return true;
            } else {
                return false;
            }
        }
        return false;
    }

    bool isValidAddressingMode(char c) {
        return (c == 'M' || c == 'A' || c == 'R' || c == 'I' || c == 'E');
    }

    void createSymbol(Symbol symbol, int val) {
        if (symbolTable.find(symbol.text) != symbolTable.end()) {
            cout << symbol.text << "=" << symbolTable[symbol.text] << " ";
            cout << "Error: This variable is multiple times defined; first value used" << endl;
            return;
        }
        int absAddress = baseAddrOfCurrentModule + val;
        symbolTable[symbol.text] = absAddress;
        cout << symbol.text << "=" << symbolTable[symbol.text] << endl;
    }

    Token getToken() {
        /*
         * Check if file is open or close
         * If file is open, check if the current token is null
         *
         * If current token is not null, continue reading from the current line till you get the next token
         * If new token found->good
         * Else call getToken again
         *
         * If current token is null, read a new line from the file
         * Read the file till you get a new token->good
         *
         * check the line offset logic once and test the get token function thoroughly
         */

        if (inputFile.is_open()) {
            if (currToken == nullptr) {
                while (currToken == nullptr && !inputFile.eof()) {
                    getline(inputFile, currentLine);
                    lineNumber++;
                    lineOffset = 0;
                    currToken = strtok(&currentLine[0], delimiters);
                    if (currToken != nullptr) {
                        lineOffset = currentLine.find(currToken, lineOffset);
                        Token token(currToken, lineNumber, lineOffset);
                        lineOffset += strlen(currToken);
                        return token;
                    }
                }
                if (inputFile.eof()) {
                    inputFile.close();
                    char *c = "";
                    Token token(c, -1, -1);
                    return token;
                }
            } else {
                currToken = strtok(nullptr, delimiters);
                if (currToken == nullptr) {
                    return getToken();
                }
                lineOffset = currentLine.find(currToken, lineOffset);
                Token token(currToken, lineNumber, lineOffset);
                lineOffset += strlen(currToken);
                return token;
            }
        } else {
            // throw appropriate error if input file is closed
        }
    }

    int readInt() {
        Token token = getToken();
        if (token.lineNumber != -1) {
            char *ptr = token.text;
            string num;
            while (*ptr != '\0') {
                char ch = *ptr;
                if (!isDigit(ch)) {
                    // throw error for invalid number
                    cout << "Error" << endl;
                    return 0;
                }
                num.push_back(ch);
                ptr++;
            }
            return stoi(num);
        }
        return -1;
    }

    Symbol readSymbol() {
        Token token = getToken();
        Symbol symbol;
        string text;
        if (token.lineNumber != -1 && strlen(token.text) > 0) {
            char *ptr = token.text;
            while (*ptr != '\0') {
                char ch = *ptr;
                text.push_back(ch);
                ptr++;
            }
        } else {
            // throw appropriate error
        }
        if (!isValidSymbol(text)) {
            // throw appropriate validation error
        }
        symbol.text = text;
        symbol.lineNumber = token.lineNumber;
        symbol.lineOffset = token.lineOffset;
        return symbol;
    }

    char readMARIE() {
        Token token = getToken();
        char addressingMode;
        if (token.lineNumber != -1) {
            char *ptr = token.text;
            string text;
            while (*ptr != '\0') {
                char ch = *ptr;
                text.push_back(ch);
                ptr++;
            }
            if (text.length() > 1) {
                // throw error
                cout << "Error" << endl;
            }
            if (!isValidAddressingMode(text[0])) {
                // throw appropriate error
                cout << "Error" << endl;
            }
            addressingMode = text[0];
            return addressingMode;
        } else {
            // throw appropriate error
            return '0';
        }
    }

    bool validateInstruction(int val) {
        int opcode = val / 1000;
        int operand = val % 1000;

        return opcode < 10 && operand < 512;
    }

    void pass1() {
        cout << "Symbol Table" << endl;
        while (1) {
            // definition list
            int defCount = readInt();
            if (defCount < 0) {
                return;
            }
            for (int i = 0; i < defCount; i++) {
                Symbol symbol = readSymbol();
                int val = readInt();
                createSymbol(symbol, val);
            }

            // use list
            int useCount = readInt();
            for (int i = 0; i < useCount; i++) {
                Symbol symbol = readSymbol();
            }

            // instruction list
            int instrCount = readInt();
            moduleTable[currentModuleNumber] = {baseAddrOfCurrentModule, instrCount};

            for (int i = 0; i < instrCount; i++) {
                char addrMode = readMARIE();
                int val = readInt();

//                if (!validateInstruction(val)) {
//                    // throw appropriate error
//                }
            }

            currentModuleNumber++;
            baseAddrOfCurrentModule += instrCount;
        }
    }

    void resolveExternalAddress(int operand, vector<Symbol> &useList, int memoryRef, string location) {
        if (operand >= useList.size() || operand < 0) {
            string t = useList[0].text;
            memoryRef += symbolTable[t];
            memoryMap[location] = memoryRef;
            cout << memoryMap[location] << " ";
            cout << "Error: External operand exceeds length of uselist; treated as relative=0" << endl;
            return ;
        }
        string token = useList[operand].text;
        if (symbolTable.find(token) != symbolTable.end()) {
            memoryRef += symbolTable[token];
            memoryMap[location] = memoryRef;
            cout << memoryMap[location] << endl;
        } else {
            memoryMap[location] = memoryRef;
            cout << memoryMap[location] << " ";
            cout << "Error: " << token << " is not defined; zero used" << endl;
        }
    }

    void resolveAbsoluteAddress(int operand, int memoryRef, string location) {
        if (operand >= 512) {
            memoryMap[location] = memoryRef;
            cout << memoryMap[location] << " ";
            cout << "Error: Absolute address exceeds machine size; zero used" << endl;
        } else {
            memoryRef += operand;
            memoryMap[location] = memoryRef;
            cout << memoryMap[location] << endl;
        }
    }

    void resolveImmediateAddress(int operand, int memoryRef, string location) {
        if (operand >= 900) {
            memoryRef += 999;
            memoryMap[location] = memoryRef;
            cout << memoryMap[location] << " ";
            cout << "Error: Illegal immediate operand; treated as 999" << endl;
        } else {
            memoryRef += operand;
            memoryMap[location] = memoryRef;
            cout << memoryMap[location] << endl;
        }
    }

    void resolveRelativeAddress(int operand, int instrCount, int memoryRef, string location) {
        if (operand >= instrCount) {
            memoryRef += baseAddrOfCurrentModule;
            memoryMap[location] = memoryRef;
            cout << memoryMap[location] << " ";
            cout << "Error: Relative address exceeds module size; relative zero used" << endl;
        } else {
            memoryRef += baseAddrOfCurrentModule + operand;
            memoryMap[location] = memoryRef;
            cout << memoryMap[location] << endl;
        }
    }

    void resolveModuleAddress(int operand, int memoryRef, string location) {
        if (moduleTable.find(operand) != moduleTable.end()) {
            memoryRef += moduleTable[operand].first;
            memoryMap[location] = memoryRef;
            cout << memoryMap[location] << endl;
        } else {
            memoryMap[location] = memoryRef;
            cout << memoryMap[location] << " ";
            cout << "Error: Illegal module operand ; treated as module=0" << endl;
        }
    }

    void resolveMemoryReference(char addrMode, int val, vector<Symbol> &useList, int instrCount) {
        int opcode = val / 1000;
        int operand = val % 1000;

        int memoryRef = opcode * 1000;

        string location = to_string(currentMemoryLocation);
        int zeros = 3 - location.length();

        for (int i = 0; i < zeros; i++) {
            location = '0' + location;
        }

        cout << location << ": ";

        if (opcode >= 10) {
            memoryMap[location] = 9999;
            cout << memoryMap[location] << " ";
            cout << "Error: Illegal opcode; treated as 9999" << endl;
            return;
        }

        switch (addrMode) {
            case 'M':
                resolveModuleAddress(operand, memoryRef, location);
                break;
            case 'A':
                resolveAbsoluteAddress(operand, memoryRef, location);
                break;
            case 'R':
                resolveRelativeAddress(operand, instrCount, memoryRef, location);
                break;
            case 'I':
                resolveImmediateAddress(operand, memoryRef, location);
                break;
            case 'E':
                resolveExternalAddress(operand, useList, memoryRef, location);
                break;
            default: // throw error in case control moves to default
                break;
        }

        currentMemoryLocation++;
    }

    void pass2() {
        cout << endl;
        cout << "Memory Map" << endl;
        while (1) {
            // definition list
            int defCount = readInt();
            if (defCount < 0) {
                return;
            }
            for (int i = 0; i < defCount; i++) {
                Symbol symbol = readSymbol();
                int val = readInt();
            }

            // use list
            int useCount = readInt();
            vector<Symbol> useList;

            for (int i = 0; i < useCount; i++) {
                Symbol symbol = readSymbol();
                useList.push_back(symbol);
            }

            // instruction list
            int instrCount = readInt();

            for (int i = 0; i < instrCount; i++) {
                char addrMode = readMARIE();
                int val = readInt();
                resolveMemoryReference(addrMode, val, useList, instrCount);
            }

            currentModuleNumber++;
            baseAddrOfCurrentModule += instrCount;
        }
    }

    void resetTokenizerParams() {
        this->inputFile.close();
        this->inputFile.open(filename);
        this->currToken = nullptr;
        this->lineNumber = -1;
        this->currentModuleNumber = 0;
        this->baseAddrOfCurrentModule = 0;
    }

    void __parseError(int errorCode, int lineNum, int lineOffset) {
        static char *errStr[] = {
                "NUM_EXPECTED", // Number expect, anything >= 2^30 is not a number either
                "SYM_EXPECTED", // Symbol Expected
                "MARIE_EXPECTED", // Addressing Expected which is M/A/R/I/E
                "SYM_TOO_LONG", // Symbol Name is too long
                "TOO_MANY_DEF_IN_MODULE", //>16
                "TOO_MANY_USE_IN_MODULE", // > 16
                "TOO_MANY_INSTR", // total num_instr exceeds memory size (512)
        };
        printf("Parse Error line %d offset %d: %s\n", lineNum, lineOffset, errStr[errorCode]);
    }

public:
    Tokenizer(string filename) {
        this->filename = filename;
        this->inputFile.open(filename);
        this->delimiters = " \n\t";
        this->currToken = nullptr;
        this->lineNumber = -1;
        this->baseAddrOfCurrentModule = 0;
        this->currentModuleNumber = 0;
        this->currentMemoryLocation = 0;
    }

    void parse() {
        pass1();
        resetTokenizerParams();
        pass2();
    }
};

int main() {
    Tokenizer *tokenizer = new Tokenizer("/Users/akashshrivastva/Documents/OS/Linker/input.txt");
    tokenizer->parse();
    return 0;
}
