#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <map>
#include <sys/stat.h>
#include <cstdlib>
#define PROMPT "sish> "
using namespace std;

std::map<pid_t, std::string> background_processes;

volatile sig_atomic_t exit_flag = 0;
volatile sig_atomic_t sigint_triggered = 0;

void handle_SIGCHLD(int) {
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (background_processes.count(pid)) {
            std::cout << "\n[Background Process] PID: " << pid << " completed." << std::endl;
            background_processes.erase(pid);
            std::cout << PROMPT << std::flush;
        }
    }
}

void handle_SIGINT(int) {
    if (exit_flag) {
        return;
    }
    if (!sigint_triggered) {
        sigint_triggered = 1;
        std::cout << "\nSIGINT (Ctrl+C) ignored. Type 'exit' to quit the shell.\n" << PROMPT << std::flush;
    }
}

std::vector<std::string> tokenize(const std::string &input) {
    std::vector<std::string> tokens;
    char *input_cstr = strdup(input.c_str());
    char *token = strtok(input_cstr, " ");
    while (token != nullptr) {
        tokens.push_back(token);
        token = strtok(nullptr, " ");
    }
    free(input_cstr);
    return tokens;
}

bool is_command_valid(const std::string &command) {
    if (command.find('/') != std::string::npos) {
        struct stat buffer;
        if (stat(command.c_str(), &buffer) != 0) {
            std::cerr << "Command not found: <" << command << ">" << std::endl;
            return false;
        }
        if (!(buffer.st_mode & S_IXUSR)) {
            std::cerr << "Permission denied: <" << command << ">" << std::endl;
            return false;
        }
        return true;
    }

    const char *path_env = getenv("PATH");
    if (!path_env) {
        std::cerr << "Environment variable PATH not found." << std::endl;
        return false;
    }

    std::string path_env_str(path_env);
    size_t start = 0, end;
    while ((end = path_env_str.find(':', start)) != std::string::npos) {
        std::string path_dir = path_env_str.substr(start, end - start);
        std::string full_path = path_dir + "/" + command;
        if (access(full_path.c_str(), X_OK) == 0) {
            return true;
        }
        start = end + 1;
    }

    std::string last_path_dir = path_env_str.substr(start);
    std::string full_path = last_path_dir + "/" + command;
    if (access(full_path.c_str(), X_OK) == 0) {
        return true;
    }

    std::cerr << "Command not found: <" << command << ">" << std::endl;
    return false;
}

void execute_command(const std::vector<std::string> &tokens, bool is_background) {
    if (tokens.empty()) {
        std::cerr << "Error: No command provided." << std::endl;
        return;
    }

    const std::string &command = tokens[0];
    if (!is_command_valid(command)) {
        return;
    }

    std::vector<char *> args;
    for (const auto &token : tokens) {
        args.push_back(const_cast<char *>(token.c_str()));
    }
    args.push_back(nullptr);

    pid_t pid = fork();
    if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        if (is_background) {
            int dev_null = open("/dev/null", O_RDWR);
            if (dev_null != -1) {
                dup2(dev_null, STDIN_FILENO);
                dup2(dev_null, STDOUT_FILENO);
                dup2(dev_null, STDERR_FILENO);
                close(dev_null);
            }
        }
        execvp(args[0], args.data());
        perror("Error");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        if (is_background) {
            std::string command_line;
            for (const auto &token : tokens) {
                command_line += token + " ";
            }
            background_processes[pid] = command_line;
            std::cout << "[Background Process] PID: " << pid << " started." << std::endl;
        } else {
            waitpid(pid, nullptr, 0);
        }
    } else {
        perror("Fork failed");
    }
}

void list_background_processes() {
    if (background_processes.empty()) {
        std::cout << "No background processes running." << std::endl;
    } else {
        std::cout << "Running background processes:" << std::endl;
        for (const auto &entry : background_processes) {
            std::cout << "PID: " << entry.first << ", Command: " << entry.second << std::endl;
        }
    }
}

void terminate_background_processes() {
    for (const auto &entry : background_processes) {
        kill(entry.first, SIGKILL);
        std::cout << "Terminated background process: PID " << entry.first << std::endl;
    }
}

void run_shell() {
    signal(SIGCHLD, handle_SIGCHLD);
    signal(SIGINT, handle_SIGINT);

    while (true) {
        sigint_triggered = 0;
        std::cout << PROMPT;
        std::string input;
        std::getline(std::cin, input);

        if (input.empty()) {
            continue;
        }

        std::vector<std::string> tokens = tokenize(input);

        if (tokens.empty()) {
            continue;
        }

        bool is_background = false;
        if (tokens.back() == "&") {
            is_background = true;
            tokens.pop_back();
        }

        if (tokens[0] == "exit") {
            exit_flag = 1;
            terminate_background_processes();
            std::cout << "Exiting shell..." << std::endl;
            break;
        }

        if (tokens[0] == "jobs") {
            list_background_processes();
            continue;
        }

        if (tokens[0] == "cd") {
            if (tokens.size() < 2) {
                std::cerr << "Error: 'cd' requires a target directory" << std::endl;
            } else {
                if (chdir(tokens[1].c_str()) != 0) {
                    perror("cd");
                }
            }
            continue;
        }

        execute_command(tokens, is_background);
    }
}

int main() {
    cout << "This is shell made by 2024313206 Jeong Ho Won" <<endl;
    cout << "for System Programmming [SWE2001_43]" << endl;
    std::string asciiArt = R"(
        _    .  ,   .           .
    *  / \_ *  / \_      _  *        *   /\'__        *
      /    \  /    \,   ((        .    _/  /  \  *'.
 .   /\/\  /\/ :' __ \_  `          _^/  ^/    `--.
    /    \/  \  _/  \-'\      *    /.' ^_   \_   .'\  *
  /\  .-   `. \/     \ /==~=-=~=-=-;.  _/ \ -. `_/   \
 /  `-.__ ^   / .-'.--\ =-=~_=-=~=^/  _ `--./ .-'  `-
/         `.  / /       `.~-^=-=~=^=.-'      '-._ `._
    )";
    cout << asciiArt << endl;
    cout << "------ Starting sish from now on ------------------------" << endl;
    run_shell();
    return 0;
}
