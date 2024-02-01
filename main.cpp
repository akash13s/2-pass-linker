#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>

using namespace std;

struct Token {
    char *token;
    int lineNumber;
    int lineOffset;

    Token(char *text, int lineNumber, int lineOffset) {
        this->token = text;
        this->lineNumber = lineNumber;
        this->lineOffset = lineOffset;
    }
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
};

int main() {
    Tokenizer *tokenizer = new Tokenizer("/Users/akashshrivastva/Documents/OS/Linker/input.txt");
    while (1) {
        Token token = tokenizer->getToken();
        if (token.lineNumber == -1) {
            break;
        }
        cout<<"Token: "<<(token.lineNumber+1)<<":"<<(token.lineOffset+1)<<" : "<<token.token<<endl;
    }
    return 0;
}
