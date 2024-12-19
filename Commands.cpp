#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <signal.h>
#include <sys/types.h>
#include <stdio.h>
#include <dirent.h>
#include <algorithm>
#define _GNU_SOURCE
#include <complex>
#include <dirent.h>     /* Defines DT_* constants */
#include <err.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>


struct linux_dirent {
    unsigned long  d_ino;
    off_t          d_off;
    unsigned short d_reclen;
    char           d_name[];
};


using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h

SmallShell::SmallShell():prompt("smash"),jobs(new JobsList),prev_dir("")
,aliases(),curr_running_fg(-1){

    pid=getpid();
    if(pid == -1)
    {
        perror("smash error: getpid failed");
        return;
    }
}

SmallShell::~SmallShell() {
    delete jobs;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/

Command *SmallShell::CreateCommand(const char *cmd_line) {

    string c = (string) cmd_line;
    if(c.empty()){
        return nullptr;
    }
    char r[200] = {0};
    strcpy(r,cmd_line);
    _removeBackgroundSign(r);

    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    char* args[20]={0};

    if (firstWord.compare("chprompt") == 0) {
        SmallShell& smash = SmallShell::getInstance();

        int num=_parseCommandLine(r,args);
        if(num == 1) {
            smash.prompt="smash";
        } else {
            smash.prompt=(string)args[1];
        }
    }
    else if (firstWord.compare("pwd") == 0) {
        return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord.compare("showpid") == 0) {
        return new ShowPidCommand(cmd_line);
    }
    else if(firstWord.compare("cd") == 0){
        return new ChangeDirCommand(cmd_line);
    }
    else if(firstWord.compare("jobs") == 0){
        return new JobsCommand(cmd_line,jobs);
    }
    else if(firstWord.compare("fg") == 0){
        return new ForegroundCommand(cmd_line,jobs);
    }
    else if(firstWord.compare("quit") == 0){
        return new QuitCommand(cmd_line,jobs);
    }
    else if(firstWord.compare("kill") == 0){
        return new KillCommand(cmd_line,jobs);
    }
    else if(firstWord.compare("alias") == 0){
        return new aliasCommand(cmd_line);
    }
    else if(firstWord.compare("unalias") == 0){
        return new unaliasCommand(cmd_line);
    }
    else if(((string)cmd_line).find(">") != string::npos){
        return new RedirectionCommand(cmd_line);
    }
    else if(((string)cmd_line).find("|") != string::npos){
        return new PipeCommand(cmd_line);
    }
    else if(firstWord.compare("listdir") == 0){
        return new ListDirCommand(cmd_line);
    }
    else if(firstWord.compare("getuser") == 0){
        return new GetUserCommand(cmd_line);
    }
        /*else if(firstWord.compare("watch") == 0){
            return new WatchCommand(cmd_line);
        }*/
    else{
        return new ExternalCommand(cmd_line);
    }

    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // for example:
    jobs->removeFinishedJobs();
    Command* cmd = CreateCommand(cmd_line);
    if(cmd == nullptr){
        return;
    }
    cmd->execute();


}



/////////////////////////////////////////////// job

JobsList::JobEntry::JobEntry(int jobId, int pid, const string &cmdLine, bool isStopped) : job_id(
        jobId), pid(pid), cmd_line(cmdLine), is_stopped(isStopped) {}

JobsList::JobsList():jobs(),max_job_id(0){}

void JobsList::addJob(Command *cmd,int pid, bool isStopped) {
    removeFinishedJobs();
    max_job_id++;
    //JobEntry new_job(this->max_job_id,getpid(),cmd->cmd_line, isStopped);
    std::shared_ptr<JobsList::JobEntry> ptr(new JobsList::JobEntry(this->max_job_id,pid,cmd->cmd_line, isStopped));
    jobs.push_back(ptr);
}

void JobsList::printJobsList() {
    removeFinishedJobs();

    for(std::shared_ptr<JobsList::JobEntry> j : jobs){
        cout<<"["<<j->job_id<<"] "<<j->cmd_line<<endl;
    }
}

void JobsList::killAllJobs() {

    removeFinishedJobs();
    int num = jobs.size();
    cout<<"smash: sending SIGKILL signal to "<<num<<" jobs:"<<endl;
    for(std::shared_ptr<JobEntry> j : jobs){
        cout<<j->pid<<": "<<j->cmd_line<<endl;
        kill(j->pid,9);
    }
    updateMaxJobID();
}


void JobsList::updateMaxJobID() {
    max_job_id = 0;

    for(auto element : jobs){
        if (element->job_id > max_job_id){
            max_job_id = element->job_id;
        }
    }
}





void JobsList::removeFinishedJobs() {

    vector<shared_ptr<JobEntry>> newJobList;

    for (int i = 0; i < (int )jobs.size(); ++i) {

        pid_t pid=waitpid(jobs[i]->pid,NULL,WNOHANG);
        if(pid == 0){
            newJobList.push_back(jobs[i]);
        }
    }
    jobs=newJobList;
    updateMaxJobID();
}

JobsList::JobEntry * JobsList::getJobById(int jobId){

    for(auto ptr : jobs){
        if(ptr->job_id == jobId) return ptr.get();
    }
    return nullptr;
}

JobsList::JobEntry * JobsList::getLastJob(int *lastJobId) {
    return nullptr;
}

JobsList::JobEntry * JobsList::getLastStoppedJob(int *jobId) {
    return nullptr;
}





///////////////////////////////////////////////////////

Command::Command(const char *cmd_line):cmd_line(cmd_line),is_bg(_isBackgroundComamnd(cmd_line)) {

    char removed_sign_line[200]={0};
    strcpy(removed_sign_line,cmd_line);
    if(is_bg) {
        _removeBackgroundSign(removed_sign_line);
    }
    num_of_args= _parseCommandLine(removed_sign_line,arguments);

}


BuiltInCommand::BuiltInCommand(const char *cmd_line): Command(cmd_line){}


////////////////////////// ShowPidCommand
ShowPidCommand::ShowPidCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {

    SmallShell& smash=SmallShell::getInstance();
    cout<<"smash pid is "<<smash.pid<<endl;
}

//////////////////////////////////// GetCurrDirCommand
GetCurrDirCommand::GetCurrDirCommand(const char *cmd_line): BuiltInCommand(cmd_line){}

void GetCurrDirCommand::execute() {
    long longest_path_size =pathconf(".", _PC_PATH_MAX);
    char* path = (char*)malloc((size_t )longest_path_size);
    if(getcwd(path, longest_path_size) != nullptr)
    {
        cout<< path << endl;
    } else
    {
        perror("smash error: getcwd failed");
    }
    free(path);
}

/////////////////////////////////////

/////////////////////////////// ChangeDirCommand

ChangeDirCommand::ChangeDirCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}

