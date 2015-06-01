/*
= Brainfuck
If you have gcc:
----
g++ -o brainfuck.exe brainfuck.cpp
brainfuck.exe helloworld.bf
----
*/

#include <vector>
#include <iostream>
#include <fstream>

using namespace std;

/**
 * Primitive Brainfuck commands
 */
typedef enum { 
    INCREMENT, // +
    DECREMENT, // -
    SHIFT_LEFT, // <
    SHIFT_RIGHT, // >
    INPUT, // ,
    OUTPUT // .
} Command;

// Forward references. Silly C++!
class CommandNode;
class Loop;
class Program;

/**
 * Visits?!? Well, that'd indicate visitors!
 * A visitor is an interface that allows you to walk through a tree and do stuff.
 */
class Visitor {
    public:
        virtual void visit(const CommandNode * leaf) = 0;
        virtual void visit(const Loop * loop) = 0;
        virtual void visit(const Program * program) = 0;
};

/**
 * The Node class (like a Java abstract class) accepts visitors, but since it's pure virtual, we can't use it directly.
 */
class Node {
    public:
        virtual void accept (Visitor *v) = 0;
};

/**
 * CommandNode publicly extends Node to accept visitors.
 * CommandNode represents a leaf node with a primitive Brainfuck command in it.
 */
class CommandNode : public Node {
    public:
        Command command;
        CommandNode(char c) {
            switch(c) {
                case '+': command = INCREMENT; break;
                case '-': command = DECREMENT; break;
                case '<': command = SHIFT_LEFT; break;
                case '>': command = SHIFT_RIGHT; break;
                case ',': command = INPUT; break;
                case '.': command = OUTPUT; break;
            }
        }
        void accept (Visitor * v) {
            v->visit(this);
        }
};

class Container: public Node {
    public:
        vector<Node*> children;
        virtual void accept (Visitor * v) = 0;
};

/**
 * Loop publicly extends Node to accept visitors.
 * Loop represents a loop in Brainfuck.
 */
class Loop : public Container {
    public:
        void accept (Visitor * v) {
            v->visit(this);
        }
};

/**
 * Program is the root of a Brainfuck program abstract syntax tree.
 * Because Brainfuck is so primitive, the parse tree is the abstract syntax tree.
 */
class Program : public Container {
    public:
        void accept (Visitor * v) {
            v->visit(this);
        }
};

/**
 * Read in the file by recursive descent.
 * Modify as necessary and add whatever functions you need to get things done. 20 lines
 */
void parse(fstream & file, Container * container) {
    char c;
    
	while ((char)file.peek() != -1) {
		file >> c; // read in next character
		
		if (c == '[') { 
			// If beginning of loop declare new loop and call parse recursively til loop is over
			Loop * loop = new Loop;
			container->children.push_back(loop);
			parse(file, loop);
		}
		else if (c == ']') { // Loop has ended, end recursive call
			return;
		}
		else if (c == '+' || c == '-' || c == '<' || c == '>' || c == ',' || c == '.') { // Command found, add it to the parent container
			container->children.push_back(new CommandNode(c));
		}
	}
}

/**
 * A printer for Brainfuck abstract syntax trees.
 * As a visitor, it will just print out the commands as is.
 * For Loops and the root Program node, it walks trough all the children.
 */
class Printer : public Visitor {
    public:
        void visit(const CommandNode * leaf) {
            switch (leaf->command) {
                case INCREMENT:   cout << '+'; break;
                case DECREMENT:   cout << '-'; break;
                case SHIFT_LEFT:  cout << '<'; break;
                case SHIFT_RIGHT: cout << '>'; break;
                case INPUT:       cout << ','; break;
                case OUTPUT:      cout << '.'; break;
            }
        }
        void visit(const Loop * loop) {
            cout << '[';
            for (vector<Node*>::const_iterator it = loop->children.begin(); it != loop->children.end(); ++it) {
                (*it)->accept(this);
            }
            cout << ']';
        }
        void visit(const Program * program) {
            for (vector<Node*>::const_iterator it = program->children.begin(); it != program->children.end(); ++it) {
                (*it)->accept(this);
            }
            cout << '\n';
        }
};

class Compiler : public Visitor {
    public:
        void visit(const CommandNode * leaf) {
            switch (leaf->command) {
                case INCREMENT:   cout << "++*ptr;"; break;
                case DECREMENT:   cout << "--*ptr;"; break;
                case SHIFT_LEFT:  cout << "--ptr;\n"; break;
                case SHIFT_RIGHT: cout << "++ptr;\n"; break;
                case INPUT:       cout << "*ptr=getchar();\n"; break;
                case OUTPUT:      cout << "putchar(*ptr);\n"; break;
            }
        }
        void visit(const Loop * loop) {
            cout << "while (*ptr) {";
            for (vector<Node*>::const_iterator it = loop->children.begin(); it != loop->children.end(); ++it) {
                (*it)->accept(this);
            }
            cout << "}";
        }
        void visit(const Program * program) {
			cout << "#include <stdio.h>\n";
			cout << "char array[30000] = {0};\n";
			cout << "char *ptr=array;\n";
			cout << "int main(int argc, char **argv) {\n";
			
            for (vector<Node*>::const_iterator it = program->children.begin(); it != program->children.end(); ++it) {
                (*it)->accept(this);
            }
            cout << "}\n";
        }
};

class Interpreter : public Visitor {
	char memory[3000];
	int curPos;
	public:
        void visit(const CommandNode * leaf) {
            switch (leaf->command) {
                case INCREMENT:
					memory[curPos] = memory[curPos] + 1;
                    break;
                case DECREMENT:
					memory[curPos] = memory[curPos] - 1;
                    break;
                case SHIFT_LEFT:
					curPos--;
                    break;
                case SHIFT_RIGHT:
					curPos++;
                    break;
                case INPUT:
                    break;
                case OUTPUT:
					cout << memory[curPos];
                    break;
            }
        }
        void visit(const Loop * loop) {
            for (vector<Node*>::const_iterator it = loop->children.begin(); it != loop->children.end(); ++it) {
                (*it)->accept(this);
            }
			if (memory[curPos] != 0) {
				visit(loop);
			}
        }
        void visit(const Program * program) {
			// zero init the memory array
			// set the pointer to zero
			curPos = 0;
			for (int i = 0; i < 3000; i++) {
				memory[i] = 0;
			}
            for (vector<Node*>::const_iterator it = program->children.begin(); it != program->children.end(); ++it) {
                (*it)->accept(this);
            }
        }
};

int main(int argc, char *argv[]) {
    fstream file;
    Program program;
    Printer printer;
    Interpreter interpreter;
	Compiler compiler;
    if (argc == 1) {
        cout << argv[0] << ": No input files." << endl;
    } else if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            file.open(argv[i], fstream::in);
            parse(file, & program);
//            program.accept(&printer);
            program.accept(&interpreter);
			//program.accept(&compiler);
            file.close();
        }
    }
}