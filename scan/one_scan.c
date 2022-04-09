
/*
* 컴파일러 과제 - scanner 구현
* 2020112757 컴퓨터학부 김유진
*/

#define _CRT_SECURE_NO_WARNINGS
#define MAXTOKENLEN 40 /* MAXTOKENLEN is the maximum size of a token */
#define MAXRESERVED 6 /* MAXRESERVED = the number of reserved words */
#define BUFLEN 256
#define FALSE 0
#define TRUE 1

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

int lineno = 0;
FILE* source;
FILE* listing;
FILE* fp;
FILE* code;

/* allocate and set tracing flags */
int EchoSource = TRUE;
int TraceScan = TRUE;
int Error = FALSE;

static char lineBuf[BUFLEN]; /* holds the current line */
static int linepos = 0; /* current position in LineBuf */
static int bufsize = 0; /* current size of buffer string */
static int EOF_flag = FALSE; /* corrects ungetNextChar behavior on EOF */

// 토큰을 담을 배열
char tokenString[MAXTOKENLEN + 1];

// StateType 정의
typedef enum
{
    START, INASSIGN, INCOMMENT, INNUM, INID, DONE
}
StateType;


// 토큰 타입
typedef enum
/* book-keeping tokens */
{
	ENDFILE, ERROR,
	/* reserved words */
	IF, ELSE, INT, RETURN, VOID, WHILE,
	/* multicharacter tokens */
	ID, NUM,
	/* special symbols */
	PLUS, MINUS, TIMES, OVER, LT, LTE, GT, GTE, EQ, NEQ, ASSIGN, COMMA, SEMI, LPAREN, RPAREN, LBRAC, RBRAC, LCBRAC, RCBRAC
} TokenType;

// 예약어 테이블
/* lookup table of reserved words */
static struct
{
    char* str;
    TokenType tok;
} reservedWords[MAXRESERVED]
= { {"if",IF},{"else",ELSE},
   {"int",INT},{"return",RETURN},
    {"void",VOID},{"while",WHILE} };

static int getNextChar(void);
static void ungetNextChar(void);
static TokenType reservedLookup(char* s);
TokenType getToken(void);
void printToken(TokenType token, const char* tokenString);

main(int argc, char* argv[])
{
    char pgm[120]; /* source code file name */

    // filename[.exe] input[.c] ouput[.txt] 
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <input_filename> <output_filename>\n", argv[0]);
        exit(1);
    }

	// c파일 열기
    strcpy(pgm, argv[1]);
    if (strchr(pgm, '.') == NULL)
        strcat(pgm, ".c"); // 대문자?
    source = fopen(pgm, "r");

    if (source == NULL)
    {
        fprintf(stderr, "File %s not found\n", pgm);
        exit(1);
    }

    // 결과 출력: .txt 파일에 결과 출력.
    fp = fopen(argv[2], "w");
	if (fp == NULL)
	{
		fprintf(stderr, "File %s not found\n", argv[2]);
		exit(1);
	}
    // listing = stdout; /* send listing to screen */
    listing = fp;
    fprintf(listing, "\nC- COMPILATION: %s\n", pgm);

    while (getToken() != ENDFILE); // 파일이 끝날 때 까지 토큰 얻어오기

    fclose(source);
	fclose(fp);
    return 0;
}

static int getNextChar(void)
{
    if (!(linepos < bufsize))
    {
        lineno++;
        if (fgets(lineBuf, BUFLEN - 1, source))
        {
            if (EchoSource) fprintf(listing, "%4d: %s", lineno, lineBuf);
            bufsize = strlen(lineBuf);
            linepos = 0;
            return lineBuf[linepos++];
        }
        else
        {
            EOF_flag = TRUE;
            return EOF;
        }
    }
    else return lineBuf[linepos++];
}

/* ungetNextChar backtracks one character
   in lineBuf */
static void ungetNextChar(void)
{
    if (!EOF_flag) linepos--;
}

// 여기서 이진탐색해서 성능을 향상 시킴
/* lookup an identifier to see if it is a reserved word */
/* uses linear search */
static TokenType reservedLookup(char* s)
{
    int i;
    for (i = 0; i < MAXRESERVED; i++)
        if (!strcmp(s, reservedWords[i].str))
            return reservedWords[i].tok;
    return ID;
}

/****************************************/
/* the primary function of the scanner  */
/****************************************/
/* function getToken returns the
 * next token in source file
 */
