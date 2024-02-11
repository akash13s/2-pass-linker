#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <map>
#include <cctype>
#include <algorithm>
#include <cstring>

using namespace std;

struct Token {
    char *text;
    int lineNumber;
    int lineOffset;
    bool isEOF;

    Token(char *text, int lineNumber, int lineOffset) {
        this->text = text;
        this->lineNumber = lineNumber;
        this->lineOffset = lineOffset;
        this->isEOF = false;
    }
};

struct Symbol {
    string text;
    int moduleNum;
    int relativeAddress;
    int lineNumber;
    int lineOffset;
};

struct ModuleTableEntry {
    int moduleNum;
    int baseAddress;
    int size;

    ModuleTableEntry() {}

    ModuleTableEntry(int moduleNum, int baseAddress, int size) {
        this->moduleNum = moduleNum;
        this->baseAddress = baseAddress;
        this->size = size;
    }
};

struct SymbolTableEntry {
    string text;
    int moduleNum;
    int absAddress;
    bool used;
    string errorMsg;

    SymbolTableEntry(string text, int moduleNum, int absAddress, bool used, string errorMsg) {
        this->text = text;
        this->moduleNum = moduleNum;
        this->absAddress = absAddress;
        this->used = used;
        this->errorMsg = errorMsg;
    }

    SymbolTableEntry() {}
};

class Tokenizer {
private:
    char *currToken;
    int lineNumber;
    int lineOffset;

    int prevValidTokenLineNum;
    int prevValidTokenStartLineOffset;
    int prevValidTokenEndLineOffset;

    string filename;
    ifstream inputFile;
    string currentLine;
    string previousLine;
    char *delimiters;

    // starts with 0
    int currentModuleNumber;
    int baseAddrOfCurrentModule;

    vector<ModuleTableEntry> moduleTable;

    vector<SymbolTableEntry> symbolTable;

    // memory location, [opcode, absolute address]
    int currentMemoryLocation;
    map<string, string> memoryMap;

    // unused symbols in module
    vector<string> unusedSymbols;

    bool isDigit(char c) {
        return (c == '1' || c == '2' || c == '3' || c == '4' || c == '5' || c == '6' || c == '7' || c == '8' ||
                c == '9' || c == '0');
    }

