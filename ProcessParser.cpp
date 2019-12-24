//includes definitions of all functions declared in ProcessParser.h header file....

#include "ProcessParser.h"
#include <algorithm>
#include <iostream>
#include <math.h>
#include <thread>
#include <chrono>
#include <iterator>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include "util.h"

/*figure out how much RAM is a given process using */
std::string ProcessParser::getVmSize(std::string pid) {

    std::string line;

    // declare search attributes for file
    std::string name = "VmData";
    std::string value;
    float result;

    // open stream for a specific file
    std::ifstream stream = Util::getStream( (Path::basePath() + pid + Path::statusPath()) );

    while (std::getline(stream, line)) { // read the data by lines
        // searching line by line
        if (line.compare(0, name.size(), name) == 0) {
            // slicing string line on ws for values using sstream
            
            std::istringstream buf(line);
            std::istream_iterator<std::string> begin(buf), end;
            std::vector<std::string> values(begin, end);

            // conversion kB -> GB
            result = ( stof(values[1]) / float(1024*1024) );
            break;
        }
    }

    return std::to_string(result);
}

/* function goal: get CPU percentage by acquiring relevant times of active occupation of CPU*/
std::string ProcessParser::getCpuPercent(std::string pid) {

    std::string line;

    // open stream for a specific file
    std::ifstream stream = Util::getStream( (Path::basePath() + pid + "/" + Path::statPath()) );

    std::getline(stream, line);
    // std::string str = line; // Why do we need this line? can we omit it?
    std::istringstream buf(line);
    std::istream_iterator<std::string> begin(buf), end;
    std::vector<std::string> values(begin, end);

    // acquiring relevant times for calculation of active occupation of CPU for selected process
    float utime = stof(ProcessParser::getProcUpTime(pid));
    float stime = stof(values[14]);  // 14; based on linux format
    float cutime = stof(values[15]); // 15; based on linux format
    float cstime = stof(values[16]); // 16; based on linux format
    float starttime = stof(values[21]); // 21; based on linux format
    float uptime = ProcessParser::getSysUpTime();
    float freq = sysconf(_SC_CLK_TCK); // the number of clock ticks per second,
    // specifically, the kernel is configured for how many clocks per second (or Hz of clock).

    float total_time = utime + stime + cutime + cstime;
    float seconds = uptime - (starttime / freq);
    float result = 100.0 * ( (total_time / freq) / seconds );

    return std::to_string(result);
}


std::string ProcessParser::getProcUpTime(std::string pid) {
    std::string line;
    std::string value;

    // open stream for a specific file
    std::ifstream stream = Util::getStream( (Path::basePath() + pid + "/" + Path::statPath()) );

    std::getline(stream, line);
    // std::string str = line; // Why do we need this line? can we omit it?
    std::istringstream buf(line);
    std::istream_iterator<std::string> begin(buf), end;
    std::vector<std::string> values(begin, end);

    // get clock ticks of the host machine using sysconf()
    return std::to_string( float( stof(values[13]) / sysconf(_SC_CLK_TCK) ) );
}

long int ProcessParser::getSysUpTime() {
    std::string line;
    std::string value;

    // open stream for a specific line
    std::ifstream stream = Util::getStream( ( Path::basePath() + Path::upTimePath() ) );

    std::getline(stream, line);
    std::istringstream buf(line);
    std::istream_iterator<std::string> begin(buf), end;
    std::vector<std::string> values(begin, end);

    return stoi(values[0]); // it should be the default index, which is 0
}

/* Retrieve the process user thru UID*/
std::string ProcessParser::getProcUser(std::string pid) {
    std::string line;

    std::string name = "Uid"; // TODO: confirm if I have to use "Uid:"
    std::string result = "";
    // open stream for a specific line
    std::ifstream stream = Util::getStream( ( Path::basePath() + pid + Path::statusPath() ) );

    // Getting UID for user
    while ( std::getline(stream, line) ) {
        if ( line.compare(0, name.size(), name) == 0 ) {
            std::istringstream buf(line);
            std::istream_iterator<std::string> begin(buf), end;
            std::vector<std::string> values(begin, end);
            result = values[1];
            break;
        }
    }
    stream = Util::getStream("/etc/passwd");
    name = "x:" + result;

    // Search for name of the user with selected UID
    while ( std::getline(stream, line) ) {
        if ( line.find(name) != std::string::npos ) {
            result = line.substr( 0, line.find(":") );
            return result;
        }
    }

    return "";
}

