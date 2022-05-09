#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/ioctl.h>

#define MSGSIZE 16384
#define MAXVALUE 65535
#define MAXPIPE 45000

#define GETOPT 11
#define OPEN 12
#define CLOSE 13
#define READ 14
#define WRITE 15
#define LSEEK 16
#define PIPE 17
#define FCNTL 18
#define FORK 19
#define CLOCK_GETTIME 20
#define SNPRINTF 21
#define DUP2 22
#define EXECL 23

struct __attribute__((__packed__)) record {
    unsigned short value;
    pid_t pid;
};

typedef struct record Record;

int nValidation(int value, const char *unit);

void fillWithZeros(int fdFile);

void saveSuccess(int fdFile, unsigned short value, pid_t pid);

void saveEvent(int fdFile, char *type, pid_t pid);

void createChild(int *input, int *output, char *path, char *name, char *argument);

int relayData(int fdFile, int *input, int volume);

void closePipe(int* input);

int relayData(int fdFile, int *input,  int volume);

int main(int argc, char *argv[]) {

    //ETAP1 - GETOPT
    int opt;
    int d = 0, s = 0, w = 0, f = 0, l = 0, p = 0;
    int sWolumnen, nChild, currChildren;
    int dFile, fFile, lFile;
    char *pEnd, *wBlok;

    while ((opt = getopt(argc, argv, ":d:s:w:f:l:p:")) != -1) {

        switch (opt) {
            case 'd':
                d++;
                if (access(optarg, F_OK) != 0) {
                    perror("Source File does not exists (-d)");
                    exit(GETOPT);
                }
                dFile = open(optarg, O_RDONLY);
                if (dFile == -1) {
                    perror("Opening source file failed (-d)");
                    exit(OPEN);
                }
                break;
            case 's':
                s++;
                sWolumnen = strtol(optarg, &pEnd, 10);
                sWolumnen = nValidation(sWolumnen, pEnd);
                if (!sWolumnen) {
                    perror("Inappropriate value of wolumen (-s).");
                    exit(GETOPT);
                }
                break;
            case 'w':
                w++;
                wBlok = optarg;
                if (!wBlok) {
                    exit(GETOPT);
                }
                break;
            case 'f':
                f++;
                fFile = open(optarg, O_RDWR | O_CREAT | O_TRUNC);
                fillWithZeros(fFile);
                if (fFile == -1) {
                    perror("Opening successes file failed (-f)");
                    exit(OPEN);
                }
                break;
            case 'l':
                l++;
                lFile = open(optarg, O_WRONLY | O_CREAT | O_TRUNC);
                if (lFile == -1) {
                    perror("Opening log file failed (-l)");
                    exit(OPEN);
                }
                break;
            case 'p':
                p++;
                nChild = strtol(optarg, &pEnd, 10);
                if (nChild < 1 || *pEnd != '\0') {
                    perror("Value of child has to be positive");
                    exit(GETOPT);
                }
                currChildren = nChild;
                break;
            case '?':
                printf("Unknown option %c\n", optopt);
                exit(GETOPT);
            case ':':
                printf("Option %c requires argument\n", optopt);
                exit(GETOPT);
        }
    }

    if (!(d && s && w && f && l && p) || d + s + w + f + l + p != 6) {
        perror("Inappropriate flags (you have to use all, but only once).");
        exit(GETOPT);
    }

    // ETAP 2 - PIPE'Y + WSTEPNI POTOMKOWIE
    int inputChild[2];
    int outputChild[2];
    pid_t proc_fork;

    if (pipe(inputChild) < 0 || pipe(outputChild) < 0) {
        perror("Creating pipes failed");
        exit(PIPE);
    }

    if (fcntl(outputChild[0], F_SETFL, fcntl(outputChild[0], F_GETFL) | O_NONBLOCK) < 0 ||
        fcntl(inputChild[1], F_SETFL, fcntl(inputChild[1], F_GETFL) | O_NONBLOCK) < 0) {
        perror("Setting operations as non-blocking failed");
        exit(FCNTL);
    }

    for (int i = 0; i < nChild; i++) {
        proc_fork = fork();

        if (proc_fork < 0) {
            perror("Creating child process with fork function failed.");
            exit(FORK);

        } else if (proc_fork == 0) {

            saveEvent(lFile, "born", getpid());

            createChild(inputChild, outputChild, "poszukiwacz.out", "poszukiwacz", wBlok);

        }
    }

    //ETAP3 - CYKL
    pid_t pidtemp, new_fork;
    int suc = 0;
    int failedRead = 1;
    int failedWaitpid = 1;
    Record rec;
    int status;
    int flaga = 0;

    while (1) {

        if (sWolumnen > 0) {
            sWolumnen = relayData(dFile, inputChild, sWolumnen);
        }

        if (sWolumnen==0 && flaga==0){
            if(close(inputChild[1])==-1){
                perror("Closing pipe failed");
                exit(CLOSE);
            }
            flaga=1;
        }

        while (1) {

            if (read(outputChild[0], &rec, sizeof(rec)) > 0) { //odbieram od dzieciaka strukture

                saveSuccess(fFile, rec.value, rec.pid);
                suc++;
                failedRead = 0;

            } else {
                break;
            }
        }

        if (currChildren <= 0) {
            printf("No more children, time to finish - no fails");
            break;
        }

        while (1) {
            pidtemp = waitpid(-1, &status, WNOHANG);
            if (pidtemp <= 0) {
                break;
            }
            else {
                currChildren--;
                saveEvent(lFile, "destroyed", pidtemp);
                if (WEXITSTATUS(status) <= 10 && (MAXVALUE * 3) / 4 > suc) {
                    currChildren++;
                    failedWaitpid = 0;
                    new_fork = fork();
                    if (new_fork == 0) {

                        saveEvent(lFile, "born", getpid());

                        createChild(inputChild, outputChild, "poszukiwacz.out", "poszukiwacz", wBlok);

                    }
                    else if (proc_fork < 0) {
                        perror("Creating child process with fork function failed.");
                        exit(FORK);

                    }
                }
            }
        }

        if (failedWaitpid && failedRead) {
            nanosleep((const struct timespec[]) {{0, 480000000L}}, NULL);
        }

        failedWaitpid = 0;
        failedRead = 0;

    }

    printf("\n");

    if (close(fFile) == -1 || close(lFile) == -1 || close(dFile) == -1) {
        perror("Closing files failed ");
        exit(CLOSE);
    }

}