void ChangeDirCommand::execute() {

    if(num_of_args == 1) return ;
    if(num_of_args > 2){
        cerr<<"smash error: cd: too many arguments"<<endl;
        return;
    }
    string new_dir;
    SmallShell &smash = SmallShell::getInstance();
    if(((string)arguments[1]).compare("-") == 0 )
    {
        if(smash.prev_dir == "")
        {
            cerr<<"smash error: cd: OLDPWD not set"<<endl;
            return;
        }else
        {
            new_dir=smash.prev_dir;
        }

    } else
    {
        new_dir=string(arguments[1]);
    }
    char curr_dir[200];
    if (getcwd(curr_dir,sizeof(curr_dir)) == NULL)
    {
        perror("smash error: getcwd failed");
    }

    if(chdir(new_dir.c_str()) == -1)
    {
        perror("smash error: chdir failed");
    }
    smash.prev_dir=string(curr_dir);
    return ;
}

/////////////////////////////////

//////////////////////////////////// JobsCommand
JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs): BuiltInCommand(cmd_line),jobs(jobs){}

void JobsCommand::execute() {
    jobs->printJobsList();
}

//////////////////////////////////////////

//////////////////////////////////// ForegroundCommand
ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs): BuiltInCommand(cmd_line),jobs(jobs){

}

bool check_job(char* argument){

    int i=0;
    while (argument[i]){
        if(argument[i] < 48 || argument[i] > 57){
            return false;
        }
        i++;
    }

    int num=stoi(argument);
    if(num <= 0) return false;

    return true;
}

