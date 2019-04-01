

#define _BSD_SOURCE
#define BUFF_SIZE 1024
#define NUM_JOBS 512
#define NUM_BUILTINS 2
#define MAX_ARGS 512
#define MAX_ARG 128
#define PATH_MAX 128

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
#include <memory.h>


// declarations
void shellExecute(int isBackGround, int argc, char **argv);

typedef struct Job {
    int pid;
    int argc;
    char **argv;
} Job;

Job *jobs[NUM_JOBS];
int lastJob;
char cwd[PATH_MAX];

char *builtinsNames[] = {"cd", "exit"};


// check bad alloc
void checkBadAlloc(void *ptr) {
    if (!ptr) {
        fprintf(stderr, "Bad alloc\n");
    }
}

// free argv
void freeArgv(int argc, char **argv) {
    int i = 0;
    for (; i < argc; ++i) {
        free(argv[i]);
    }
    free(argv);
}

// free job
void freeJob(Job *job) {
    if (job != NULL) {
        freeArgv(job->argc, job->argv);
        free(job);
    }
}

// cd implementation
void cdImp(char **argv) {
    // cd and cd ~
    if (argv[1] == NULL || !strcmp(argv[1], "~")) {
        getcwd(cwd, sizeof(cwd));
        chdir(getenv("HOME"));
        return;
    }

    //cd -
    if (!strcmp(argv[1], "-")) {
        chdir(cwd);
        return;
    }

    // regular cd and error in cd
    getcwd(cwd, sizeof(cwd));
    if (chdir(argv[1])) {
        usleep(1);
        fprintf(stderr, "Error in cd\n");
        usleep(1);
    }
}

// exit implementation
void exitImp(char **argv) {
    // free all jobs and lines before exit
    int i = 0;
    for (; i < lastJob; ++i) {
        freeJob(jobs[i]);
    }

    exit(0);
}

// builtinsImps
void (*builtinsImps[])(char **) = {&cdImp, &exitImp};


// compress jobs
void compressJobs() {
    Job *tempJobs[NUM_JOBS + 1];

    // init temp jobs
    int i = 0;
    for (i; i < NUM_JOBS + 1; ++i) {
        tempJobs[i] = NULL;
    }

    // move active jobs to temp
    int l = 0, j = 0;
    for (; l < NUM_JOBS; ++l) {
        if (jobs[l] != NULL) {
            tempJobs[j] = jobs[l];
            j++;
        }
    }

    // move from temp to jobs
    int k = 0;
    while (tempJobs[k] != NULL) {
        jobs[k] = tempJobs[k];
        k++;
    }
    lastJob = k - 1;
}


// shell read
char *shellRead() {
    int size = BUFF_SIZE;
    char *line = (char *) malloc(sizeof(char) * BUFF_SIZE);
    checkBadAlloc(line);

    int i = 0;
    while ("os is the best") {
        // realloc if needed
        if (i == size) {
            size += BUFF_SIZE;
            line = (char *) realloc(line, sizeof(char) * size);
            checkBadAlloc(line);
        }

        // read one char
        char c = getchar();

        // put special sign $ at the end
        if (c == EOF || c == '\n') {
            line[i] = '$';
            break;
        } else {
            line[i] = c;
        }
        ++i;
    }

    return line;
}