int nValidation(int value, const char *unit) {

    if (value < 1) {
        return 0;
    }

    if (*unit != '\0') {
        if (*(unit + 2) != '\0' && *(unit + 1) != 'i')
            return 0;
        switch (*(unit)) {
            case 'k':
            case 'K':
                value *= 1024;
                break;
            case 'm':
            case 'M':
                value *= 1024 * 1024;
                break;
            default:
                return 0;
        }
    }
    return value;
}

void saveSuccess(int fdFile, unsigned short value, pid_t pid) {

    if (lseek(fdFile, value * sizeof(pid_t), SEEK_SET) == -1) {
        perror("Lseek() function using fail (saveSuccess())");
        exit(WRITE);
    }

    pid_t temp;

    if (read(fdFile, &temp, sizeof(pid_t)) == -1) {
        perror("Reading data from success fail failed (relayData())");
        exit(READ);
    }


    if (lseek(fdFile, value * sizeof(pid_t), SEEK_SET) < 0) {
        perror("Lseek() function using fail (saveSuccess())");
        exit(WRITE);
    }

    if (temp == 0) {
        if(write(fdFile, &pid, sizeof(pid_t)) == -1) {
            perror("Writing to file failed (saveSuccess())");
            exit(WRITE);
        }
    }

}

void fillWithZeros(int fdFile) {

    pid_t zeros[MAXVALUE] = {0};

    if (write(fdFile, zeros, sizeof(zeros)) == -1) {
        perror("Filling file with zeros failed (fillWithZeros()");
        exit(WRITE);
    }

}

void saveEvent(int fdFile, char *type, pid_t pid) {

    struct timespec time;
    char buff[100];

    if (clock_gettime(CLOCK_MONOTONIC, &time) == -1) {
        perror("Getting CLOCK_MONOTONIC value (saveEvent())");
        exit(CLOCK_GETTIME);
    }

    int bytes = snprintf(buff, sizeof(buff), "%ld.%ld %s %d\n", time.tv_sec, time.tv_nsec, type, pid);
    if (bytes <= 0) {
        perror("Writing to buffor with snprintf() function failed (saveEvent())");
        exit(SNPRINTF);
    }

    if (write(fdFile, buff, bytes) < 0) {
        perror("Writing to file failed (saveEvent())");
        exit(WRITE);
    }

}

int relayData(int fdFile, int *input,  int volume) {

    char bufin[MSGSIZE];
    int bytesAvailable;

    ioctl(input[1], FIONREAD, &bytesAvailable);

    if (MSGSIZE >= 2 * volume && MAXPIPE > bytesAvailable && bytesAvailable==0) {

        if (read(fdFile, &bufin, 2 * volume) == -1) {
            perror("Reading data from source file failed (relayData())");
            exit(READ);
        }
        if (write(input[1], bufin, 2 * volume) == -1) {
            perror("Sending data to children failed (relayData())");
            exit(WRITE);
        }
        volume = 0;

    }
    else if (MSGSIZE < 2 * volume && MAXPIPE > bytesAvailable && bytesAvailable==0) {

        if (read(fdFile, &bufin, sizeof(bufin)) == -1) {
            perror("Reading data from source fail failed (relayData())");
            exit(READ);
        }
        if (write(input[1], bufin, sizeof(bufin)) == -1) {
            perror("Sending data to children failed (relayData())");
            exit(WRITE);
        }
        volume = volume - (int) MSGSIZE / 2;
    }

    return volume;
}

void createChild(int *input, int *output, char *path, char *name, char *argument) {

    if (dup2(input[0], 0) == -1 || dup2(output[1], 1) ==
                                   -1) {//to co do dziecka wchodzi jak input/read || to co dziecko przesyla z powrotem jako output
        perror("Using dup2() function failed.");
        exit(DUP2);
    }

    if (close(output[0]) == -1 || close(input[1]) == -1 || close(input[0]) == -1 || close(output[1]) == -1) {
        perror("Closing pipes failed :D");
        exit(CLOSE);
    }

    if (execl(path, name, argument, (char *) NULL) == -1) {
        perror("Execl() function failed");
        exit(EXECL);
    }
}