void ForegroundCommand::execute() {

    SmallShell& smash=SmallShell::getInstance();

    if (num_of_args == 1 && jobs->jobs.empty()){
        cerr<<"smash error: fg: jobs list is empty"<<endl;
    } else if(num_of_args == 2){

		if(!check_job(arguments[1])){
           cerr<<"smash error: fg: invalid arguments"<<endl;
           return;
       }
        for(vector<std::shared_ptr<JobsList::JobEntry>>::iterator it=jobs->jobs.begin();it!=jobs->jobs.end();++it){
            if((*it)->job_id == stoi(arguments[1])){
                cout<<(*it)->cmd_line<<" "<<(*it)->pid<<endl;
                smash.curr_running_fg = (*it)->pid;
                waitpid((*it)->pid,NULL,WUNTRACED);
                smash.curr_running_fg = -1;
                jobs->jobs.erase(it);
                return;
            }
        }
        cerr<<"smash error: fg: job-id "<<arguments[1]<<" does not exist"<<endl;
    } else if(num_of_args > 2){
        cerr<<"smash error: fg: invalid arguments"<<endl;
    } else if(num_of_args == 1){
        for(vector<std::shared_ptr<JobsList::JobEntry>>::iterator it=jobs->jobs.begin();it!=jobs->jobs.end();++it) {
            if ((*it)->job_id == jobs->max_job_id) {
                cout << (*it)->cmd_line << " " << (*it)->pid << endl;
                smash.curr_running_fg = smash.jobs->getJobById(smash.jobs->max_job_id)->pid;
                waitpid((*it)->pid, NULL, WNOHANG);
                smash.curr_running_fg = -1;
                jobs->jobs.erase(it);
                return;
            }
        }
    }

}

////////////////////////////////////////

/////////////////////////////// QuitCommand
QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs): BuiltInCommand(cmd_line),jobs(jobs){}

void QuitCommand::execute(){

    if(this->num_of_args == 1){ ////////// only quit
        exit(0);
    }
    else if(num_of_args >= 2){
        if(((string)arguments[1]).compare("kill") == 0 ){
            jobs->killAllJobs();
        }
    }

    return;
}

/////////////////////////////////////////////

////////////////////////////////// KillCommand

bool check_arguments(char* str1,char* str2){
    if(str1[0] != '-') return false;
    int i=1;
    while(*(str1+i)){
        if(str1[i] < 48 || str1[i] > 57 ) return false;
        i++;
    }
    if(i > 3 || str1[1] >51) return false;
    if(str1[1] == '3' && str1[2] > 49) return false;
    i=0;
    while(*(str2+i)){
        if(str2[i] < 48 || str2[i] > 57 ) return false;
        i++;
    }


    return true;
}

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs): BuiltInCommand(cmd_line),jobs(jobs){}

void KillCommand::execute() {

    if(num_of_args != 3){
        cerr<<"smash error: kill:invalid arguments"<<endl;
    } else if(!check_arguments(arguments[1],arguments[2])){
        cerr<<"smash error: kill:invalid arguments"<<endl;
    } else if(this->jobs->getJobById(stoi(arguments[2])) == nullptr){
        cerr<<"smash error: kill: job-id "<<arguments[2]<<" does not exist"<<endl;
    } else {
        char* temp=arguments[1]+1;
        cout<<"signal number "<<temp<<" was sent to pid "<<this->jobs->getJobById(stoi(arguments[2]))->pid<<endl;
        kill(this->jobs->getJobById(stoi(arguments[2]))->pid,stoi(temp));
    }

    return ;
}

///////////////////////////////////////////////

////////////////// Alias Command
aliasCommand::aliasCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}

bool check_name(const char* cmd_line,string& name,string& command){

    int i=0;
    while(*(cmd_line+i) && cmd_line[i]!='=' ){
        i++;
    }
    for (int j = 0; j < i; ++j) {
        name+=cmd_line[j];
    }
    i+=2;
    int counter=0;
    while(*(cmd_line+i)){
        command+=cmd_line[i];
        i++;
    }
    i=0;
    for(i=0 ; i<name.length(); ++i){
        if(!(name[i] >= 48 && name[i] <= 57) && !(name[i] >= 65 && name[i] <= 90)
           && !(name[i] >= 97 && name[i] <= 122) && name[i]!= 95){
            return false;
        }
    }


    return true;
}