/* Retrieve a pid list from /proc */
std::vector<std::string> ProcessParser::getPidList() {

    DIR* dir;

    std::vector<std::string> container;

    // check if opening the directory is succeeded
    if ( !( dir = opendir("/proc") ) ) {
        throw std::runtime_error( std::strerror(errno) );
    }

    
    while ( dirent* dirp = readdir(dir) ) {
        // STEP 1 : check if it is this a directory
        if ( dirp->d_type != DT_DIR ) continue;

        if ( all_of( dirp->d_name, dirp->d_name + std::strlen(dirp->d_name), [](char c){ return std::isdigit(c); } ) ) {
            container.push_back(dirp->d_name);
        }
    }

    // check if closing the directory is succeeded
    if ( closedir(dir) ) {
        throw std::runtime_error( std::strerror(errno) );
    }

    return container;

}

/* Retrieve the command that executed the process */
std::string ProcessParser::getCmd(std::string pid) {
    string line;
    std::ifstream stream = Util::getStream( (Path::basePath() + pid + Path::cmdPath() ) );
    std::getline(stream, line);

    return line;
}

/* Retrieve the number of CPU cores on the host */
int ProcessParser::getNumberofCores() {
    std::string line;
    std::string name = "cpu cores";
    std::ifstream stream = Util::getStream((Path::basePath() + "cpuinfo"));

    while ( std::getline(stream, line) ) {
        if ( line.compare(0, name.size(), name) == 0 ) {
            std::istringstream buf(line);
            std::istream_iterator<std::string> begin(buf), end;
            std::vector<std::string> values(begin, end);
          
            return stoi(values[3]); // FYI, the raw info: "cpu cores       : 2"
        }
    }
}


std::vector<std::string> ProcessParser::getSysCpuPercent(std::string core_number) {
    std::string line;
    std::string name = "cpu" + core_number;
    std::ifstream stream = Util::getStream( Path::basePath() + Path::statPath() );

    while ( std::getline(stream, line) ) {
        if ( line.compare(0, name.size(), name) == 0 ) {
            std::istringstream buf(line);
            std::istream_iterator<std::string> begin(buf), end;
            std::vector<std::string> values(begin, end);

            return values;
        }
    }

    // otherwise, just return an empty vector
    // Note: look at the way it is constructed
    return std::vector<std::string>();

}

std::string ProcessParser::printCpuStats(std::vector<std::string> values_1,
                                         std::vector<std::string> values_2)
{
    /* Note:
     * Because CPU stats can be calculated only if you take measurements
     * in two different time, this function has two parameters:
     * two vectors of relevant values.
     * We use a formula to calculate overall activity of processor.
     */
    float active_time =   ProcessParser::getSysActiveCpuTime(values_2)
                          - ProcessParser::getSysActiveCpuTime(values_1);
    float idle_time   =   ProcessParser::getSysIdleCpuTime(values_2)
                          - ProcessParser::getSysIdleCpuTime(values_1);

    float total_time  = active_time + idle_time;
    float result      = 100.0 * (active_time / total_time);
    return std::to_string(result);
}

bool ProcessParser::isPidExisting(std::string pid) {
    std::vector<std::string> pid_list = ProcessParser::getPidList();
    if ( std::find(pid_list.begin(), pid_list.end(), pid) != pid_list.end() ) return true;
    else return false;
}

float ProcessParser::getSysActiveCpuTime(std::vector<std::string> values) {
    return ( stof(values[S_USER])       +
             stof(values[S_NICE])       +
             stof(values[S_SYSTEM])     +
             stof(values[S_IRQ])        +
             stof(values[S_SOFTIRQ])    +
             stof(values[S_STEAL])      +
             stof(values[S_GUEST])      +
             stof(values[S_GUEST_NICE]) );
}

float ProcessParser::getSysIdleCpuTime(std::vector<std::string> values) {
    return ( stof(values[S_IDLE]) + stof(values[S_IOWAIT]) );
}


