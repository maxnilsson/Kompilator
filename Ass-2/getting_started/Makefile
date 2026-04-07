compiler: lex.yy.c parser.tab.o SymbolTable.o STBuilder.o SemanticAnalyzer.o main.cc
		g++ -g -w -ocompiler parser.tab.o SymbolTable.o STBuilder.o SemanticAnalyzer.o lex.yy.c main.cc -std=c++14
parser.tab.o: parser.tab.cc
		g++ -g -w -c parser.tab.cc -std=c++14
SymbolTable.o: SymbolTable.cc SymbolTable.h
		g++ -g -w -c SymbolTable.cc -std=c++14
STBuilder.o: STBuilder.cc STBuilder.h SymbolTable.h Node.h
		g++ -g -w -c STBuilder.cc -std=c++14
SemanticAnalyzer.o: SemanticAnalyzer.cc SemanticAnalyzer.h SymbolTable.h Node.h
		g++ -g -w -c SemanticAnalyzer.cc -std=c++14
parser.tab.cc: parser.yy
		bison parser.yy
lex.yy.c: lexer.flex parser.tab.cc
		flex lexer.flex
tree: 
		 dot -Tpdf tree.dot -otree.pdf
clean:
		rm -f parser.tab.* lex.yy.c* compiler stack.hh position.hh location.hh tree.dot tree.pdf SymbolTable.o STBuilder.o SemanticAnalyzer.o symboltable.dot symboltable.pdf
		rm -rf compiler.dSYM