void aliasCommand::execute() {

    SmallShell& smash=SmallShell::getInstance();
    string name="",command="";
    if(!check_name(this->cmd_line,name,command)){
        cerr<<"smash error: alias: invalid alias format"<<endl;
    } else {
        bool checker= true;
        for (auto ptr : smash.aliases) {
            if(ptr->name.compare(name) == 0) checker=false;
        }

        if(name.compare("chprompt") == 0) checker= false;
        if(name.compare("showpid") == 0) checker= false;
        if(name.compare("pwd") == 0) checker= false;
        if(name.compare("cd") == 0) checker= false;
        if(name.compare("jobs") == 0) checker= false;
        if(name.compare("fg") == 0) checker= false;
        if(name.compare("quit") == 0) checker= false;
        if(name.compare("kill") == 0) checker= false;
        if(name.compare("alias") == 0) checker= false;
        if(name.compare("unalias") == 0) checker= false;
        if(name.compare("listdir") == 0) checker= false;
        if(name.compare("getuser") == 0) checker= false;
        if(name.compare("watch") == 0) checker= false;

        if(!checker){
            cerr<<"smash error: alias "<<name<<" already exists or is a reserved command"<<endl;
        }
        char* cmd= const_cast<char *>(command.c_str());
        std::shared_ptr<SmallShell::Node> ptr( new SmallShell::Node(name,cmd));
        smash.aliases.push_back(ptr);
    }

}

//////////////////////////////////////

///////////////////////////// unalias command
unaliasCommand::unaliasCommand(const char *cmd_line): BuiltInCommand(cmd_line){}

void unaliasCommand::execute() {

    SmallShell& smash=SmallShell::getInstance();

    if(num_of_args == 1){
        cerr<<"smash error: unalias: not enough arguments"<<endl;
    } else {
        bool checker= true;
        for(int i=1 ; i < num_of_args; ++i){
            checker= true;
            for(vector<shared_ptr<SmallShell::Node>>::iterator it=smash.aliases.begin();it!=smash.aliases.end();++it){
                if((*it)->name.compare((string)arguments[i]) == 0){
                    checker= false;
                    smash.aliases.erase(it);
                }
            }
            if (checker){
                cerr<<"smash error: unalias: "<<arguments[i]<<" alias does not exist"<<endl;
            }
        }
    }
}

//////////////////////////////////////////////////////

////////////////////////////// External Command

ExternalCommand::ExternalCommand(const char *cmd_line): Command(cmd_line),is_complex(((const string)cmd_line).find('*') != string::npos
                                                                                     || ((const string)cmd_line).find('?') != string::npos){}




void ExternalCommand::execute() {

    SmallShell& smash = SmallShell::getInstance();
    pid_t pid = fork();

    if (pid == -1){
        perror("smash error: fork failed");
        return;
    }

    if(!is_complex)
    {
        if (!is_bg)
        {
            if(pid == 0){ // child
                if (setpgrp() == -1) {
                    perror("smash error: setpgrp failed");
                    exit(1);
                }
                execvp(arguments[0], arguments);
                perror("smash error: execvp failed");
                exit(1);
            } else { // parent
                smash.curr_running_fg = pid;
                if(waitpid(pid,NULL, WUNTRACED) == -1){
                    perror("smash error: waitpid failed");
                    return;
                }
                smash.curr_running_fg = -1;
            }
        }
        else // backgroung command
        {
            if(pid == 0){ //child
                if (setpgrp() == -1) {
                    perror("smash error: setpgrp failed");
                    exit(1);
                }
                execvp(arguments[0], arguments);
                perror("smash error: execvp failed");
                exit(0);
            }else { // parent
                smash.jobs->addJob(this,pid);
                return;
            }
        }
    }
    else
    {
        char removed_sign_cmd_line[80] = {0};
        strcpy(removed_sign_cmd_line,cmd_line);
        _removeBackgroundSign(removed_sign_cmd_line);
        const char *args[] = {"/bin/bash", "-c", removed_sign_cmd_line, nullptr};

        if (!is_bg)
        {
            if(pid == 0){ // child
                if (setpgrp() == -1) {
                    perror("smash error: setpgrp failed");
                    exit(1);
                }
                if (execv(args[0], (char **) args) == -1) {
                    perror("smash error: execv failed");
                    exit(0);
                }
            } else { // parent
                smash.curr_running_fg = pid;
                if(waitpid(pid,NULL, WUNTRACED) == -1){
                    perror("smash error: waitpid failed");
                    return;
                }
                smash.curr_running_fg = -1;
            }
        }
        else // backgroung command
        {
            if (pid == 0) { // child
                if (setpgrp() == -1) {
                    perror("smash error: setpgrp failed");
                    exit(1);
                }
                if (execv(args[0], (char **) args) == -1) {
                    perror("smash error: execv failed");
                    exit(0);
                }
            } else { // parent
                smash.jobs->addJob(this,pid);
                return;
            }
        }
    }
}

