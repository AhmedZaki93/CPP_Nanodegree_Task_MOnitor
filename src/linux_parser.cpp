#include "linux_parser.h"

#include <dirent.h>
#include <unistd.h>

#include <string>
#include <vector>

using std::stof;
using std::string;
using std::to_string;
using std::vector;

// DONE: An example of how to read data from the filesystem
string LinuxParser::OperatingSystem() {
  string line;
  string key;
  string value;
  std::ifstream filestream(kOSPath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ' ', '_');
      std::replace(line.begin(), line.end(), '=', ' ');
      std::replace(line.begin(), line.end(), '"', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          std::replace(value.begin(), value.end(), '_', ' ');
          return value;
        }
      }
    }
  }
  return value;
}

// DONE: An example of how to read data from the filesystem
string LinuxParser::Kernel() {
  string os, version, kernel;
  string line;
  std::ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> os >> version >> kernel;
  }
  return kernel;
}

// BONUS: Update this to use std::filesystem
vector<int> LinuxParser::Pids() {
  vector<int> pids;
  DIR* directory = opendir(kProcDirectory.c_str());
  struct dirent* file;
  while ((file = readdir(directory)) != nullptr) {
    // Is this a directory?
    if (file->d_type == DT_DIR) {
      // Is every character of the name a digit?
      string filename(file->d_name);
      if (std::all_of(filename.begin(), filename.end(), isdigit)) {
        int pid = stoi(filename);
        pids.push_back(pid);
      }
    }
  }
  closedir(directory);
  return pids;
}

// TODO: Read and return the system memory utilization
// memory utilization calculated as
// memory_utilization = (total memory - free memory)/total memory
// according to site : support.site24x7.com
float LinuxParser::MemoryUtilization() {
  float free_memory = 1.0;
  float total_memory = 1.0;
  string line, key, value;
  std::ifstream fileStream(kProcDirectory + kMeminfoFilename);
  if (fileStream.is_open()) {
    while (std::getline(fileStream, line)) {
      std::istringstream lineStream(line);
      // check for specifi keyword according to system time folder in linux
      lineStream >> key;
      if (key == "MemTotal:") {
        lineStream >> total_memory;
      } else if (key == " MemAvailable:") {
        lineStream >> free_memory;
        break;
      }
    }
  }

  return ((total_memory - free_memory) / total_memory);
}

// TODO: Read and return the system uptime
long LinuxParser::UpTime() {
  string line, uptimeStr;
  long uptime;
  std::ifstream stream(kProcDirectory + kUptimeFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream lineStream(line);
    lineStream >> uptimeStr;
  }
  uptime = std::stol(uptimeStr);

  return uptime;
}

// TODO: Read and return the number of jiffies for the system
long LinuxParser::Jiffies() {
  return LinuxParser::ActiveJiffies() + LinuxParser::IdleJiffies();
}

// TODO: Read and return the number of active jiffies for a PID
// REMOVE: [[maybe_unused]] once you define the function
long LinuxParser::ActiveJiffies(int pid) {
  string line, value;
  vector<string> Active_jiffies;
  std::ifstream fileStream(kProcDirectory + std::to_string(pid) +
                           kStatFilename);
  if (fileStream.is_open()) {
    // extract line of cpu information for specified PID
    std::getline(fileStream, line);
    std::istringstream lineStream(line);
    // get each value inside displayed line
    while (lineStream >> value) {
      Active_jiffies.push_back(value);
    }
  }

  // here we have Active jiffies
  // we need to parse utime ,stime, cstime
  /* utime :total time  this process is scheduled in user mode
     stime :amount of time this process scheduled in kernel mode
     cutime:amount of time this process wait for children process in User mode
     cstime : amount of time this process wait for children in Kernel mode
     */
  long utime, stime, cutime, cstime, total_time;
  // use std:: all of  function to search inside defined loop for digit
  if (std::all_of(Active_jiffies[13].begin(), Active_jiffies[13].end(),
                  isdigit)) {
    utime = std::stol(Active_jiffies[13]);
  }
  if (std::all_of(Active_jiffies[14].begin(), Active_jiffies[14].end(),
                  isdigit)) {
    stime = std::stol(Active_jiffies[14]);
  }
  if (std::all_of(Active_jiffies[15].begin(), Active_jiffies[15].end(),
                  isdigit)) {
    cutime = std::stol(Active_jiffies[15]);
  }
  if (std::all_of(Active_jiffies[16].begin(), Active_jiffies[16].end(),
                  isdigit)) {
    cstime = std::stol(Active_jiffies[16]);
  }
  // reference :https://man7.org/linux/man-pages/man5/proc.5.html
  total_time = utime + stime + cutime + cstime;
  return total_time / sysconf(_SC_CLK_TCK);
}

