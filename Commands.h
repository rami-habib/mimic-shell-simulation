#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <memory>
#include <list>
#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)


#define DO_SYS( syscall, name ) do { \
if( (syscall) == -1 ) { \
perror("smash error: " #name " failed"); \
return; \
}         \
} while(0)  \


using namespace std;
class Command {



public:
    const char* cmd_line;
    char* arguments[20];
    int num_of_args;
    bool is_bg;


    Command(const char *cmd_line);

    virtual ~Command()=default;

    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line);

    virtual ~BuiltInCommand()=default;
};

class ExternalCommand : public Command {
public:
    bool is_complex;
    ExternalCommand(const char *cmd_line);

    virtual ~ExternalCommand()=default;

    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    bool regular;
    std::string cmd_1="";
    std::string cmd_2="";
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand()=default;

    void execute() override;
};

class WatchCommand : public Command {
    // TODO: Add your data members
public:
    WatchCommand(const char *cmd_line);

    virtual ~WatchCommand()=default;

    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
    bool type; ///// false='<' , true='<<'
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() =default;

    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
    ChangeDirCommand(const char *cmd_line);

    virtual ~ChangeDirCommand() =default;

    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmd_line);

    virtual ~GetCurrDirCommand() =default;

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand()=default;

    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:

public:
    JobsList *jobs;
    QuitCommand(const char *cmd_line, JobsList *jobs);

    virtual ~QuitCommand() =default;

    void execute() override;
};


class JobsList {
public:
    class JobEntry {
    public:
        int job_id;
        pid_t pid;
        const string cmd_line;
        bool is_stopped;
        JobEntry(int jobId, int pid,  const string &cmdLine, bool isStopped);
        //   JobEntry(pid_t pid)
    };
    // TODO: Add your data members
public:
    vector<std::shared_ptr<JobEntry>> jobs;
    int max_job_id;

    JobsList();

    ~JobsList()=default;;

    void addJob(Command *cmd,int pid, bool isStopped = false);

    void printJobsList();

    void killAllJobs();

    void updateJobsStatus();

    void updateMaxJobID();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);
    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList* jobs;
public:
    JobsCommand(const char *cmd_line, JobsList *jobs);

    virtual ~JobsCommand() =default;

    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsList *jobs;
    KillCommand(const char *cmd_line, JobsList *jobs);

    virtual ~KillCommand() =default;

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    JobsList* jobs;

public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs);

    virtual ~ForegroundCommand() =default;

    void execute() override;
};

class ListDirCommand : public BuiltInCommand {
public:
    ListDirCommand(const char *cmd_line);

    virtual ~ListDirCommand() =default;

    void execute() override;
};

class GetUserCommand : public BuiltInCommand {
public:
    GetUserCommand(const char *cmd_line);

    virtual ~GetUserCommand()=default;

    void execute() override;
};

class aliasCommand : public BuiltInCommand {
public:
    aliasCommand(const char *cmd_line);

    virtual ~aliasCommand() =default;

    void execute() override;
};

class unaliasCommand : public BuiltInCommand {
public:
    unaliasCommand(const char *cmd_line);

    virtual ~unaliasCommand() =default;

    void execute() override;
};


class SmallShell {
private:
    // TODO: Add your data members
    SmallShell();

public:
    class Node{
    public:
        string name;
        const char *cmd_line;
        Node(const string& name,const char *cmd_line):name(name),cmd_line(cmd_line){}
        ~Node()=default;
    };

    string prompt ;
    JobsList* jobs;
    pid_t pid;
    string prev_dir;
    std::vector<std::shared_ptr<Node>> aliases;
    pid_t curr_running_fg ;

    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    ~SmallShell();

    void executeCommand(const char *cmd_line);
    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
