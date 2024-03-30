parser: main.c lexical.l syntax.y syntaxParsingTree.c symbolTable.c ir.c asmGen.c
				flex lexical.l
				bison -d -v -t syntax.y
				gcc main.c syntax.tab.c syntaxParsingTree.c symbolTable.c asmGen.c ir.c -lfl -ly -g -o parser

clean:
				rm -f *.o
				rm -f parser lex.yy.c syntax.tab.c syntax.tab.h syntax.output output.ir output.s