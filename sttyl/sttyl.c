/* sttyl.c 
 * CSCI E-28 HW3
 * author:    Frank O'Connor
 * purpose:   allows user to print current tty settings, or make changes
 *            to tty settings.
 * args:      passing the name of a tty flag sets that bit on, passing
 *            '-' preceeding the flag name sets the bit off. Passing the
 *            name of a control char followed by a char sets the command 
 *            to be performed by the specified char. 
 *            passing no args prints the current tty settings.
 *            multiple flag name may be passed.
 * action:    if no args are passed, then print tty settings.
 *            if args are passed, then parse args, match arg to values in
 *            tables and set the appropriate flag in tty settings are 
 *            specified by the arg.
 * compiling: to compile this, use
 *            gcc sttyl.c -o sttyl
 */

#include   <stdio.h>
#include   <stdlib.h>
#include   <string.h>
#include   <ctype.h> 
#include   <termios.h>
#include   <unistd.h>
#define BCKSPC 127        /* ascii value for backspace */
#define CTRLCUTOFF 32     /* ascii value below which are ctrl characters */

typedef struct {
    char    *fl_name;     /* the name of the flag to lookup */
    tcflag_t fl_value;    /* flag value */
} flag_info_t;

void parseargs(int argc, char **argv, struct termios *ttyp );
void set_ctrl_char(struct termios *ttyp, int ccindex, char* newval);
void cmp_arg_to_flags(int *ttyp_flag, flag_info_t flag_table[], 
                       int *found, char* argstr);
void print_all_flags(struct termios *ttyp);
void print_ctrl_chars(char* carray, flag_info_t flag_table[]);
void print_flagset(int thevalue, flag_info_t flag_table[]);
void print_baudrate(int thespeed);

int main(int argc, char **argv)
{
    struct termios ttyinfo;    /* struct to hold tty info */

    /* get the current tty settings and store local copy in termios struct*/
    if (tcgetattr(STDIN_FILENO , &ttyinfo) == -1){
        perror("sttyl: standard input");
        exit(EXIT_FAILURE);
    }
     
    if (argc > 1) {
        /* parse cmd line args and set vars */
        parseargs(argc, argv, &ttyinfo);
        
        /* set the tty settings to altered the values in termios struct */
        if (tcsetattr(STDIN_FILENO, TCSANOW, &ttyinfo) != 0){    
            perror("sttyl: standard input");
            exit(EXIT_FAILURE);
        }
    } else {
        /* print tty info */
        print_baudrate(cfgetospeed(&ttyinfo));
        print_all_flags(&ttyinfo);
    }
    return EXIT_SUCCESS;
}

/* Table definition - start */
/* table def for input modes - c_iflag */
flag_info_t input_flags[] = 
{
    { "brkint", BRKINT },
    { "inpck",  INPCK },
    { "icrnl",  ICRNL },
    { "ixany",  IXANY },
    { NULL ,    0 }
};

/* table def for output modes - c_oflag */
flag_info_t output_flags[] = 
{
    { "onlcr",  ONLCR },
    { "olcuc",  OLCUC },
    { NULL,     0}
};

/* table def for control modes - c_cflag */
flag_info_t control_flags[] = 
{
    { "parenb", PARENB },    
    { "hupcl",  HUPCL },
    { "cread",  CREAD },
    { NULL,     0 }
};

/* table def for local modes - c_lflag */
flag_info_t local_flags[] = 
{
    { "isig",   ISIG },
    { "icanon", ICANON } ,
    { "iexten", IEXTEN }, 
    { "echo",   ECHO }, 
    { "echoe",  ECHOE }, 
    { "echok",  ECHOK }, 
    { NULL,     0 }
};

/* table def for control chars - c_cc */    
flag_info_t control_chars[] = 
{
    { "intr",   VINTR },
    { "erase",  VERASE },
    { "kill",   VKILL },
    { "start",  VSTART },
    { "stop",   VSTOP },
    { "werase", VWERASE },
    { NULL,     0 }
};
/* Table definition - end */

