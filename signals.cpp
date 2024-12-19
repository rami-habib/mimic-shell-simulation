#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
    // TODO: Add your implementation
    SmallShell& smash = SmallShell::getInstance();
    cout << "smash: got ctrl-C" << endl;
    if(smash.curr_running_fg != -1) {
        if (kill(smash.curr_running_fg, 9) == -1) {
            perror("smash error: kill failed");
            return;
        }
        cout << "smash: process " << smash.curr_running_fg << " was killed." << endl;
        //smash.curr_running_fg = -1;
    }
}
