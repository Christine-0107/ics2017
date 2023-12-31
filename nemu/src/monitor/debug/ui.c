#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);
// add declaration
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_p(char *args);
static int cmd_x(char *args);
static int cmd_w(char *args);
static int cmd_d(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  /* TODO: Add more commands */
  { "si", "si [N]; Execute N instructions step by step, and default N is 1; e.g. si 10;", cmd_si },
  { "info", "info r/w; info r means printing information about registers; info w means printing information about watchpoints;", cmd_info },
  { "p", "p EXPR; Evaluate the value of EXPR;", cmd_p },
  { "x", "x N EXPR; Evaluate the value of EXPR and take the result as the beginning memory address, and output N consecutive 4 bytes in hexadecimal format", cmd_x },
  { "w", "w EXPR; Set the watchpoint and when the value of EXPR changes, pause th program; e.g. w *0x2000;", cmd_w },
  { "d", "d N; Delete the watchpoint with sequence N;", cmd_d },  

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args) {
  uint64_t N=1;
  if(args!=NULL) {
    int flag=sscanf(args,"%ld",&N); //read args as a decimal
    if(flag<=0) {
      printf("Error: Args error in cmd_si\n");
      return 0;
    }
  }  
  cpu_exec(N);
  return 0;
}

static int cmd_info(char *args) {
  if(args==NULL) {
    printf("Error: Args cannot be NULL in cmd_info\n");
    return 0;
  }
  char c;
  int flag=sscanf(args,"%c",&c); //read args as a character
  if(flag<=0) {
    printf("Error: Args error in cmd_info\n");
    return 0;
  }
  if(c=='r') {
    //eip
    printf("$EIP \t0x%x\n", cpu.eip);
    //32
    for(int i=0;i<8;i++){
      printf("$%s \t0x%x\n", regsl[i], cpu.gpr[i]._32);
    }
    //16
    for(int i=0;i<8;i++){
      printf("$%s \t0x%x\n", regsw[i], cpu.gpr[i]._16);
    }
    //8
    for(int i=0;i<4;i++){
      printf("$%s \t0x%x\n", regsb[i], cpu.gpr[i]._8[0]);
    }
    for(int i=4;i<8;i++){
      printf("$%s \t0x%x\n", regsb[i], cpu.gpr[i-4]._8[1]);
    }
  }
  else if(c=='w') {
    print_wp();
  }
  return 0;
}

static int cmd_p(char *args) {
  char s[1000];
  int flag = sscanf(args, "%s", s);
  if(flag<=0){
    printf("Erro: Args error in cmd_p\n");
    return 0;
  }
  bool success=false;
  int ret = expr(s, &success);
  if(success==false){
    printf("Error: In evacuation .\n");
    return 0;
  }
  else{
    printf("The value is %d .\n", ret);
    printf("The unsigned value is %u .\n", ret);
    return 0;
  }
}

static int cmd_x(char *args) {
  vaddr_t addr; //starting address
  int len; //length
  char *ptr;
  len = strtol(args, &ptr, 10);
  //int flag=sscanf(args,"%d 0x%x",&len,&addr);
  if(ptr==NULL){
    printf("Error: Args error in cmd_x\n");
    return 0;
  }
  bool success;
  addr = expr(ptr,&success);
  if(success==false){
    printf("Error: Evaluation error in cmd_x\n");
    return 0;
  }
  printf("Memory from 0x%x: ",addr);
  for(int i=0;i<len;i++){
    if(i%5==0){
      printf("\n");
    }
    printf("0x%08x \t",vaddr_read(addr,4));
    addr+=4;
  }
  printf("\n");
  return 0;
}

static int cmd_w(char *args) {
  char s[1000];
  int flag = sscanf(args, "%s", s);
  if(flag<=0){
    printf("Erro: Args error in cmd_p\n");
    return 0;
  }
  WP *result = new_wp(s);
  printf("A new watchpoint has been allocated. NO=%d, exp: %s ,value=%u.\n", result->NO, result->exp, result->value);
  return 0;
}

static int cmd_d(char *args) {
  int num;
  int flag = sscanf(args, "%d", &num);
  if(flag<=0){
    printf("Erro: Args error in cmd_p\n");
    return 0;
  }
  WP *wp = find_wp(num);
  if(wp==NULL){
    printf("Nothing to delete\n");
    return 0;
  }
  bool res=free_wp(wp);
  if(res==true){
    printf("A watchpoint has been deleted. NO=%d .\n", num);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  while (1) {
    char *str = rl_gets();
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