void parseargs(int argc, char **argv, struct termios *ttyp)
/* checks cmd line args and tries to match with values from the tables.
 * if match found to arg, then the value is set as appropriate in the 
 * local copy of termios struct.
 * args: argc - num of cmd line args.
 *       **argv - cmd line args.
 *       *ttyp - ptr to local copy of struct with termios values.
 * return: nothing
 */
{
    int i;            /* outer loop counter */
    for (i = 1; i < argc; i++) {
        int found = 0;        /* found a match for the arg */
        int j;                /* inner loop counter */

        /* check if arg matches ctrl char from table */
        for (j=0; control_chars[j].fl_name != NULL && found != 1; j++) {
            if (strcmp(argv[i], control_chars[j].fl_name) == 0) {
                if (i+1 < argc) {
                    i++;
                    /* if matches then set the ctrl char to the next arg */
                    set_ctrl_char(ttyp, control_chars[j].fl_value, argv[i]);
                    found = 1;
                } else {
                    /* no arg arg exists */
                    fprintf(stderr, "sttyl: missing argument to `%s'\n", 
                        control_chars[j].fl_name);
                    exit(EXIT_FAILURE);
                }
            }
        }
        
        /* check arg against flag tables */
        cmp_arg_to_flags((int *)&(ttyp->c_iflag), input_flags, 
                           &found, argv[i]);
        cmp_arg_to_flags((int *)&(ttyp->c_oflag), output_flags, 
                           &found, argv[i]);
        cmp_arg_to_flags((int *)&(ttyp->c_lflag), local_flags, 
                           &found, argv[i]);
        cmp_arg_to_flags((int *)&(ttyp->c_cflag), control_flags, 
                           &found, argv[i]);

        /* if didn't find, then arg is not valid */ 
        if (found == 0) {
            fprintf(stderr, "invalid argument `%s'\n", argv[i]);
            exit(EXIT_FAILURE);
        }
    }
}

void set_ctrl_char(struct termios *ttyp, int ccindex, char* newcharval)
/* vaidate and set the char value for the specified control char.
 * check each bit pattern and display descriptive title
 * args: *ttyp - ptr to local copy of struct with termios values.
 *       ccindex - index for specified ctrl char in its table.
 *       newcharval - value to set ctrl char to.
 * return: nothing
 */
{
    /* accepting ctrl btn + char as '^' and the character. 
       char with '^' must be after @ in ascii table, i.e. from 'A' on */
    if (newcharval[0] == '^' && newcharval[1] > '@') {
        /* char with ctrl btn press is always upper case, so convert */
        newcharval[1] = toupper(newcharval[1]); 
        /* convert char to it's ctrl-btn equivalent, and set*/
        ttyp->c_cc[ccindex] = (newcharval[1] - 'A' + 1);
    } else if (newcharval[0] == '^' && newcharval[1] == '?') {
        /* special case for backspace, which is entered as '^?' */
        ttyp->c_cc[ccindex] = BCKSPC;
    } else {
        /* for none-ctrlbtn char */
        ttyp->c_cc[ccindex] = newcharval[0];
    }
}

void cmp_arg_to_flags(int *ttyp_flag, flag_info_t flag_table[], 
                       int *found, char* argstr)
/* compare passed arg to values in specified table to find a match.
 * if found, set the value of flag specified by ptr ttyp_flag.
 * args: *ttyp_flag - ptr to flag in local copy of termios struct.
 *       flag_table[] - specific table to look through.
 *       *found - indication if match is found.
 *       argstr - arg to find match for.
 * return: nothing
 */
{
    /* if already found don't do comparison */
    if (*found != 1) {
        int switch_on = 1;    /* '-' determines on/off */
        if (argstr[0] == '-') {
            /* if '-', then flag is rest of string, and turn flag off */
            argstr++;
            switch_on = 0;
        }
        int i;
        /* compare arg to flags in specified table */
        for (i=0; flag_table[i].fl_name != NULL && *found != 1; i++) {
            if (strcmp(argstr, flag_table[i].fl_name) == 0) {
                /* found matching flag, set on/off as appropriate */
                if (switch_on == 1) {
                    *ttyp_flag |= flag_table[i].fl_value;
                } else {
                    *ttyp_flag &= ~(flag_table[i].fl_value);
                }
                *found=1;
            }
        }
    }
}