/////////////////////////////////////////

////////////////////////////// Redirection Command

RedirectionCommand::RedirectionCommand(const char *cmd_line) : Command(cmd_line)
        ,type(((string)cmd_line).find("<<") != string::npos){ }


void RedirectionCommand::execute() {

    SmallShell& smash = SmallShell::getInstance();
    Command* command = smash.CreateCommand(cmd_line);

    pid_t pid = fork();

    if (pid == -1) {
        perror("smash error: fork failed");
        return;
    }
    else if (pid == 0){ // son execute
        if (setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            exit(1);
        }
        if(close(1) == -1){
            perror("smash error: close failed");
            exit(0);
        }
        size_t pos=((string)cmd_line).find('>');
        string m_text_file;
        if(type)
        {
            m_text_file=_trim(((string)cmd_line).substr(pos+2));
        }
        else
        {
            m_text_file=_trim(((string)cmd_line).substr(pos+1));
        }

        int fd;
        if (type){
            fd = open(m_text_file.c_str(),O_CREAT | O_WRONLY | O_APPEND, 0664);
        }else{
            fd = open(m_text_file.c_str(),O_CREAT | O_WRONLY | O_TRUNC, 0664);
        }
        if (fd == -1){
            perror("smash error: open failed");
            exit(0);
        }
        if (command == nullptr){
            return;
        }
        command->execute();
        exit(0);

    } else{ // parent wait pid
        if (waitpid(pid,NULL,WUNTRACED) == -1){
            perror("smash error: waitpid failed");
            return;
        }
    }
}




/////////////////////////////////////////////////////

////////////////////////////// Pipe Command

PipeCommand::PipeCommand(const char *cmd_line): Command(cmd_line), regular(!(((const string)cmd_line).find("|&") != string::npos)){
    // find the commands
    int i=0;
    while(*(cmd_line+i) && cmd_line[i]!='|' ){
        i++;
    }
    for (int j = 0; j < i; ++j) {
        cmd_1+=cmd_line[j];
    }
    if(regular){
        i+=1; //2?
    } else{
        i+=2; //3?
    }
    while(*(cmd_line+i)){
        cmd_2+=cmd_line[i];
        i++;
    }
}

void PipeCommand::execute() {

    SmallShell &smash = SmallShell::getInstance();
    int fd[2];
    if (pipe(fd) == -1) {
        perror("smash error: pipe failed");
        return;
    }
    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("smash error: fork failed");
        return;
    }
    if (pid1 == 0) {
        if (setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            exit(1);
        }
        if (regular) {
            if (dup2(fd[1], 1) == -1) {
                perror("smash error: dup2 failed");
                exit(0);
            }
            if (close(1) == -1) {
                perror("smash error: close failed");
                exit(0);
            }
        } else {
            if (dup2(fd[1], 2) == -1) {
                perror("smash error: dup2 failed");
                exit(0);
            }
            if (close(2) == -1) {
                perror("smash error: close failed");
                exit(0);
            }
        }
        if(close(fd[0]) == -1){
            perror("smash error: close failed");
            exit(0);
        }
        if(close(fd[1]) == -1){
            perror("smash error: close failed");
            exit(0);
        }
        Command* command1 = smash.CreateCommand(cmd_1.c_str());
        command1->execute();
        exit(0);
    }
    pid_t pid2 = fork();
    if(pid2 == -1){
        perror("smash error: fork failed");
        return;
    }
    if(pid2 == 0){
        if (setpgrp() == -1) {
            perror("smash error: setpgrp failed");
            exit(1);
        }
        if (dup2(fd[0], 0) == -1) {
            perror("smash error: dup2 failed");
            exit(0);
        }
        if (close(0) == -1) {
            perror("smash error: close failed");
            exit(0);
        }
        if(close(fd[0]) == -1){
            perror("smash error: close failed");
            exit(0);
        }
        if(close(fd[1]) == -1){
            perror("smash error: close failed");
            exit(0);
        }
        Command* command2 = smash.CreateCommand(cmd_2.c_str());
        command2->execute();
        exit(0);
    }

    if(close(fd[0]) == -1){
        perror("smash error: close failed");
        exit(0);
    }
    if(close(fd[1]) == -1){
        perror("smash error: close failed");
        exit(0);
    }
    if (waitpid(pid1,NULL,WUNTRACED) == -1){
        perror("smash error: waitpid failed");
        return;
    }
    if(waitpid(pid2,NULL,WUNTRACED) == -1){
        perror("smash error: waitpid failed");
        return;
    }

}

