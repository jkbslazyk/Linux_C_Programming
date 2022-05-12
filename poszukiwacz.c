#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

struct __attribute__((__packed__)) record {
    unsigned short value;
    pid_t pid;
};
typedef struct record Record;

int nValidation(int value, const char *unit);

int main(int argc, char *argv[]) {

    if (argc != 2) {
        exit(11);
    }

    struct stat sb;
    if (fstat(1, &sb) == -1) {
        exit(12);
    }

    if ((sb.st_mode & S_IFMT) != S_IFIFO) {
        exit(13);
    }

    char *pEnd;
    int n = strtol(argv[1], &pEnd, 10);
    n = nValidation(n, pEnd);
    if (n < 1) {
        exit(14);
    }

    int numbers[65536] = {0};
    Record rec;
    unsigned short value;
    int newValue = 0;

    for (int i = 0; i < n; i++) {

        if (read(0, &value, 2) <= 0) {
            exit(16);
        }

        if (numbers[value] == 0) {
            numbers[value]++;
            newValue++;
            rec.value = value;
            rec.pid = getpid();
            if (write(1, &rec, sizeof(rec)) == -1) {
                exit(17);
            }

        }
    }

    int ret = 1 + ((n - newValue) * 10 - 1.) / (float) n;
    return ret;

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