// TODO: Read and return the number of active jiffies for the system
long LinuxParser::ActiveJiffies() {
  // get CPU utilization
  auto jiffies = CpuUtilization();

  return std::stol(jiffies[CPUStates::kUser_]) +
         std::stol(jiffies[CPUStates::kIRQ_]) +
         std::stol(jiffies[CPUStates::kNice_]) +
         std::stol(jiffies[CPUStates::kSoftIRQ_]) +
         std::stol(jiffies[CPUStates::kSteal_]) +
         std::stol(jiffies[CPUStates::kSystem_]);
}

// TODO: Read and return the number of idle jiffies for the system
long LinuxParser::IdleJiffies() {
  auto jiffies = CpuUtilization();

  return std::stol(jiffies[CPUStates::kIdle_]) +
         std::stol(jiffies[CPUStates::kIOwait_]);
}

// TODO: Read and return CPU utilization
vector<string> LinuxParser::CpuUtilization() {
  string line, cpu, value;
  vector<string> cpu_uti;
  std::ifstream fileStream(kProcDirectory + kStatFilename);
  if (fileStream.is_open()) {
    std::getline(fileStream, line);
    std::istringstream LineStream(line);
    // cpu id
    LineStream >> cpu;
    // Parse Cpu value
    while (LineStream >> value) {
      cpu_uti.push_back(value);
    }
  }

  return cpu_uti;
}

// TODO: Read and return the total number of processes
int LinuxParser::TotalProcesses() {
  string line, key;
  int value;
  std::ifstream Filestream(kProcDirectory + kStatFilename);
  if (Filestream.is_open()) {
    while (std::getline(Filestream, line)) {
      std::istringstream Stream(line);
      Stream >> key;
      if (key == "processes") {
        Stream >> value;
      }
    }
  }
  return value;
}

// TODO: Read and return the number of running processes
int LinuxParser::RunningProcesses() {
  string line, key;
  int value;

  std::ifstream Filestream(kProcDirectory + kStatFilename);
  if (Filestream.is_open()) {
    while (std::getline(Filestream, line)) {
      std::istringstream Stream(line);
      Stream >> key;
      if (key == "procs_running") {
        Stream >> value;
        break;
      }
    }
  }
  return value;
}

// TODO: Read and return the command associated with a process
// REMOVE: [[maybe_unused]] once you define the function
string LinuxParser::Command(int pid) {
  string line;

  std::ifstream Filestream(kProcDirectory + to_string(pid) + kCmdlineFilename);
  if (Filestream.is_open()) {
    // as its only single command line without spaces
    std::getline(Filestream, line);
  }
  return line;
}

// TODO: Read and return the memory used by a process
// REMOVE: [[maybe_unused]] once you define the function
string LinuxParser::Ram(int pid) {
  string line, key, ram;
  long value;
  std::ifstream Filestream(kProcDirectory + to_string(pid) + kStatusFilename);
  if (Filestream.is_open()) {
    while (std::getline(Filestream, line)) {
      std::istringstream Stream(line);
      Stream >> key;
      if (key == "VmSize") {
        Stream >> value;
        // to get memory value in Mega byte
        value /= 1024;
        ram = to_string(value);
      }
    }
  }
  return ram;
}

// TODO: Read and return the user ID associated with a process
// REMOVE: [[maybe_unused]] once you define the function
string LinuxParser::Uid(int pid) {
  string line, uid, key;
  std::ifstream FIleStream(kProcDirectory + to_string(pid) + kStatusFilename);
  if (FIleStream.is_open()) {
    while (std::getline(FIleStream, line)) {
      std::istringstream Stream(line);
      Stream >> key;
      if (key == "Uid:") {
        Stream >> uid;
        break;
      }
    }
  }
  return uid;
}

// TODO: Read and return the user associated with a process
// REMOVE: [[maybe_unused]] once you define the function
string LinuxParser::User(int pid) {
  string line, usr, x, value, uid, key;
  std::ifstream FileStream(kPasswordPath);
  // get the Uid from method Uid
  uid = Uid(pid);
  if (FileStream.is_open()) {
    while (std::getline(FileStream, line)) {
      // note : use single Quote instead of Double to avoid Compiler Error
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream Stream(line);
      Stream >> key >> x >> value;
      if (value == uid) {
        usr = key;
        break;
      }
    }
  }
  return usr;
}

// TODO: Read and return the uptime of a process
// REMOVE: [[maybe_unused]] once you define the function
long LinuxParser::UpTime(int pid) {
  string line, key;
  long utime = 0;
  vector<string> values;

  std::ifstream FileStream(kProcDirectory + to_string(pid) + kStatFilename);
  if (FileStream.is_open()) {
    std::getline(FileStream, line);
    std::istringstream Stream(line);
    while (Stream >> key) {
      values.push_back(key);
    }
  }
  // Error hadle for worng time calculation
  try {
    utime = std::stol(values[21]) / sysconf(_SC_CLK_TCK);
    if (utime < 0) {
      throw std::invalid_argument("Wrong" + to_string(pid) + "uptime");
    }
  } catch (...) {
    utime = 0;
  }
  return utime;
}