    string getMemoryRef(int m) {
        string s = to_string(m);
        int n = 4 - s.length();
        for (int i = 0; i < n; i++) {
            s = '0' + s;
        }
        return s;
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

    void createSymbol(Symbol symbol, int val, vector<Symbol> &defList) {
        symbol.moduleNum = currentModuleNumber;
        symbol.relativeAddress = val;
        defList.push_back(symbol);
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

                    if (inputFile.eof()) {
                        if (!currentLine.empty()) {
                            previousLine = currentLine;
                        }
                    } else {
                        previousLine = currentLine;
                    }

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
                    token.isEOF = true;
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
                    __parseError(0, token.lineNumber + 1, token.lineOffset + 1);
                    exit(0);
                }
                num.push_back(ch);
                ptr++;
            }
            prevValidTokenLineNum = lineNumber;
            prevValidTokenStartLineOffset = token.lineOffset;
            prevValidTokenEndLineOffset = lineOffset;
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
            __parseError(1, lineNumber, previousLine.length() + 1);
            exit(0);
        }
        if (!isValidSymbol(text)) {
            // throw appropriate validation error
            __parseError(1, token.lineNumber + 1, token.lineOffset + 1);
            exit(0);
        }
        symbol.text = text;
        symbol.lineNumber = token.lineNumber;
        symbol.lineOffset = token.lineOffset;

        prevValidTokenLineNum = lineNumber;
        prevValidTokenStartLineOffset = token.lineOffset;
        prevValidTokenEndLineOffset = lineOffset;

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
            prevValidTokenLineNum = lineNumber;
            prevValidTokenStartLineOffset = token.lineOffset;
            prevValidTokenEndLineOffset = lineOffset;

            addressingMode = text[0];
            return addressingMode;
        } else {
            // throw appropriate error
            __parseError(2, prevValidTokenLineNum + 1, prevValidTokenEndLineOffset + 1);
            exit(0);
        }
    }

    bool validateInstruction(int val) {
        int opcode = val / 1000;
        int operand = val % 1000;

        return opcode < 10 && operand < 512;
    }

    void printSymbolTable() {
        cout << "Symbol Table" << endl;
        for (SymbolTableEntry entry: symbolTable) {
            cout << entry.text << "=" << entry.absAddress;
            if (!entry.errorMsg.empty()) {
                cout << " " << entry.errorMsg;
            }
            cout << endl;
        }
    }

    void pass1() {
        while (1) {
            // definition list
            int defCount = readInt();
            if (defCount < 0) {
                return;
            }

            if (defCount > 16) {
                __parseError(4, prevValidTokenLineNum + 1, prevValidTokenStartLineOffset + 1);
                exit(0);
            }

            vector<Symbol> defList;

            for (int i = 0; i < defCount; i++) {
                Symbol symbol = readSymbol();
                int val = readInt();
                createSymbol(symbol, val, defList);
            }

            // use list
            int useCount = readInt();

            if (useCount > 16) {
                __parseError(5, prevValidTokenLineNum + 1, prevValidTokenStartLineOffset + 1);
                exit(0);
            }

            for (int i = 0; i < useCount; i++) {
                Symbol symbol = readSymbol();
            }

            // instruction list
            int instrCount = readInt();

//            int prevLineNum = this->lineNumber;
//            int prevLineOffset = this->lineOffset;

            if (baseAddrOfCurrentModule + instrCount >= 512) {
                __parseError(6, prevValidTokenLineNum + 1, prevValidTokenStartLineOffset + 1);
                exit(0);
            }

            ModuleTableEntry moduleTableEntry(currentModuleNumber, baseAddrOfCurrentModule, instrCount);
            moduleTable.push_back(moduleTableEntry);

            for (int i = 0; i < instrCount; i++) {
                char addrMode = readMARIE();
                int val = readInt();

//                if (!validateInstruction(val)) {
//                    // throw appropriate error
//                }
            }

            for (auto itr = defList.begin(); itr != defList.end(); itr++) {
                if (!isSymbolAlreadyDefined(itr->text, currentModuleNumber, instrCount)) {
                    int offset = itr->relativeAddress;
                    if (offset >= instrCount) {
                        offset = 0;
                        cout << "Warning: Module " << currentModuleNumber << ": " << itr->text << "="
                             << itr->relativeAddress
                             << " valid=[0.." << (instrCount - 1) << "] assume zero relative" << endl;
                    }
                    int absAddress = baseAddrOfCurrentModule + offset;
                    SymbolTableEntry entry(itr->text, currentModuleNumber, absAddress, false, "");
                    symbolTable.push_back(entry);
                }
            }

            currentModuleNumber++;
            baseAddrOfCurrentModule += instrCount;
        }
    }

    bool isSymbolAlreadyDefined(string symbol, int moduleNumber, int moduleSize) {
        bool isAlreadyDefined = false;
        for (SymbolTableEntry &entry: symbolTable) {
            if (entry.text == symbol) {
                isAlreadyDefined = true;
                entry.errorMsg = "Error: This variable is multiple times defined; first value used";
                cout << "Warning: Module " << moduleNumber << ": " << symbol << " redefinition ignored" << endl;
                break;
            }
        }
        return isAlreadyDefined;
    }

    int getBaseAddressOf(string symbol) {
        for (SymbolTableEntry &entry: symbolTable) {
            if (entry.text == symbol) {
                entry.used = true;
                return entry.absAddress;
            }
        }
        return 0;
    }

    void resolveExternalAddress(int operand, vector<Symbol> &useList, int memoryRef, string location,
                                vector<int> &useListVis) {
        if (operand >= useList.size() || operand < 0) {
//            string t = useList[0].text;
//            memoryRef += getBaseAddressOf(t);

            memoryMap[location] = getMemoryRef(memoryRef);
            cout << memoryMap[location] << " ";
            cout << "Error: External operand exceeds length of uselist; treated as relative=0" << endl;
            return;
        }
        string token = useList[operand].text;
        useListVis[operand] = 1;

        bool found = false;
        SymbolTableEntry tableEntry;
        for (SymbolTableEntry &entry: symbolTable) {
            if (entry.text == token) {
                found = true;
                memoryRef += entry.absAddress;
                entry.used = true;

                // delete from unusedSymbols
                auto it = std::find(unusedSymbols.begin(), unusedSymbols.end(), token);
                if (it != unusedSymbols.end()) {
                    unusedSymbols.erase(it);
                }

                memoryMap[location] = getMemoryRef(memoryRef);
                cout << memoryMap[location] << endl;
                break;
            }
        }

        if (!found) {
            memoryMap[location] = getMemoryRef(memoryRef);
            cout << memoryMap[location] << " ";
            cout << "Error: " << token << " is not defined; zero used" << endl;
        }
    }

    void resolveAbsoluteAddress(int operand, int memoryRef, string location) {
        if (operand >= 512) {
            memoryMap[location] = getMemoryRef(memoryRef);
            cout << memoryMap[location] << " ";
            cout << "Error: Absolute address exceeds machine size; zero used" << endl;
        } else {
            memoryRef += operand;
            memoryMap[location] = getMemoryRef(memoryRef);
            cout << memoryMap[location] << endl;
        }
    }

    void resolveImmediateAddress(int operand, int memoryRef, string location) {
        if (operand >= 900) {
            memoryRef += 999;
            memoryMap[location] = getMemoryRef(memoryRef);
            cout << memoryMap[location] << " ";
            cout << "Error: Illegal immediate operand; treated as 999" << endl;
        } else {
            memoryRef += operand;
            memoryMap[location] = getMemoryRef(memoryRef);
            cout << memoryMap[location] << endl;
        }
    }

    void resolveRelativeAddress(int operand, int instrCount, int memoryRef, string location) {
        if (operand >= instrCount) {
            memoryRef += baseAddrOfCurrentModule;
            memoryMap[location] = getMemoryRef(memoryRef);
            cout << memoryMap[location] << " ";
            cout << "Error: Relative address exceeds module size; relative zero used" << endl;
        } else {
            memoryRef += baseAddrOfCurrentModule + operand;
            memoryMap[location] = getMemoryRef(memoryRef);
            cout << memoryMap[location] << endl;
        }
    }

    void resolveModuleAddress(int operand, int memoryRef, string location) {
        bool found = false;
        for (ModuleTableEntry moduleTableEntry: moduleTable) {
            if (moduleTableEntry.moduleNum == operand) {
                found = true;
                memoryRef += moduleTableEntry.baseAddress;
                memoryMap[location] = getMemoryRef(memoryRef);
                cout << memoryMap[location] << endl;
                break;
            }
        }

        if (!found) {
            memoryMap[location] = getMemoryRef(memoryRef);
            cout << memoryMap[location] << " ";
            cout << "Error: Illegal module operand ; treated as module=0" << endl;
        }
    }

    void
    resolveMemoryReference(char addrMode, int val, vector<Symbol> &useList, int instrCount, vector<int> &useListVis) {
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
            memoryMap[location] = getMemoryRef(9999);
            cout << memoryMap[location] << " ";
            cout << "Error: Illegal opcode; treated as 9999" << endl;
            currentMemoryLocation++;
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
                resolveExternalAddress(operand, useList, memoryRef, location, useListVis);
                break;
            default: // throw error in case control moves to default
                break;
        }

        currentMemoryLocation++;
    }

    int getIndexOf(string str, vector<Symbol> v) {
        int index = 0;
        for (int i = 0; i < v.size(); i++) {
            if (v[i].text == str) {
                index = i;
                break;
            }
        }
        return index;
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

            // to keep track of unused symbols from use list
            vector<int> useListVis(useCount, -1);

            for (int i = 0; i < useCount; i++) {
                Symbol symbol = readSymbol();
                useList.push_back(symbol);
            }

            // instruction list
            int instrCount = readInt();

            for (int i = 0; i < instrCount; i++) {
                char addrMode = readMARIE();
                int val = readInt();
                resolveMemoryReference(addrMode, val, useList, instrCount, useListVis);
            }

            for (int i = 0; i < useListVis.size(); i++) {
                if (useListVis[i] == -1) {
                    printf("Warning: Module %d: uselist[%d]=%s was not used\n", currentModuleNumber, i,
                           useList[i].text.c_str());
                }
            }

            currentModuleNumber++;
            baseAddrOfCurrentModule += instrCount;
        }
    }

    void printUnusedSymbols() {
        cout << endl;
        for (auto itr = symbolTable.begin(); itr != symbolTable.end(); itr++) {
            if (!itr->used) {
                cout << "Warning: Module " << itr->moduleNum << ": " << itr->text << " was defined but never used"
                     << endl;
            }
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
        printSymbolTable();
        resetTokenizerParams();
        pass2();
        printUnusedSymbols();
    }
};

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cout << "Invalid arguments";
        return 0;
    }
    Tokenizer *tokenizer = new Tokenizer(argv[1]);
    tokenizer->parse();
    return 0;
}
