#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <climits>
#include <time.h>
#include <vector>
#include <sys/time.h>
#include <sys/stat.h>
#include <algorithm>

#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <semaphore.h>

#include "utilities.cpp"

#define IS_DIRECTORY 0x4
#define IS_SYMLINK 0xA

using namespace std;

// The queue of directories to be searched
deque<string> dir_queue;

// DEFAULT_THREAD_COUNT is 5, set in utilities.cpp
int thread_count = DEFAULT_THREAD_COUNT;
// An array for determining if all threads are idle, which means 
// the task is done.
bool* thread_idle;

int max_size = INT_MAX;
int min_size = 0;

// DEFAULT_QUEUE_SIZE = -1, which means unbounded queue
int queue_size = UNBOUNDED_QUEUE_SIZE;
string pattern = ""; // The pattern of the file to be searched

// mutex for outputting
pthread_mutex_t outputting_mutex = PTHREAD_MUTEX_INITIALIZER;
// mutex for thread_idle
pthread_mutex_t thread_idle_mutex = PTHREAD_MUTEX_INITIALIZER;
// mutex for reading a directory from the directory queue
pthread_mutex_t read_from_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

// Declare semaphores
//Semaphore for getting directory from que
sem_t semReadQue;
//Semaphore for writing path
sem_t semWritePath;
//Semaphore for writing path
sem_t semWritePathDir;
//Semaphore for addding dir to que
sem_t semQuePush;

void* parallel_search(void*);   // Parallel directory search
void recursive_search(string);  // Recursive directory search

int main(int argc, char* argv[])
{
    /****************Just some setup****************/
    int rc; // return code for pthread

    string start_dir; // Name of the starting directory

    // For measuring time
    struct timeval start_time, end_time, diff;

    // Parsing arguments
    if(parse_argv(argc, argv, thread_count, queue_size,
                  max_size, min_size, start_dir, pattern) == false)
    {
        help_message(argv);
        exit(EXIT_FAILURE);
    }

    // output the configurations
    output_configuration(thread_count, queue_size, start_dir, pattern, 
                         max_size, min_size); 
    /****************END OF setup****************/

    dir_queue.push_back(start_dir);
    if(start_dir.find(pattern) != string::npos 
       && max_size == INT_MAX && min_size == 0) 
    // if start_dir satisfies the search criteria, output it
    {
        cout << start_dir << endl;
    }


    //Initialize semaphores
    sem_init(&semReadQue, 0, 1);
    sem_init(&semWritePath, 0, 1);
    sem_init(&semWritePathDir, 0, 1);
    sem_init(&semQuePush, 0, 1);
    // TODO: Declare threads...done
	// declare thread
    pthread_t *thread_id = new pthread_t[thread_count];
    
    // flags for checking if thread is idle
    thread_idle = new bool[thread_count];


    gettimeofday(&start_time, 0);
    /****************Parallel Directory Search****************/
    // Create threads
    int name_list[thread_count]; // name of threads
                                 // to avoid confusion with thread_id
    // TODO: Create threads...done
	// create thread, join thread
    int id_list[thread_count];
    for(int i = 0; i < thread_count; i++)
    {
        id_list[i] = i;
        thread_idle[i] = 0;
        // create thread and pass thread_id[i] 
		// (by reference) as the thread id
        pthread_create(&thread_id[i], NULL, parallel_search, &id_list[i]);
    }
    // TODO: Join threads...done
    for(int i = 0; i < thread_count; i++)
    {
        pthread_join(thread_id[i], NULL);
    }
    // NOTE: CREATE ALL THREADS and then JOIN ALL THREADS.
    //       Not create one thread and join one thread!
    /****************END OF Parallel Directory Search****************/
    gettimeofday(&end_time, 0);
    timeval_diff(diff, start_time, end_time);

    if(queue_size == UNBOUNDED_QUEUE_SIZE)
    {
        cout << "Searching " << pattern
             << " with " << thread_count 
             << " threads and unbounded queue";
    }
    else
    {
        cout << "Searching " << pattern 
             << " with " << thread_count 
             << " threads and queue size " << queue_size;
    }
    print_time(diff, " took ");


    delete []thread_idle;
    return 0;
}

/*********************************
 * Purpose: get a directory from dir queue and search it
 * Receive: void* args starts a thread name as int
 * Return:  None
 *********************************/
