#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

class Token {
private:
    int lineNumber;
    int offset;
    char *ch;

public:
    Token(char *ch, int lineNumber, int offset) {
        this->ch = ch;
        this->lineNumber = lineNumber;
        this->offset = offset;
    }
};

void tokenize(string filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << endl;
        return;
    }

    string line;
    int line_number = 0;

    while (getline(file, line)) {
        line_number++;
        istringstream iss(line);
        string token;
        int offset = 0;
        while (iss >> token) {
            offset = line.find(token, offset);
            cout << "Token: " << line_number << ":" << (offset + 1) << " : " << token << endl;
            offset += token.length();
        }
    }
//    cout<<"EOF position "<<line_number+1<<" "<<<<endl;

    file.close();
}

//void pass1() {
//    while (true) {
//        int defcount = readInt(); // return negative to indicate no more tokens if (defcount < 0) exit(2); // eof reached
//        for (int i=0;i<defcount;i++) {
//            Symbol sym = readSym();
//            int val = readInt();
//            createSymbol(sym,val);
//        }
//        int usecount = readInt();
//        for (int i=0;i<usecount;i++) {
//            Symbol sym = readSym();
//            // we don’t do anything here
//        }
//        int instcount = readInt();
//        for (int i=0;i<instcount;i++) {
//            char addressmode = ReadIEAR();
//            int  operand = ReadInt();
//            :  // various checks
//            :  //  - “ -
//        }
//    }
//}

int main() {
    string filename = "/Users/akashshrivastva/Documents/OS/Linker/input.txt";
    tokenize(filename);
    return 0;
}
