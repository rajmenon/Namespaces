#define _GNU_SOURCE
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <string.h>


// Define size of space for stack used by child process 
#define STACK_SIZE (64 * 1024)

struct arguments {
    int  verbose;
    char **command;
};


// Function to print command usage
static void usage(char *prog)
{
    fprintf(stderr, "Usage: %s [options] [cmd [arg...]]\n", prog);
    fprintf(stderr, "Options can be:\n");
    fprintf(stderr, "    -h   display this help message\n");
    fprintf(stderr, "    -v   Display verbose messages\n");
    fprintf(stderr, "    -p   new PID namespace\n");
}
 

// Function passed to the clone system call
int childFunction(void *child_args)
{
    struct arguments *args = child_args;

    if (args->verbose)
        printf(" Child: PID of child is %d\n", getpid());

    // Execute command if given
    if (args->command != NULL) {
        printf(" Child: Executing command %s ...\n", args->command[0]);
        execvp(args->command[0], &args->command[0]);
    }
    else
        exit(EXIT_SUCCESS);

    perror(" Child: execv");
    exit(EXIT_FAILURE);
}


 
int main(int argc, char *argv[])
{
    char *child_stack;
    int i, option, flags = 0;
    pid_t child;
    struct arguments args;

    args.verbose = 0;
    args.command = NULL;

    // Parse command line options and construct arguments
    // to be passed to childFunction
    while ((option = getopt(argc, argv, "+hvp")) != -1) {      
        switch (option) {
        case 'p':
            flags |= CLONE_NEWPID;
            break;
        case 'v':
            args.verbose = 1;
            break;
        case 'h':
            usage(argv[0]);
            exit(EXIT_SUCCESS);
            break;
        default:
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // Assemble command to be executed in namespace
    if(optind != argc) {
        args.command = malloc(sizeof(char *) * (argc - optind + 1));
        for (i = optind; i < argc; i++) {
            args.command[i - optind] = malloc(strlen(argv[i]) + 1);
            strcpy(args.command[i - optind], argv[i]);
        }
        args.command[argc - optind] = NULL;
    }

      if (args.verbose)
        printf("Parent: PID of parent is %d\n", getpid());
 
      // Allocate heap for child's stack
      child_stack = malloc(STACK_SIZE);
      if (child_stack == NULL) {
        perror("Parent: malloc");
        exit(EXIT_FAILURE);
    }
      
      // Clone child process
    child = clone(childFunction, child_stack + STACK_SIZE, flags | SIGCHLD, &args);
    if (child == -1) {
        perror("Parent: clone");
        exit(EXIT_FAILURE);
    }

    if (args.verbose)
        printf("Parent: PID of child is %d\n", child);
 
    // Wait for child to finish 
    if (waitpid(child, NULL, 0) == -1) {
        perror("Parent: waitpid");
        exit(EXIT_FAILURE);
    }

    if (args.verbose)
        printf("Parent: %s - Finishing up\n", argv[0]);

    exit(EXIT_SUCCESS);
}
