#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>

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

public:
    Tokenizer(string filename) {
        this->filename = filename;
        this->inputFile.open(filename);
        this->delimiters = " \n\t";
        this->currToken = nullptr;
        this->lineNumber = -1;
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

};

int main() {
    Tokenizer *tokenizer = new Tokenizer("/Users/akashshrivastva/Documents/OS/Linker/input.txt");
//    while (1) {
//        Symbol symbol = tokenizer->readSymbol();
//        if (symbol.lineNumber<0) {
//            break;
//        }
//        cout<<symbol.text<<endl;
//
////        cout << "Token: " << (token.lineNumber+1) << ":" << (token.lineOffset+1) << " : " << token.text << endl;
//    }
    char c = tokenizer->readMARIE();
    cout<<c<<endl;

    return 0;
}