TokenType getToken(void)
{
	/* index for storing into tokenString */
	int tokenStringIndex = 0;

	/* holds current token to be returned */
	TokenType currentToken;

	/* current state - always begins at START : 처음 시작 start로 */
	StateType state = START;

	/* flag to indicate save to tokenString */
	int save;

	while (state != DONE)
	{
		int c = getNextChar();
		save = TRUE;

		switch (state)
		{
		case START:
			if (isdigit(c)) // 숫자
				state = INNUM;
			else if (isalpha(c)) // 문자
				state = INID;
			else if ((c == '<') || (c == '>') || (c == '=') || (c == '!')) // INASSIGN state에 들어가는 조건 변경
				state = INASSIGN; // <, >, =, !
			else if ((c == ' ') || (c == '\t') || (c == '\n')) // 공백문자
				save = FALSE;
			else if (c == '/') // 주석 처리 부분
			{
				// [OTHER]
				// 다음 문자가 '*'일 경우 INCOMMENT state로 진입함.
				c = getNextChar(); 
				if (c == '*') { 
					save = FALSE;
					state = INCOMMENT;
				}
				// 다음 문자가 '*'가 아닐 경우, 커서를 당기고 '/'를 토큰으로 인식함.
				else {
					ungetNextChar();
					state = DONE;
					currentToken = OVER;
				}
			}
			else
			{
				state = DONE;
				switch (c)
				{
				case EOF:
					save = FALSE;
					currentToken = ENDFILE;
					break;
				case '+':
					currentToken = PLUS;
					break;
				case '-':
					currentToken = MINUS;
					break;
				case '*':
					currentToken = TIMES;
					break;
				case ',':	// , 추가
					currentToken = COMMA;
					break;
				case '(':
					currentToken = LPAREN;
					break;
				case ')':
					currentToken = RPAREN;
					break;
				case '[':	// [ 추가
					currentToken = LBRAC;
					break;
				case ']':	// ] 추가
					currentToken = RBRAC;
					break;
				case '{':	// { 추가
					currentToken = LCBRAC;
					break;
				case '}':	// } 추가
					currentToken = RCBRAC;
					break;
				case ';':
					currentToken = SEMI;
					break;
				default:
					currentToken = ERROR;
					break;
				}
			}
			break;
		case INCOMMENT:
			save = FALSE;
			// comment가 끝나기 전에 파일이 끝이나면 error를 출력함.
			if (c == EOF)
			{
				state = DONE;
				currentToken = ENDFILE;
				fprintf(listing, "ERROR: stop before ending\n");
			}
			else if (c == '*')
			{
				// 다음 문자가 '/'일 경우, comment가 끝나고 START state로 돌아감.
				c = getNextChar();
				if (c == '/') {
					state = START;
					break;
				}
				else { ungetNextChar(); }
			}
			break;
		case INASSIGN:
			state = DONE;
			char t = tokenString[0]; // 앞서 저장한 문자
			// 두 번째로 오는 문자가 '='이 아닐 경우
			if (c != '=') {
				/* backup in the input */
				ungetNextChar();
				save = FALSE;
				switch (t)
				{
				case '<':
					currentToken = LT;
					break;
				case '>':
					currentToken = GT;
					break;
				case '=':
					currentToken = ASSIGN;
					break;
				case '!':
					currentToken = ERROR;
					break;
				}
			}
			else // c == '='
			{
				switch (t)
				{
				case '<':
					currentToken = LTE;
					break;
				case '>':
					currentToken = GTE;
					break;
				case '=':
					currentToken = EQ;
					break;
				case '!':
					currentToken = NEQ;
					break;
				}
			}
			break;
		case INNUM:
			if (!isdigit(c))
			{ /* backup in the input */
				ungetNextChar();
				save = FALSE;
				state = DONE;
				currentToken = NUM;
			}
			break;
		case INID:
			if (!isalpha(c))
			{ /* backup in the input */
				ungetNextChar();
				save = FALSE;
				state = DONE;
				currentToken = ID;
			}
			break;
		case DONE:
		default: /* should never happen */
			fprintf(listing, "Scanner Bug: state= %d\n", state);
			state = DONE;
			currentToken = ERROR;
			break;
		}
		if ((save) && (tokenStringIndex <= MAXTOKENLEN))
			tokenString[tokenStringIndex++] = (char)c;
		if (state == DONE)
		{
			tokenString[tokenStringIndex] = '\0';
			if (currentToken == ID)
				currentToken = reservedLookup(tokenString);
		}
	}
	if (TraceScan) {
		fprintf(listing, "\t%d: ", lineno); // 라인 넘버
		printToken(currentToken, tokenString);  // UTIL.C에 있음
	}
	return currentToken;
} /* end getToken */

void printToken(TokenType token, const char* tokenString)
{
	switch (token)
	{
	case IF:
	case ELSE:
	case INT:
	case RETURN:
	case VOID:
	case WHILE:
		fprintf(listing,
			"reserved word: %s\n", tokenString);
		break;
	case ASSIGN: fprintf(listing, "=\n"); break;
	case EQ: fprintf(listing, "==\n"); break;
	case NEQ: fprintf(listing, "!=\n"); break;
	case LT: fprintf(listing, "<\n"); break;
	case LTE: fprintf(listing, "<=\n"); break;
	case GT: fprintf(listing, ">\n"); break;
	case GTE: fprintf(listing, ">=\n"); break;
	case LPAREN: fprintf(listing, "(\n"); break;
	case RPAREN: fprintf(listing, ")\n"); break;
	case LBRAC: fprintf(listing, "[\n"); break;
	case RBRAC: fprintf(listing, "]\n"); break;
	case LCBRAC: fprintf(listing, "{\n"); break;
	case RCBRAC: fprintf(listing, "}\n"); break;
	case SEMI: fprintf(listing, ";\n"); break;
	case PLUS: fprintf(listing, "+\n"); break;
	case MINUS: fprintf(listing, "-\n"); break;
	case TIMES: fprintf(listing, "*\n"); break;
	case OVER: fprintf(listing, "/\n"); break;
	case COMMA: fprintf(listing, ",\n"); break;
	case ENDFILE: fprintf(listing, "EOF\n"); break;
	case NUM:
		fprintf(listing,
			"NUM, val= %s\n", tokenString);
		break;
	case ID:
		fprintf(listing,
			"ID, name= %s\n", tokenString);
		break;
	case ERROR:
		fprintf(listing,
			"ERROR: %s\n", tokenString);
		break;
	default: /* should never happen */
		fprintf(listing, "Unknown token: %d\n", token);
	}
}