#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <map>

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
    map<int, int> memoryMap;

    bool isDigit(char c) {
        return (c=='1' || c=='2' || c=='3' || c=='4' || c=='5' || c=='6' || c=='7' || c=='8' || c=='9' || c=='0');
    }

    bool isValidSymbol(string text) {
        /*
         * what constitutes a valid symbol?
         * not an integer/floating point number
         * what else?
         */
        return  true;
    }

    bool isValidAddressingMode(char c) {
        return (c=='M' || c=='A' || c=='R' || c=='I' || c=='E');
    }

    void createSymbol(Symbol symbol, int val) {
        int absAddress = baseAddrOfCurrentModule + val;
        symbolTable[symbol.text] = absAddress;
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
            while (*ptr!='\0') {
                char ch = *ptr;
                if (!isDigit(ch)) {
                    // throw error for invalid number
                    cout<<"Error"<<endl;
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
        if (token.lineNumber != -1 && strlen(token.text)>0) {
            char *ptr = token.text;
            while (*ptr!='\0') {
                char ch = *ptr;
                text.push_back(ch);
                ptr++;
            }
        } else {
            // throw appropriate error
        }
        if (!isValidSymbol(text)) {
            // throw appropriate vcalidation error
        }
        symbol.text = text;
        symbol.lineNumber = token.lineNumber;
        symbol.lineOffset = token.lineOffset;
        return symbol;
    }

    char readMARIE() {
        Token token = getToken();
        char addressingMode;
        if (token.lineNumber!=-1) {
            char *ptr = token.text;
            string text;
            while (*ptr!='\0') {
                char ch = *ptr;
                text.push_back(ch);
                ptr++;
            }
            if (text.length()>1) {
                // throw error
                cout<<"Error"<<endl;
            }
            if (!isValidAddressingMode(text[0])) {
                // throw appropriate error
                cout<<"Error"<<endl;
            }
            addressingMode = text[0];
            return addressingMode;
        } else {
            // throw appropriate error
            return '0';
        }
    }

    void validateInstructionValue(int val) {

    }

    void pass1() {
        while (1) {
            // definition list
            int defCount = readInt();
            if (defCount < 0) {
                return ;
            }
            for (int i=0; i<defCount; i++) {
                Symbol symbol = readSymbol();
                int val = readInt();
                createSymbol(symbol, val);
            }

            // use list
            int useCount = readInt();
            for (int i=0; i<useCount; i++) {
                Symbol symbol = readSymbol();
            }

            // instruction list
            int instrCount = readInt();
            moduleTable[currentModuleNumber] = {baseAddrOfCurrentModule, instrCount};

            for (int i=0; i<instrCount; i++) {
                char addrMode = readMARIE();
                int val = readInt();

                validateInstructionValue(val);
            }

            currentModuleNumber++;
            baseAddrOfCurrentModule += instrCount;
        }
    }

    int resolveExternalAddress(int operand, vector<Symbol> &useList) {
        string token = useList[operand].text;
        return symbolTable[token];
    }

    void resolveMemoryReference(char addrMode, int val, vector<Symbol> &useList) {
        int opcode = val/1000;
        int operand = val%1000;

        int memoryRef = opcode*1000;

        switch (addrMode) {
            case 'M':
                operand = baseAddrOfCurrentModule;
                memoryRef += operand;
                break;
            case 'A':
                assert(operand<512);
                memoryRef += operand;
                break;
            case 'R':
                memoryRef += baseAddrOfCurrentModule + operand;
                break;
            case 'I':
                assert(operand<900);
                memoryRef += operand;
                break;
            case 'E':
                memoryRef += resolveExternalAddress(operand, useList);
                break;
            default: // throw error in case control moves to default
        }

        memoryMap[currentMemoryLocation] = memoryRef;
        currentMemoryLocation++;
    }

    void pass2() {
        while (1) {
            // definition list
            int defCount = readInt();
            if (defCount < 0) {
                return ;
            }
            for (int i=0; i<defCount; i++) {
                Symbol symbol = readSymbol();
                int val = readInt();
            }

            // use list
            int useCount = readInt();
            vector<Symbol> useList;

            for (int i=0; i<useCount; i++) {
                Symbol symbol = readSymbol();
                useList.push_back(symbol);
            }

            // instruction list
            int instrCount = readInt();

            for (int i=0; i<instrCount; i++) {
                char addrMode = readMARIE();
                int val = readInt();
                resolveMemoryReference(addrMode, val, useList);
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