void print_all_flags(struct termios *ttyp)
/* prints out the values for all flags and ctrl chars. 
 * args: *ttyp - ptr to local copy of struct with termios values.
 * return: nothing
 */
{
    /* printed all flags, in similar format to that
       shown in assignment */
    print_flagset(ttyp->c_cflag, control_flags);
    printf("\n");
    print_ctrl_chars((char *)ttyp->c_cc, control_chars);
    printf("\n");
    print_flagset(ttyp->c_iflag, input_flags);
    print_flagset(ttyp->c_oflag, output_flags);
    printf("\n");
    print_flagset(ttyp->c_lflag, local_flags);
    printf("\n");
}

void print_ctrl_chars(char* carray, flag_info_t flag_table[])
/* loop through the specified flag table and print the name
 * of the var and the char currently set for it with it. 
 * args: carray - ptr to cc_flag from local copy of termios struct.
 *       flag_table - value of flagtable to loop through.
 * return: nothing
 */
{
    int i;    /*loop counter */
    for (i=0; flag_table[i].fl_name != NULL; i++) {
        if ( carray[flag_table[i].fl_value] < CTRLCUTOFF){
            /* if char is below cutoff it is a ctrlbtn+char, so prepend '^' */
            printf("%s = ^%c; ", flag_table[i].fl_name, 
                    carray[flag_table[i].fl_value] + 'A' - 1);
        } else if (carray[flag_table[i].fl_value] == BCKSPC) {
            /* special case for backspace, printed as '^?' */
            printf("%s = ^?; ", flag_table[i].fl_name);
        } else {
            /* print non ctrl-btn char */
            printf("%s = %c; ", flag_table[i].fl_name, 
                    carray[flag_table[i].fl_value]);
        }
    }
}

void print_flagset(int flag, flag_info_t flag_table[])
/* loop through the specified flag table and print the name
 * of the var if it is set, or a '-' followed by the var name 
 * if it is not set.
 * args: flag - value of flag in termios struct.
 *       flag_table - value of flagtable to loop through.
 * return: nothing
 */
{
    int i;    /* loop counter */
    for (i=0; flag_table[i].fl_name != NULL; i++) {
        /* do bitwise and with mask to see if set */
        if (!(flag & flag_table[i].fl_value)) {
            /* prepend with '-' if it is off */
            printf("-");
        }
        printf("%s ", flag_table[i].fl_name);
    }
}

void print_baudrate(int baudrate)
/* converts and prints out the passed baud rate speed.
 * args: baudrate - int value of baudrate.
 * return: nothing
 */
{
    /* Note: I don't particularlly like this switch statement method
       of converting, I feel this should be parsed. But this is the 
       example given in class so I assume it is the correct way, also
       these are the only values defined in termios man page. */
    printf("speed ");
    switch (baudrate) {
        case B0:     printf("0");       break;
        case B50:    printf("50");      break;
        case B75:    printf("75");      break;
        case B110:   printf("110");     break;
        case B134:   printf("134.5");   break;
        case B150:   printf("150");     break;
        case B200:   printf("200");     break;
        case B300:   printf("300");     break;
        case B600:   printf("600");     break;
        case B1200:  printf("1200");    break;
        case B1800:  printf("1800");    break;
        case B2400:  printf("2400");    break;
        case B4800:  printf("4800");    break;
        case B9600:  printf("9600");    break;
        case B19200: printf("19200");   break;
        case B38400: printf("38400");   break;
        default:     printf("Fast");    break;
    }
    printf(" baud; ");
}
