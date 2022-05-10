# Linux_C_Programming

#### To improve

In general Poszukiwacz.c checks if stdin is a pipe file, then process some part of that file and check if loaded binary values occur for the first time.  If yes, then a structure with value and PID is sended to stdout. Program returns percent value of repetitions diveded by 10. 

Collector program requier 6 arguments:
-d <path to file with data>
-s <amount of processing values from file>
-w <amount of processing values from file by child processes>
-f <path to file with achivments for the fastest child processes>
-l <path to file with raport about every child processes birth and death>
-p <max number of child processes>