void* parallel_search(void* arg)
{
    int* t_name = (int *) arg;
    int allIdle = 0;
    pthread_mutex_lock(&outputting_mutex);
      cout << "Thread " << *t_name << " started." << endl;
    pthread_mutex_unlock(&outputting_mutex);
    while(true)
	{
      for(int i = 0; i<thread_count; i++)
      {
         if( thread_idle[i] == 0 )
         {
           //There is a thread working
           break;
         }
         allIdle = 1;
         //cout << "All threads idle" << endl;
      }                                
        if( allIdle == 1 && dir_queue.empty() )
        {
          break;
        }
		DIR *dir; //directory object returned from opendir()
		string d_name = "";
        string matchingFile;
		
        // Use semaphore to make sure only one thread
        // can access at a time
		sem_wait(&semReadQue);
          thread_idle[*t_name] = 0;
          if( !dir_queue.empty() )
		  {
			d_name = dir_queue.front();
			dir_queue.pop_front();
		  } 
        //Post someone else can enter
        sem_post(&semReadQue);

		if( d_name != "" )
		{
			dirent* entry;
			dir = opendir(d_name.c_str());
            if(errno != 0)
            {
              cout << "opendir error. " << strerror(errno) << endl;
            }
            else if(dir)
            {
              errno = 0;
              // Read through contents of directory
			  //readdir() return pointer to object of type dirent
			  // *****Assigment to entry must be done in loop because
			  // Recalling readddir effects it's contents and
			  // it will read the directory incorrectly******	
			  while( (entry = readdir(dir)) != NULL )
			  {
                  bool criteriaSatisfied = 1;
				  struct stat sb;
				  if(entry->d_type == IS_DIRECTORY )
				  {
					  string directory = entry->d_name;
                      int foundPatternDir = directory.find(pattern);
                      // exclude parent directories
					  if( directory != "." && directory != ".." &&
                          directory != ".snapshot")
                      {
                        if( pattern != "" && foundPatternDir >= 0) // need to match
                        {
                            cout << "Directory: " << d_name+directory+"/" << endl;
                        }
                        //Add directory to que to be searched
                        string dirPath = d_name+directory+"/";
                        sem_wait(&semQuePush);
                          dir_queue.push_back(dirPath);
                          //cout << "Added to que: " << dirPath << endl;
                        sem_post(&semQuePush);
                      }
				  }
                  // Ignore symbolic linkds
			  	  else if( entry->d_type == IS_SYMLINK )
				  {
					  break;
				  }
				  else //found file
				  {
                    string name = entry->d_name;
                    string full_path = d_name+entry->d_name;
                    int foundPattern = name.find(pattern);
                    if( pattern != "") // need to match
                    {
                      if( foundPattern >= 0)
                      {
                        criteriaSatisfied = 1;
                        matchingFile = full_path;
                      }
                      else
                      { 
                        criteriaSatisfied = 0;
                      }
                    }
                    else
                    {
                      matchingFile = full_path;
                    }
				  	struct stat filestatus; 		// the buffer
                    int ret = stat(full_path.c_str(), &filestatus);
                    //Don't bother checking size if file already failed
					  if(ret == 0 && criteriaSatisfied == 1) 
					  {
                        if(filestatus.st_size<min_size)
                        {
                          criteriaSatisfied = 0;
                          matchingFile = full_path;
                        }
                        if(filestatus.st_size>max_size)
                        {
                          criteriaSatisfied = 0;
                          matchingFile = full_path;
                        }
                      }
					//cout << "  length: " << entry->d_reclen << endl;
                    if(criteriaSatisfied == 1)
                    {
                      sem_wait(&semWritePath);
                        cout << "File: " << matchingFile << endl;
                      sem_post(&semWritePath);
                    }
                    criteriaSatisfied = 1;
				  }
			  }
              if(errno != 0)
              {
                cout << "readdir error. " << strerror(errno) << endl;
              }
            }
            closedir(dir); // Close directory
            thread_idle[*t_name] = 1;
        }
		//break;
        // Thread terminates (break the loop) if both of the following 
        // conditions are true
        // 1. All threads have nothing to work on. (all thread_idle 
        //    are false)
        // 2. There are no more directory in the dir_queue to be processed.

        // If the loop continues
        // Each thread grabs a directory from the queue and process it
        // 1. open the directory...done
        // 2. keep reading the directory
        //    - for each entry, check if it is a directory or a file...done
        //    - if it is a directory, check if it satisfies the criteria.
        //      If it satisfies, output the full path....done
        //    - No matter the directory satisfies the criteria or not, add 
        //      it to the queue for processing its subdirectory....done
        //    - if it is a file, check if it satisfies the criteria.
        //      If it satisfies, output the full path....done
        // 3. close the direcotry...done
		
    }
		
    pthread_mutex_lock(&outputting_mutex);
    cout << "Thread " << *t_name << " terminated." << endl;
    pthread_mutex_unlock(&outputting_mutex);

    // TODO: terminate the thread...done
	pthread_exit(t_name);
}


void recursive_search(string curr_dir)
{
}

