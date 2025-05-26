#include <iostream>
#include <cstdlib>
#include <sys/wait.h>  // For WEXITSTATUS and related macros

enum Status {
    SYSTEM_FAILED = -1,
    ACTIVE = 0,
    INACTIVE = 3,         // systemctl returns 3 for inactive
    SYSTEMCTL_FAILED = 1  // systemctl returns 1 for generic failure
};

bool getServiceStatus(const std::string& serviceName) {
    int ret = system(("systemctl is-active --quiet " + serviceName).c_str());

    if (ret == -1) {
        std::cout << "system() call failed" << std::endl;
        return false;
    }

    int exitStatus = WEXITSTATUS(ret);

    if (exitStatus == ACTIVE) {
        std::cout << serviceName << " is active" << std::endl;
        return true;
    } else if (exitStatus == INACTIVE) {
        std::cout << serviceName << " is inactive" << std::endl;
        return false;
    } else if (exitStatus == SYSTEMCTL_FAILED) {
        std::cout << "systemctl failed (e.g., service not found)" << std::endl;
        return false;
    } else {
        std::cout << "Unknown failure, exit code: " << exitStatus << std::endl;
        return false;
    }
}

int main() {
    getServiceStatus("ssh.service");
    return 0;
}
