Input Format:
    value of nw followed by value of nk followed by value of kw and then kr and then muCS and muRem

    nw nr kw kr muCS muRem

Input file should be named as "inp-params.txt" and should be placed in the same folder as of the code.

To compile the program:
    Open terminal in this folder and run the command
    g++ -pthread filename.cpp -o ./filename
    Ex: g++ -pthread rw-CS22BTECH11050.cpp -o ./rw-CS22BTECH11050
        g++ -pthread frw-CS22BTECH11050.cpp -o ./frw-CS22BTECH11050

To execute :
    Run the command
    ./filename
    Ex : ./rw-CS22BTECH11050
         ./frw-CS22BTECH11050

Output Format :
    Each of the two codes produces two output files

    Output of rw-CS22BTECH11050.cpp
        “RW-log.txt” which contains the request time,entry time,exit time of all the iterations of the threads.
        “Average-RW.txt” which contains the average waiting time of each Reader thread and of each Writer Thread and also average of all the reader threads and the writer threads.

    Output of frw-CS22BTECH11050.cpp
        “FairRW-log.txt” which contains the request time,entry time,exit time of all the iterations of the threads.
        “Average-FairRW.txt” which contains the average waiting time of each Reader thread and of each Writer Thread and also average of all the reader threads and the writer threads.