float ProcessParser::getSysRamPercent() {
    std::string line;
  
    std::string name_1 = "MemAvailable:";
    std::string name_2 = "MemFree:";
    std::string name_3 = "Buffers:";

    std::string value;
    int result;
    std::ifstream stream = Util::getStream( ( Path::basePath() + Path::meminfoPath()) );
    float total_mem = 0;
    float free_mem = 0;
    float buffers = 0;

    while ( std::getline(stream, line) ) {
        if (total_mem != 0 && free_mem != 0) break;

        if (line.compare(0, name_1.size(), name_1) == 0) {
            total_mem = ProcessParser::generateMemory(line);
        }

        if (line.compare(0, name_2.size(), name_2) == 0) {
            free_mem = ProcessParser::generateMemory(line);
        }

        if (line.compare(0, name_3.size(), name_3) == 0) {
            buffers = ProcessParser::generateMemory(line);
        }
    }

    // calculate usage
    return float( 100 * ( 1- free_mem / (total_mem - buffers) ) );
}

float ProcessParser::generateMemory(std::string line) {
    std::istringstream buf(line);
    std::istream_iterator<std::string> begin(buf), end;
    std::vector<std::string> values(begin, end);

    return stof(values[1]); // For example:
    
}

/* Open a stream on /proc/version, getting data about the kernel version */
std::string ProcessParser::getSysKernelVersion() {
    std::string line;
    std::string name = "Linux version"; // TODO: just erase a ' '(space) in the end
    std::ifstream stream = Util::getStream( ( Path::basePath() + Path::versionPath() ) );

    while ( std::getline(stream, line) ) {
        if (line.compare(0, name.size(), name) == 0) {
            std::istringstream buf(line);
            std::istream_iterator<std::string> begin(buf), end;
            std::vector<std::string> values(begin, end);
            return values[2]; 
        }
    }

    return "";
}


std::string ProcessParser::getOsName() {
    std::string line;
    std::string name = "PRETTY_NAME";
    std::ifstream stream = Util::getStream(("/etc/os-release"));

    while ( std::getline(stream, line) ) {
        if (line.compare(0, name.size(), name) == 0) {
            std::size_t found = line.find("=");
            found++; // pinpoint the position of "=", and access the next position of it
            std::string result = line.substr(found);
            result.erase( std::remove( result.begin(), result.end(), '"' ), result.end() );
            return result; // For example:
            // PRETTY_NAME="Ubuntu 16.04.5 LTS"
        }
    }

    return "";
}

/* The total thread count is calculated, rather than read from a specific file.
 * We open every PID folder and read its thread coun;
 * after that, we sum the thread counts to calculate the
 * total number of threads on the host machine. */
int ProcessParser::getTotalThreads() {
    std::string line;
    int result = 0;
    std::string name = "Threads";
    std::vector<std::string> list = ProcessParser::getPidList();

    for (int i = 0; i < list.size(); i++) {
        std::string pid = list[i];
        // get every process and read the number of their threads
        std::ifstream stream = Util::getStream( ( Path::basePath() + pid + Path::statusPath() ) );
        while ( std::getline(stream, line) ) {
            if (line.compare(0, name.size(), name) == 0) {
                std::istringstream buf(line);
                std::istream_iterator<std::string> begin(buf), end;
                std::vector<std::string> values(begin, end);

                result += stoi(values[1]); // For example:
               
                break;
            }
        }
    }

    return result;
}

/* Retrieve number of processes info by reading /proc/stat */
int ProcessParser::getTotalNumberOfProcesses() {
    std::string line;
    int result = 0;
    std::string name = "processes";

    std::ifstream stream = Util::getStream( ( Path::basePath() + Path::statPath() ) );
    while ( std::getline(stream, line) ) {
        if (line.compare(0, name.size(), name) == 0) {
            std::istringstream buf(line);
            std::istream_iterator<std::string> begin(buf), end;
            std::vector<std::string> values(begin, end);

            result += stoi(values[1]); // For example:
            
            break;
        }
    }

    return result;
}

/* Retrieve number of running processess info by reading /proc/stat */
int ProcessParser::getNumberOfRunningProcesses() {
    std::string line;
    int result = 0;
    std::string name = "procs_running";

    std::ifstream stream = Util::getStream( ( Path::basePath() + Path::statPath() ) );
    while ( std::getline(stream, line) ) {
        if (line.compare(0, name.size(), name) == 0) {
            std::istringstream buf(line);
            std::istream_iterator<std::string> begin(buf), end;
            std::vector<std::string> values(begin, end);

            result += stoi(values[1]); // For example:
            
            break;
        }
    }

    return result;
}
