#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#define BSIZE 128
#define NONE -1
#define EOS '\0'

#define NUM 256
#define DIV 257
#define MOD 258
#define ID 259
#define DONE 260
#define PROGRAM 261
#define INPUT 262
#define OUTPUT 263

int tokenval;
int lineno = 1;
int lexan(void);
void error(char *m);
void init(void);
int insert(char *s, int tok);
int lookup(char *s);
void parse(void);
void match(int t);
void expr(void);
void exprp(void);
void term(void);
void termp(void);
void factor(void);
void emit(int t, int tval);

FILE *inputFile;
FILE *outputFile;

struct entry {
    char *lexptr;
    int token;
};

#define SYMMAX 100
struct entry symtable[SYMMAX];
int lastentry = 0;

char lexbuf[BSIZE];

int lexan(void) {
    int t;
    while (1) {
        t = fgetc(inputFile);
        if (t == ' ' || t == '\t')
            continue;
        else if (t == '\n')
            lineno++;
        else if (t == '#') {
            while ((t = fgetc(inputFile)) != '\n' && t != EOF);
            if (t == '\n') lineno++;
        }
        else if (isdigit(t)) {
            ungetc(t, inputFile);
            fscanf(inputFile, "%d", &tokenval);
            return NUM;
        }
        else if (isalpha(t)) {
            int p, b = 0;
            while (isalnum(t)) {
                lexbuf[b++] = t;
                t = fgetc(inputFile);
                if (b >= BSIZE)
                    error("compiler error: token too long");
            }
            lexbuf[b] = EOS;
            if (t != EOF)
                ungetc(t, inputFile);
            p = lookup(lexbuf);
            if (p == 0)
                p = insert(lexbuf, ID);
            tokenval = p;
            return symtable[p].token;
        }
        else if (t == '\\') { 
            tokenval = NONE;
            return '/';  
        }
        else if (t == EOF)
            return DONE;
        else {
            tokenval = NONE;
            return t;
        }
    }
}

int lookup(char s[]) {
    for (int i = lastentry; i > 0; i--) {
        if (strcmp(symtable[i].lexptr, s) == 0)
            return i;
    }
    return 0;
}

int insert(char s[], int tok) {
    int len = strlen(s);
    if (lastentry + 1 >= SYMMAX)
        error("symbol table full");
    lastentry++;
    symtable[lastentry].token = tok;
    symtable[lastentry].lexptr = strdup(s);
    return lastentry;
}

struct entry keywords[] = {
    {"div", DIV},
    {"mod", MOD},
    {"program", PROGRAM},
    {"input", INPUT},
    {"output", OUTPUT},
    {NULL, 0}
};

void init(void) {
    for (struct entry *p = keywords; p->token; p++) {
        insert(p->lexptr, p->token);
    }
}

void error(char *m) {
    fprintf(stderr, "line %d: %s\n", lineno, m);
    exit(1);
}

int lookahead;

void parse(void) {
    lookahead = lexan();
    match(PROGRAM);
    
    if (lookahead != ID)
        error("Expected program identifier");
    int progNameIndex = tokenval;
    match(ID);
    
    match('(');
    match(INPUT);
    match(',');
    match(OUTPUT);
    match(')');
    match('{');
    
    fprintf(outputFile, "program %s(input,output)\n{\n", symtable[progNameIndex].lexptr);
    
    while (lookahead != '}') {
        expr();
        match(';');
        fprintf(outputFile, ";\n");
    }
    match('}');
    fprintf(outputFile, "}\n");
}

void expr(void) {
    term();
    exprp();
}

void exprp(void) {
    if (lookahead == '+' || lookahead == '-') {
        int op = lookahead;
        match(op);
        term();
        emit(op, NONE);
        exprp();
    }
}

void term(void) {
    factor();
    termp();
}

void termp(void) {
    if (lookahead == '*' || lookahead == '/' || lookahead == DIV || lookahead == MOD || lookahead == '%') {
        int op = lookahead;
        match(op);
        factor();
        emit(op, NONE);
        termp();
    }
}

void factor(void) {
    if (lookahead == '(') {
        match('(');
        expr();
        match(')');
    } else if (lookahead == NUM) {
        emit(NUM, tokenval);
        match(NUM);
    } else if (lookahead == ID) {
        emit(ID, tokenval);
        match(ID);
    } else {
        error("syntax error in factor");
    }
}

void match(int t) {
    if (lookahead == t)
        lookahead = lexan();
    else {
        fprintf(stderr, "Expected token %d, but found token %d\n", t, lookahead);
        error("syntax error in match");
    }
}

void emit(int t, int tval) {
    switch (t) {
        case '+': case '-': case '*':
            fprintf(outputFile, "%c ", t);
            break;
        case '/':
            fprintf(outputFile, "\\ ");
            break;
        case '%':
            fprintf(outputFile, "%% ");
            break;
        case DIV:
            fprintf(outputFile, "DIV ");
            break;
        case MOD:
            fprintf(outputFile, "MOD ");
            break;
        case NUM:
            fprintf(outputFile, "%d ", tval);
            break;
        case ID:
            fprintf(outputFile, "%s ", symtable[tval].lexptr);
            break;
        default:
            fprintf(outputFile, "token %d, tokenval %d ", t, tval);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
        exit(1);
    }
    
    inputFile = fopen(argv[1], "r");
    if (!inputFile) {
        fprintf(stderr, "Error opening input file %s\n", argv[1]);
        exit(1);
    }
    
    outputFile = fopen(argv[2], "w");
    if (!outputFile) {
        fprintf(stderr, "Error opening output file %s\n", argv[2]);
        fclose(inputFile);
        exit(1);
    }
    
    init();
    parse();
    
    fclose(inputFile);
    fclose(outputFile);
    return 0;
}

 
