/****************************************************/
/* File: scan.c                                     */
/* The scanner implementation for the TINY compiler */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "util.h"
#include "scan.h"

/* states in scanner DFA */
typedef enum
{
	START, INASSIGN, INCOMMENT, INNUM, INID, DONE
}
StateType;

/* lexeme of identifier or reserved word */
char tokenString[MAXTOKENLEN + 1];

/* BUFLEN = length of the input buffer for
   source code lines */
#define BUFLEN 256

static char lineBuf[BUFLEN]; /* holds the current line */
static int linepos = 0; /* current position in LineBuf */
static int bufsize = 0; /* current size of buffer string */
static int EOF_flag = FALSE; /* corrects ungetNextChar behavior on EOF */

/* getNextChar fetches the next non-blank character
   from lineBuf, reading in a new line if lineBuf is
   exhausted */
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

// 예약어 테이블!!
/* lookup table of reserved words */
static struct
{
	char* str;
	TokenType tok;
} reservedWords[MAXRESERVED]
= { {"if",IF},{"else",ELSE},
   {"int",INT},{"return",RETURN},
	{"void",VOID},{"while",WHILE} };

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

		switch (state) // state에 따라
		{
		case START:
			if (isdigit(c)) // 숫자
				state = INNUM;
			else if (isalpha(c)) // 문자
				state = INID;
			else if ((c=='<') || (c=='>') || (c=='=') || (c=='!')) // <--- in_assign에 들어가는 조건 변경
				state = INASSIGN; // <, >, =, !
			else if ((c == ' ') || (c == '\t') || (c == '\n')) // 공백문자
				save = FALSE;
			else if (c == '/')
			{
				c = getNextChar();
				if (c == '*') {
					save = FALSE;
					state = INCOMMENT;
				}
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
				case EOF: // 파일을 모두 다 읽었을 경우
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
				case ',':
					currentToken = COMMA;
					break;
				case '(':
					currentToken = LPAREN;
					break;
				case ')':
					currentToken = RPAREN;
					break;
				case '[':
					currentToken = LBRAC;
					break;
				case ']':
					currentToken = RBRAC;
					break;
				case '{':
					currentToken = LCBRAC;
					break;
				case '}':
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
		case INCOMMENT: // 코멘트 닫는 부분
			save = FALSE;
			if (c == EOF)
			{
				state = DONE;
				currentToken = ENDFILE;
				fprintf(listing, "ERROR: stop before ending\n");
			}
			else if (c == '*') 
			{
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
			char t = tokenString[0];
			if (c != '=') {
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
			{ /* backup in the input */
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
	if (TRUE) {
		fprintf(listing, "\t%d: ", lineno); // 라인 넘버
		printToken(currentToken, tokenString);  // UTIL.C에 있음
	}
	return currentToken;
} /* end getToken */

