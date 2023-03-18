#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ,

  /* TODO: Add more token types */
  TK_DEC, TK_HEX, TK_REG, TK_NEQ, TK_AND, TK_OR, TK_POINT, TK_NEG
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
      if(i==q && stack_size(&s)==1 && returnValue!=0){
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
    //printf("Fail: Not surrounded by a matched () .\n");
    return 0;
  }
  if(returnValue==1){
    printf("Success: Surrounded by a matched () when p=%d, q=%d .\n", p, q);
    return 1;
  }
}

int find_dominant_operator(int p, int q) {
  STACK s;
  s=stack_clean(&s);
  int operator[6]={-1, -1, -1, -1, -1, -1};
  // Monocular operators are right associative
  for(int i=q; i>=p; i--){
    if(tokens[i].type==')'){
      stack_push(&s, ')');
    }
    if(tokens[i].type=='('){
      stack_pop(&s);
    }
    if(stack_size(&s)>0){
      continue;
    }
    else{
      if(tokens[i].type=='!' || tokens[i].type==TK_POINT || tokens[i].type==TK_NEG){
        operator[5]=i;
      }
    }
  }
  s=stack_clean(&s);
  // Binocular operators are left associative
  for(int i=p; i<=q; i++){
    if(tokens[i].type=='('){
      stack_push(&s, '(');
    }
    if(tokens[i].type==')'){
      stack_pop(&s);
    }
    if(stack_size(&s)>0){
      continue;
    }
    else{
      if(tokens[i].type=='*' || tokens[i].type=='/'){
        operator[4]=i;
      }
      else if(tokens[i].type=='+' || tokens[i].type=='-'){
        operator[3]=i;
      }
      else if(tokens[i].type==TK_EQ || tokens[i].type==TK_NEQ){
        operator[2]=i;
      }
      else if(tokens[i].type==TK_AND){
        operator[1]=i;
      }
      else if(tokens[i].type==TK_OR){
        operator[0]=i;
      }
    }
  }
  for(int i=0;i<6;i++){
    if(operator[i]!=-1){
      return operator[i];
    }
  }
  printf("Error: Cannot find diminant operator when p=%d, q=%d .\n", p, q);
  assert(0);
}

int eval(int p, int q) {
  if(p>q) {
    printf("Error: p>q in eval() when p=%d, q=%d .\n", p, q);
    assert(0);
  }
  else if(p==q) {
    //should be a number or register
    if(tokens[p].type==TK_DEC){
      //char *ptr;
      int ret;
      ret = strtol(tokens[p].str, NULL, 10);
      return ret;
    }
    if(tokens[p].type==TK_HEX){
      //char *ptr;
      int ret;
      ret = strtol(tokens[p].str, NULL, 16);
      return ret;
    }
    if(tokens[p].type==TK_REG){
      for(int i=0; i<8; i++){
        if(strcmp(tokens[p].str, regsl[i])==0){
          return cpu.gpr[i]._32;
        }
        if(strcmp(tokens[p].str, regsw[i])==0){
          return cpu.gpr[i]._16;
        }
      }
      for(int i=0;i<4;i++){
        if(strcmp(tokens[p].str, regsb[i])==0){
          return cpu.gpr[i]._8[0];
        }
      }
      for(int i=4;i<8;i++){
        if(strcmp(tokens[p].str, regsb[i])==0){
          return cpu.gpr[i-4]._8[1];
        }
      }
      if(strcmp(tokens[p].str, "eip")==0){
        return cpu.eip;
      }
      printf("Error: Cannot eval register in TK_REG when p=%d, q=%d .\n", p, q);
      assert(0);
    }
    printf("Error: Cannot eval in single token when p=%d, q=%d .\n", p, q);
    assert(0);
  }
  else if(check_parentheses(p, q)==1) {
    return eval(p+1, q-1);
  }
  else if(check_parentheses(p, q)==0) {
    int op = find_dominant_operator(p, q);
    int op_type = tokens[op].type;
    // Monocular operators
    vaddr_t addr;
    int result;
    switch(op_type){
      case TK_POINT:
        addr=eval(p+1,q);
        result=vaddr_read(addr,4);
        return result;
      case TK_NEG:
        result=-eval(p+1,q);
        return result;
      case '!':
        result=eval(p+1,q);
        if(result==0){
          result=1;
        }
        else{
          result=0;
        }
        return result;
    }
    // Binocular operators
    int val1 = eval(p,op-1);
    int val2 = eval(op+1,q);
    switch(op_type){
      case '+':
        return val1 + val2;
      case '-':
        return val1 - val2;
      case '*':
        return val1 * val2;
      case '/':
        if(val2==0){
          printf("Error: Val2 cannot be 0 in '/' when p=%d, q=%d .\n", p, q);
          assert(0);
        }
        return val1 / val2;
      case TK_EQ:
        return val1 == val2;
      case TK_NEQ:
        return val1 != val2;
      case TK_AND:
        return val1 && val2;
      case TK_OR:
        return val1 || val2;
      default:
        printf("Error: Invalid operator when p=%d, q=%d .\n", p, q);
        assert(0);
    }
  }
  else {
    printf("Error: Invalid expression when p=%d, q=%d .\n", p, q);
    assert(0);
  }
}

uint32_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  //TODO();
  for(int i=0; i<nr_token; i++){
    if(tokens[i].type == '*' && (i == 0 || (tokens[i - 1].type != TK_DEC && tokens[i-1].type!= TK_HEX && tokens[i-1].type!= TK_REG && tokens[i-1].type!=')')) ) 
      tokens[i].type = TK_POINT;
    if(tokens[i].type == '-' && (i == 0 || (tokens[i - 1].type != TK_DEC && tokens[i-1].type!= TK_HEX && tokens[i-1].type!= TK_REG && tokens[i-1].type!=')')) ) 
      tokens[i].type = TK_NEG;
  }
  *success = true;
  return eval(0, nr_token-1);
}