/////////////////////////////////////////////////////

////////////////////////////// listdir Command

ListDirCommand::ListDirCommand(const char *cmd_line): BuiltInCommand(cmd_line){}

void ListDirCommand::execute() {

    if(num_of_args > 2){
        cerr<<"smash error: listdir:too many arguments"<<endl;
        return;
    }

    SmallShell& smash=SmallShell::getInstance();
    struct linux_dirent  *d;
    char buf[4096];
    char d_type;
    vector<string> files;
    vector<string> directories;
    int fd;

    if(num_of_args == 2){
        fd = open(arguments[1], O_RDONLY | O_DIRECTORY);
    }else{
        fd = open(smash.prev_dir.c_str(), O_RDONLY | O_DIRECTORY);
    }

    if (fd == -1){
        perror("smash error: directory does not exist");
        exit(0);
    }
    int nread = syscall(SYS_getdents, fd, buf, 4096);
    if( nread == -1){
        perror("smash error: getdents failed");
        exit(0);
    }
    for (size_t bpos = 0; bpos < nread;) {
        d = (struct linux_dirent *) (buf + bpos);
        d_type = *(buf + bpos + d->d_reclen - 1);
        if(d_type == DT_REG) files.push_back(d->d_name);
        if (d_type == DT_DIR) directories.push_back(d->d_name);
        bpos += d->d_reclen;
    }

    sort(files.begin(),files.end());
    sort(directories.begin(),directories.end());

    for(auto file : files){
        cout<<"file: "<<file<<endl;
    }
    for(auto dir : directories){
        cout<<"directory: "<<dir<<endl;
    }
    return;
}

/////////////////////////////////////////////////////

////////////////////////////// Get User Command

GetUserCommand::GetUserCommand(const char *cmd_line): BuiltInCommand(cmd_line){}

void GetUserCommand::execute() {

    if(num_of_args != 2){
        cerr<<"smash error: getuser: too many arguments"<<endl;
        return;
    }
    uid_t uid;
    gid_t gid;
    char buff[4096];
    int fd;
    string path="/proc/";
    path+=arguments[1];
    path+="/status";

    fd = open(path.c_str(), O_RDONLY );
    read(fd,buff,4096);
    close(fd);
    char* all_lines=strtok(buff,"\n");
    while(all_lines)
    {
        if(string(all_lines).substr(0,4) == "Uid:")
        {
            uid=stoi(string(all_lines).substr(4));
        }
        if(string(all_lines).substr(0,4) == "Gid:")
        {
            gid=stoi(string(all_lines).substr(4));
        }
        all_lines=strtok(nullptr,"\n");
    }

    if (uid == -1 || gid == -1)
    {
        cerr<<"smash error: getuser: procces "<<arguments[1]<<" does not exist"<<endl;
        return;
    }

    struct passwd* user=getpwuid(uid);
    struct group* g=getgrgid(gid);
    if(user == nullptr || g == nullptr )
    {
        cerr<<"smash error: getuser: procces "<<arguments[1]<<" does not exist"<<endl;
        return;
    }
    cout<<"User: "<<user->pw_name<<endl;
    cout<<"Group: "<<g->gr_name<<endl;



    return;
}