// shell parse
void shellParse(char *line) {
    int argc = 0;
    int isPath = 0;
    int isBackGround = 0;
    char **argv = (char **) malloc(sizeof(char *) * MAX_ARGS);
    checkBadAlloc(argv);

    // init argv
    int k = 0;
    for (k; k < MAX_ARGS; ++k) {
        argv[k] = NULL;
    }

    int i = 0;
    // parse arg each iteration
    while (line[i] != '$') {
        char *arg = (char *) malloc(sizeof(char) * MAX_ARG);
        checkBadAlloc(arg);

        // pass spaces
        while (line[i] != '$' && line[i] == ' ') {
            ++i;
        }

        int j = 0;
        // arg itself
        while (line[i] != '$' && (line[i] != ' ' || isPath)) {
            // case path is like "a a"
            if (line[i] == '"') {
                isPath = !isPath;
            } else {
                arg[j] = line[i];
                ++j;
            }
            ++i;
        }

        // end of arg
        arg[j] = '\0';
        // put arg in args at argc
        argv[argc] = arg;

        // pass spaces
        while (line[i] != '$' && line[i] == ' ') {
            ++i;
        }
        ++argc;
    }

    // set is background
    if (!strcmp(argv[argc - 1], "&")) {
        isBackGround = 1;
        free(argv[argc - 1]);
        argv[argc - 1] = NULL;
        --argc;
    }

    shellExecute(isBackGround, argc, argv);
}

// shell run
void shellRun() {
    char *line = shellRead();
    shellParse(line);
    free(line);
}

// shell builtins
void shellBuiltins(int funcIndex, char **argv) {
    // for builtins print this pid
    printf("%d\n", getpid());

    // call builtin
    (*builtinsImps[funcIndex])(argv);
}

// shell jobs
void shellJobs() {
    int i = 0;
    for (; i < lastJob; ++i) {
        if (jobs[i] != NULL) {
            // if job finished, put NULL
            if (waitpid(jobs[i]->pid, NULL, WNOHANG != 0)) {
                freeJob(jobs[i]);
                jobs[i] = NULL;

                // if job didn't finish, display it
            } else {
                // print pid
                printf("%d ", jobs[i]->pid);

                // print all args except the last one
                int j;
                for (j = 0; j < jobs[i]->argc - 1; ++j) {
                    printf("%s ", jobs[i]->argv[j]);
                }

                // print the last arg
                printf("%s\n", jobs[i]->argv[jobs[i]->argc - 1]);
            }
        }
    }
}

// shell regular
void shellRegular(int isBackGround, int argc, char **argv) {
    pid_t childPid = fork();

    // if parent, print child's pid
    if (childPid > 0) {
        printf("%d\n", childPid);
    }

    // error in fork
    if (childPid < 0) {
        fprintf(stderr, "Error in fork\n");
    }

        // child
    else if (childPid == 0) {
        if (execvp(argv[0], argv) == -1) {
            usleep(1);
            fprintf(stderr, "Error in system call\n");
            usleep(1);
        }
        exit(EXIT_FAILURE);

        // parent
    } else {
        // if not background, wait
        if (!isBackGround) {
            freeArgv(argc, argv);
            waitpid(childPid, NULL, WUNTRACED);

            // if background, add job to jobs
        } else {
            Job *newJob = (Job *) malloc(sizeof(Job));
            newJob->argv = argv;
            newJob->argc = argc;
            newJob->pid = childPid;

            jobs[lastJob] = newJob;
            ++lastJob;

            // if array is full, compress
            if (lastJob == NUM_JOBS) {
                compressJobs();
            }
        }
    }
}

// shell execute
void shellExecute(int isBackGround, int argc, char **argv) {
    // try run builtin
    int i = 0;
    for (; i < NUM_BUILTINS; ++i) {
        if (!strcmp(argv[0], builtinsNames[i])) {
            shellBuiltins(i, argv);
            freeArgv(argc, argv);
            return;
        }
    }

    // try run jobs
    if (!strcmp(argv[0], "jobs")) {
        shellJobs();
        freeArgv(argc, argv);
        return;
    }

    // run regular
    shellRegular(isBackGround, argc, argv);
}


int main() {
    // init
    int i = 0;
    for (; i < NUM_JOBS; ++i) {
        jobs[i] = NULL;
    }
    lastJob = 0;

    // run while os is the best (forever)
    while ("os is the best") {
        printf("> ");
        shellRun();
    }

    return 0;
}


