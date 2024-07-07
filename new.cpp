#include <iostream>
#include <random>
#include <pthread.h>
#include <semaphore.h>
#include <chrono>
#include <fstream>
#include <sys/types.h>
#include <unistd.h>
#include <ctime>
#include <iomanip>
#include <vector>
#include <sstream>

using namespace std;

// global variables to store input
int nw = 0, nr = 0, kw = 0, kr = 0, muCS = 1, muRem = 1;

default_random_engine generator(chrono::system_clock::now().time_since_epoch().count());

// semaphores
sem_t lock1; // for maintaining count of writers
sem_t lock2; // for maintaining count of readers
sem_t writelock;// for accessing the shared resource
sem_t canread;//for checking if readers can acquire the resource
sem_t file;// for accessing the output file
sem_t time_lock;// for calculating average time of each thread


ofstream output("RW-log.txt");// Output file to log the time stamps
ofstream output2("Average-RW.txt");// Output file to log the average time taken for each thread

// Number of readers and writers
int readers=0;
int writers=0;

// Vectors to store the average times
vector <chrono::microseconds> Readers;
vector <chrono::microseconds> Writers;


// Function to calculate present time
string Time(){
    auto now = std::chrono::system_clock::now();
    auto now_ms = std::chrono::time_point_cast<std::chrono::microseconds>(now);
    auto epoch = now_ms.time_since_epoch();
    auto current_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(epoch);

    // Extract time components
    std::time_t current_time = std::chrono::system_clock::to_time_t(now);
    auto current_tm = *std::localtime(&current_time);
    int hours = current_tm.tm_hour;
    int minutes = current_tm.tm_min;
    int seconds = current_tm.tm_sec;

    // Format the time
    std::ostringstream oss;
    oss << setfill('0') << setw(2) << hours << ':'
        << setfill('0') << setw(2) << minutes << ':'
        << setfill('0') << setw(2) << seconds << ':'
        << setfill('0') << setw(6) << (current_microseconds.count() % 1000000);
    return oss.str();

}

// Reader function
void *Reader(void *parameter)
{
    int id=1;
    //int id=gettid();// To get thread id
    chrono::microseconds average(0);
    for (int i = 0; i < kr; i++)
    {
        // Acquiring file semaphore to write the request time into the output file
        sem_wait(&file);
        // Time when request took place
        auto reqTime= chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
        output << i+1 << "th CS Request by Reader Thread " << id << " at " << Time() << endl;
        sem_post(&file);// Signaling the file semaphore

        //Checking if the reader can gain access to the resource
        sem_wait(&canread);
        // wait for lock2 semaphore to get access to readers count
        sem_wait(&lock2);
        readers++;// update current readers
        if(readers==1){
            sem_wait(&writelock);// If this is the first reader acquire writeLock
        }
        sem_post(&lock2);// signal the lock2 semaphore
        sem_post(&canread);//signaling the canread semaphore


        /*critical section*/

        //To gain access to write into output file
        sem_wait(&file);
        // Time of entry
        auto entryTime= chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
        output << i+1 << "th CS Entry by Reader Thread " << id << " at " << Time() << endl;
        sem_post(&file);// Signal the semaphore so that any other thread can perform writing

        // Finding sum of time taken for the thread for entering the critical from request time
        average=average+chrono::microseconds(entryTime-reqTime);
        
        // Using exponential distribution for CS time
        exponential_distribution<double> distributionCS(1.0 / muCS);
        double randCST = distributionCS(generator);        
        sleep(randCST/1000);// Performing CS

        // Waiting for lock2 to change the readers count 
        sem_wait(&lock2);
        readers--; // Decrementing the number of current readers
        if(readers==0){
            sem_post(&writelock);// If no reader is currently in critical section then release the writelock
        }
        sem_post(&lock2);


         // For file access
        sem_wait(&file);
        output << i+1 << "th CS Exit by Reader Thread " << id << " at " << Time() << endl;
        sem_post(&file);
        
        // exponential distribution for remainder section time
        exponential_distribution<double> distributionRem(1.0 / muRem);
        double randRemTime = distributionRem(generator);
        sleep(randRemTime/1000);// performing remainder section
        // simulate a thread executing in Remainder Section*/
    }
    // calculating the average time of the kr threads
    average=average/kr;

    // Waiting to gain access to the Readers vector and the output file
    sem_wait(&time_lock);
    Readers.push_back(average);
    output2<< "Average time for Reader thread " <<id<<" is "<<average.count()<<endl;
    sem_post(&time_lock);

    return NULL;
}

