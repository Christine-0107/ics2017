#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stack>

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_DEC, TK_HEX, TK_REG, TK_NEQ, TK_AND, TK_OR, 
};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"0x[1-9a-fA-F][0-9a-fA-F]*", TK_HEX},	// hexadecimal
  {"0|[1-9][0-9]*", TK_DEC},		//decimal
  {"\\$(eax|ebx|ecx|edx|esp|ebp|esi|edi|ax|cx|dx|bx|sp|bp|si|di|al|cl|dl|bl|ah|ch|dh|bh|eip)", TK_REG},		//register
  {"\\(", '('},		// left
  {"\\)", ')'},		// right
  {"\\*", '*'},		// multiple
  {"\\/", '/'},		// divide
  {"\\+", '+'},         // plus
  {"\\-", '-'},		// minus
  {"==", TK_EQ},         // equal
  {"!=", TK_NEQ},	// not equal
  {"&&", TK_AND},	// and
  {"\\|\\|", TK_OR},	// or
  {"!", '!'},		// not
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        // record the token.type
        if(rules[i].token_type==TK_NOTYPE){
          break;
        }
        tokens[nr_token].type=rules[i].token_type;
        // record the token.str
        switch (rules[i].token_type) {
          case TK_DEC:
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            *(tokens[nr_token].str+substr_len)='\0';
            break;
          case TK_HEX:
            strncpy(tokens[nr_token].str, substr_start+2, substr_len-2);
            *(tokens[nr_token].str+substr_len-2)='\0';
            break;
          case TK_REG:
            strncpy(tokens[nr_token].str, substr_start+1, substr_len-1);
            *(tokens[nr_token].str+substr_len-1)='\0';
            break;
        }
        printf("Regonize token %d, type %d, str %s\n", nr_token, tokens[nr_token].type, tokens[nr_token].str);
        nr_token += 1;

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

// Todo: check parentheses
// Stack
typedef struct {
  int size;
  char element[1000];
}STACK;
STACK stack_clean(STACK *s){
  s->size=0;
  memset(s->element, 0, 1000);
  return *s;
}
int stack_size(STACK *s){
  return s->size;
}
void stack_push(STACK *s, char c){
  if(s->size==0){
    s->element[0]=c;
    s->size++;
  }
  else{
    for(int i=s->size; i>0; i--){
      s->element[i]=s->element[i-1];
    }
    s->element[0]=c;
    s->size++;
  }
}
void stack_pop(STACK *s){
  if(s->size==0){
    return;
  }
  if(s->size==1){
    s->size=0;
    return;
  }
  for(int i=0; i<s->size; i++){
    s->element[i]=s->element[i+1];
  }
  s->size--;
}


// true return 1，mismatched return -1，not surrounded by a matched () return 0
int check_parentheses(int p, int q) {
  STACK s;
  s=stack_clean(&s);
  int returnValue=-2;
  // not surrounded by a matched ()
  if(tokens[p].type!='(' || tokens[q].type!=')') {
    returnValue=0;
  }
  else { // surrounded by ()
    stack_push(&s, '(');
  }
  for(int i=p+1; i<=q; i++) {
    if(tokens[i].type=='(') {
      stack_push(&s, '(');
    }
    if(tokens[i].type==')') {
      if(stack_size(&s)<=0) {
        printf("Error: Missing '(' .\n");
        return -1;
      }
      if(i!=q && stack_size(&s)==1){
        returnValue=0;
      }
      if(i==q && stack_size(&s)==1){
        returnValue=1;
      }
      stack_pop(&s);
    }
  }
  if(stack_size(&s)!=0){
    printf("Error: Missing '(' .\n");
    return -1;
  }
  if(returnValue==0){
    printf("Fail: Not surrounded by a matched () .\n");
    return 0;
  }
  if(returnValue==1){
    printf("Success: Surrounded by a matched () .\n");
    return 1;
  }
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  TODO();

  return 0;
}
