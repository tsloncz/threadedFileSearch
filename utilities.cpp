#include <string.h>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <vector>
#include <deque>

#define DEFAULT_THREAD_COUNT 5
#define UNBOUNDED_QUEUE_SIZE INT_MAX

using namespace std;

/*********************************
 * Name:    help_message
 * Purpose: display to the users how to run this program
 *          also show users the available paramters
 * Receive: the arguments
 * Return:  none
 *********************************/
void help_message(char *argv[])
{
    cout << "Usage " << argv[0] << " [parameters]" << endl;
    cout << "At least one following parameters are required:" << endl;
    cout << "      start_directory" << endl;
    cout << "The following parameters are optional:" << endl;
    cout << "      -b buffer_size   Specify the size of the task queue (task_queue)"
         << endl
         << "                       Default is unbounded queue." << endl;
    cout << "      -t thread_count  Specify the total number of threads."
         << endl
         << "                       Default value is " 
         << DEFAULT_THREAD_COUNT << "." << endl;
    cout << "      -name            The pattern to be searched." << endl;
    cout << "                       Default value is *" << endl;
    cout << "      -max_size        The max size (in bytes) of the file." << endl;
    cout << "      -min_size        The min size (in bytes) of the file." << endl;
    cout << "      -h               Display help message." << endl;
}

/*********************************
 * Name:    prase_argv
 * Purpose: parse the parameters
 * Receive: the arguments argv and argc
 * Return:  sort_type: the kind of sorting
 *          thread_count: the number of threads
 *          input_fn: input filename
 *          seq_len: the length of the sequence
 *********************************/
bool parse_argv(int argc, char* argv[], int& thread_count, int& queue_size, 
                int& max_size, int& min_size,
                string& start_dir, string& pattern)
{
    char* endptr; // for strtol
    char* tmp_start_dir = NULL;
    char* tmp_pattern = NULL;

    if(argc < 2)
    {
        return false;
    }
    else if((!strncmp(argv[1], "-h", 2)) ||
            (!strncmp(argv[1], "-H", 2)))
    {
        return false;
    }
    else
    {
        tmp_start_dir = argv[1];
    }

    for(int i = 2; i < argc; i++)
    {
        if((!strncmp(argv[i], "-t", 2)) ||
           (!strncmp(argv[i], "-T", 2)))
        {
            if((i + 1) >= argc){
                cerr << "Invalid thread count value." 
                     << "There must be an interval vlaue after -t"
                     << endl;
                return false;
            }
            thread_count = strtol(argv[++i], &endptr, 0);
            if(*endptr || thread_count <= 0) // Invalid interval value
            {
                cerr << "Invalid thread count value. ";
                cerr << "Thread count must be a positive integer." << endl;
                return false;
            }
        }
        else if((!strncmp(argv[i], "-b", 2)) ||
           (!strncmp(argv[i], "-B", 2)))
        {
            if((i + 1) >= argc){
                cerr << "Invalid queue size value." 
                     << "There must be a queue size vlaue after -B"
                     << endl;
                return false;
            }
            queue_size = strtol(argv[++i], &endptr, 0);
            if(*endptr || thread_count <= 0) // Invalid interval value
            {
                cerr << "Invalid queue size value." 
                     << "Queue size must be a positive integer." << endl;
                return false;
            }
        }
        else if(!strncmp(argv[i], "-name", 5))
        {
            if((i + 1) >= argc){
                cerr << "Invalid pattern." 
                     << "There must be a pattern after -name"
                     << endl;
                return false;
            }
            pattern = argv[++i];
        }
        else if(!strncmp(argv[i], "-max_size", 9))
        {
            if((i + 1) >= argc){
                cerr << "Invalid max size." 
                     << "There must be a size after -max_size"
                     << endl;
                return false;
            }
            max_size = strtol(argv[++i], &endptr, 0);
            if(*endptr || max_size <= 0) // Invalid interval value
            {
                cerr << "Invalid max size value." 
                     << "Max size must be a positive integer." << endl;
                return false;
            }
        }
        else if(!strncmp(argv[i], "-min_size", 9))
        {
            if((i + 1) >= argc){
                cerr << "Invalid min size." 
                     << "There must be a size after -min_size"
                     << endl;
                return false;
            }
            min_size = strtol(argv[++i], &endptr, 0);
            if(*endptr || min_size <= 0) // Invalid interval value
            {
                cerr << "Invalid max size value." 
                     << "Max size must be a positive integer." << endl;
                return false;
            }
        }
        else if((!strncmp(argv[i], "-h", 2)) ||
                (!strncmp(argv[i], "-H", 2)))
        {
            return false;
        }
        else // all other paramters are not allowed 
        {
            cerr << "Invalid parameter: " << argv[i] << endl; 
            return false;
        }
    }
    if(tmp_start_dir == NULL)
    {
        cerr << "Invalid start directory." 
             << "A start directory must be specified"
             << endl;
        return false;
    }
    start_dir = string(tmp_start_dir);
    if(start_dir[start_dir.length() - 1] != '/')
    {
        start_dir += "/";
    }
    if(tmp_pattern != NULL)
    {
        pattern = string(tmp_pattern);
    }
    return true;
}

void output_configuration(const int& thread_count, const int& queue_size,
                          const string& start_dir, const string& pattern,
                          const int& max_size, const int& min_size)
{
    if(queue_size == UNBOUNDED_QUEUE_SIZE)
        cout << "Task queue size:   unbounded" << endl;
    else
        cout << "Task queue size:   " << queue_size << endl;
    cout << "Number of threads: " << thread_count << endl;
    if(pattern == "")
        cout << "Searching for:     anything" << endl;
    else
        cout << "Searching for file names containing: " << pattern << endl;
    cout << "Max size:          " << max_size << endl;
    cout << "Min size:          " << min_size << endl;
    cout << endl;
}

/*********************************
 * Purpose: compute the difference of two time values 
 *********************************/
void timeval_diff (struct timeval &result, struct timeval &from, struct timeval &to)
{
    /* Perform the carry for the later subtraction by updating y. */
    result.tv_usec = (to.tv_usec - from.tv_usec);
    result.tv_sec  = (to.tv_sec - from.tv_sec);
    if(result.tv_usec < 0)
    {
        result.tv_usec += 1000000;
        result.tv_sec -= 1;
    }
}

/*********************************
 * Purpose: display the time ot the console
 *********************************/
void print_time(timeval time, const char* msg)
{
    cout << msg << time.tv_sec << "." << setfill('0')
         << setw(6) << time.tv_usec << "s." << endl;
}