void *Writer(void *parameter)
{
    int id=1;
    //int id=gettid(); // To get thread id
    chrono::microseconds average(0);
    for (int i = 0; i < kw; i++)
    {
         // Acquiring file semaphore to write the request time into the output file
        sem_wait(&file);
        // Time when request took place
        auto reqTime= chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
        output << i+1 << "th CS request by Writer Thread " << id << " at " << Time() << endl;
        sem_post(&file);// Signaling the file semaphore

        //Acquiring access to the number of writers variable
        sem_wait(&lock1);
        writers++;//Incrementing the number of writers whose request is yet to be serviced
        if(writers==1){
            sem_wait(&canread);//If it is the first writer acquire canread semaphore to prevent other readers from entering CS
        }
        sem_post(&lock1);

        //Acquire the shared resource
        sem_wait(&writelock);
        
        /*critical section*/

        // To gain access to write into output file
        sem_wait(&file);
        output << i+1 << "th CS Entry by Writer Thread " << id << " at " << Time() << endl;
        // Time of entry
        auto entryTime= chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch()).count();
        sem_post(&file);// Signal the semaphore so that any other thread can perform writing
       
        // Finding sum of time taken for the thread for entering the critical from request time
        average=average+chrono::microseconds(entryTime-reqTime);
        
        // Using exponential distribution for CS time
        exponential_distribution<double> distributionCS(1.0 / muCS);
        double randCST = distributionCS(generator);  


        sleep(randCST/1000);// Performing CS
        sem_post(&writelock);// signaling writelock

        //To gain access to number of writers variable
        sem_wait(&lock1);
        writers--;//decrementing number of writers
        if(writers==0){//If it is the last writer then signal canread so that waiting readers can enter CS
            sem_post(&canread);
        }
        sem_post(&lock1);

        // For file access
        sem_wait(&file);
        output << i+1 << "th CS Exit by Writer Thread " << id << " at " << Time() << endl;
        sem_post(&file);

        // exponential distribution for remainder section time
        exponential_distribution<double> distributionRem(1.0 / muRem);
        double randRemTime = distributionRem(generator);
        sleep(randRemTime/1000);// performing remainder section
        
    }

    // calculating the average time of the kw threads
    average=average/kw;

    // Waiting to gain access to the Writers vector and the output file
    sem_wait(&time_lock);
    Writers.push_back(average); // Adding average time of this thread into the Writers vector
    output2<< "Average time for Writer thread " <<id<<" is "<<average.count()<<endl;
    sem_post(&time_lock);
    return NULL;
}


int main()
{   
    // Reading the input file for the parameters
    ifstream input("inp-params.txt");
    input >> nw;
    input >> nr;
    input >> kw;
    input >> kr >> muCS >> muRem;

    // For Reader and writer threads
    pthread_t WriterThreads[nw];
    pthread_t ReaderThreads[nr];

    // Initialising the semaphores
    sem_t * lock1;
    //sem_init(&lock1,0,1);
   /* sem_init(&lock2,0,1);
    sem_init(&writelock,0,1);
    sem_init(&canread,0,1);
    sem_init(&file,0,1);
    sem_init(&time_lock,0,1);*/
    sem_t *lock2;
    sem_t *writelock;
    sem_t *canread;
    sem_t *file;
    sem_t *time_lock;
    if ((lock1 = sem_open("/lock1", O_CREAT, 0644, 1)) == SEM_FAILED) {
    perror("sem_open");
    exit(EXIT_FAILURE);
  }
  if ((lock2 = sem_open("/lock2", O_CREAT, 0644, 1)) == SEM_FAILED) {
    perror("sem_open");
    exit(EXIT_FAILURE);
  }
  if ((writelock = sem_open("/writelock", O_CREAT, 0644, 1)) == SEM_FAILED) {
    perror("sem_open");
    exit(EXIT_FAILURE);
  }
  if ((canread = sem_open("/canread", O_CREAT, 0644, 1)) == SEM_FAILED) {
    perror("sem_open");
    exit(EXIT_FAILURE);
  }
  if ((file = sem_open("/file", O_CREAT, 0644, 1)) == SEM_FAILED) {
    perror("sem_open");
    exit(EXIT_FAILURE);
  }
  if ((time_lock = sem_open("/time_lock", O_CREAT, 0644, 1)) == SEM_FAILED) {
    perror("sem_open");
    exit(EXIT_FAILURE);
  }


    // Creating writer threads
    for (int i = 0; i < nw; i++)
    {
        pthread_create(&WriterThreads[i], NULL, Writer, NULL);
    }

    // Creating Reader threads
    for (int i = 0; i < nr; i++)
    {
        pthread_create(&ReaderThreads[i], NULL, Reader, NULL);
    }

    // Joining the writer threads
    for (int i = 0; i < nr; i++)
    {
        pthread_join(ReaderThreads[i], NULL);
    }

    // Joining the reader threads
    for (int i = 0; i < nw; i++)
    {
        pthread_join(WriterThreads[i], NULL);
    }

    // Finding the average time for all the writer threads
    chrono::microseconds average_writers(0);
    chrono::microseconds worst_writers(0);
    for (int i = 0; i < nw; i++)
    {
        if(i==0){
            worst_writers=Writers[i];
        }
        else{
            if(Writers[i]>worst_writers){
                worst_writers=Writers[i];
            }
        }
        average_writers=average_writers+Writers[i];
    }
    average_writers=average_writers/nw;
    // writing in the output file
    output2<<"Average of all the Writer threads "<<average_writers.count()<<endl;
    cout<<"Worst time taken by writers "<<worst_writers.count()<<endl;

    // Finding the average time for all the reader threads
    chrono::microseconds average_readers(0);
    chrono::microseconds worst_readers(0);
    for (int i = 0; i < nr; i++)
    {
        if(i==0){
            worst_readers=Readers[0];
        }
        else{
            if(worst_readers<Readers[i]){
                worst_readers=Readers[i];
            }
            
        } 
        average_readers=average_readers+Readers[i];
    }
    cout<<"Worst time taken by readers "<<worst_readers.count()<<endl;
    average_readers=average_readers/nr;
    // writing in the output file
    output2<<"Average of all the Reader threads "<<average_readers.count()<<endl;
    
}