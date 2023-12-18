#include <iostream>
#include <chrono>
#include <fstream>
#include  <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <vector>
#include <string>

#include "Tokenizer.h"

// all the basic colours for a shell prompt
#define RED     "\033[1;31m"
#define GREEN	"\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE	"\033[1;34m"
#define WHITE	"\033[1;37m"
#define NC      "\033[0m"

using namespace std;
//chdir

int main () {
    vector <int> background_processes; // list of background processes pid
    bool isBg = false;

    int saved_stdin = dup(STDIN_FILENO);
    int saved_stdout = dup(STDOUT_FILENO);

    //implement cd with chdir()
    //if dir (cd <dir>) is "-", then go to previous working directory
    //variable storing previous working directory (it needs to be decalred outside loop so it can be accessed in the loop)
    char * buff = new char[256];
    string current_dir = getcwd(buff, 256);
    chdir("..");
    string previous_dir = getcwd(buff, 256);
    
    chdir(current_dir.c_str());
    bool changed_dir = false;
    for (;;) {
        //implement iteration over vector of bg pid (vector also declared outside loop)
        //waitpid() using flag for non-blocking
        for(long unsigned int i = 0; i < background_processes.size(); i++)
		{
			if(waitpid(background_processes[i], 0, WNOHANG) == background_processes[i])
			{ 
				cout << "Process: " <<background_processes[i] << " ended" <<endl;
				//remove process from list (Reaping)
				background_processes.erase(background_processes.begin() +i);
				i--; //keeping i at the same exact spot
			}
		}

 
        //USER PROMPT

        // getcwd, getenv("USER"), time()+ctime()
        // need date/time, username, and absolute path to current dir
        const auto now = std::chrono::system_clock::now();
        const std::time_t t_c = std::chrono::system_clock::to_time_t(now);
        char * user = getenv("USER");
        
        

        cout << YELLOW << "Shell$" << " " << NC << std::ctime(&t_c) << " " << user << ":" << current_dir << " ";

        
        // get user inputted command
        string input;
        getline(cin, input);

        if (input == "exit") {  // print exit message and break out of infinite loop
            cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
            break;
        }

        

        // get tokenized commands from user input
        Tokenizer tknr(input);
        if (tknr.hasError()) {  // continue to next prompt if input had an error
            continue;
        }

        
        // // print out every command token-by-token on individual lines
        // // prints to cerr to avoid influencing autograder
        /*for (auto cmd : tknr.commands) {
            for (auto str : cmd->args) {
                cerr << "|" << str << "| ";
            }
            if (cmd->hasInput()) {
                cerr << "in< " << cmd->in_file << " ";
            }
            if (cmd->hasOutput()) {
                cerr << "out> " << cmd->out_file << " ";
            }
            cerr << endl;
        }*/
        
        //handling cd commands - COMPLETED
        if(tknr.commands.at(0)->args.at(0) == "cd")
        {
            
            if(tknr.commands.at(0)->args.at(1) == "-" and changed_dir == false)
            {
                chdir(current_dir.c_str());
            }
            else if(tknr.commands.at(0)->args.at(1) == "-" and changed_dir == true)
            {
                chdir(previous_dir.c_str());
                string temp = previous_dir;
                previous_dir = current_dir;
                current_dir = temp;
            }
            else
            {
                string input_dir = tknr.commands.at(0)->args.at(1);
                chdir(input_dir.c_str());
                string temp = current_dir;
                current_dir = getcwd(buff, 256);
                previous_dir = temp;
                changed_dir = true;
            }
        }
        else
        { //not a cd command so all other cases

            //for piping ----- COMPLETED
            // for cmd : commands
            //      call pipe() to make pipe
            //      fork() - in child, redirect stdout; in parent, redirect stdin
            //      ^ is already written
            // add checks for the first/last command
            for(long unsigned int i = 0; i < tknr.commands.size(); i++)
            {
                Command* arg = tknr.commands.at(i);
                int fd[2];
                pipe(fd);
                
                // fork to create child
                pid_t pid = fork();
                if (pid < 0) {  // error check
                    perror("fork");
                    exit(2);
                }
                
                //add check for bg process - add pid to vector if bg and don't waitpid() in parent
                if(arg->isBackground())
                {
                    isBg = true;
                    background_processes.push_back(pid);
                }
                else{
                    isBg = false;
                }

                

                if (pid == 0) {  // if child, exec to run command
                    // run single commands with no arguments
                    //implement multiple arguments - iterate over args of current command to make - COMPLETED
                    //      char* array
                    char** args = new char*[arg->args.size()+1];
                    for(long unsigned int i = 0; i < arg->args.size(); i++)
                    {
                        args[i] = (char*)arg->args.at(i).c_str();
                    }
                    args[arg->args.size()] = nullptr;
                    
                    //if current command is redirected then open file and dup2 stdin/stdout that's being redirected
                    //fd = open(filename)
                    //dup2 fd over stdin/stdout
                    //implement it safely for both at same time -- COMPLETED
                    if(arg->hasInput())
                    {
                        int fd = open(arg->in_file.c_str(), O_RDONLY, 0666);
                        dup2(fd, STDIN_FILENO);
                        close(fd);

                    }
                    if(arg->hasOutput())
                    {
                        int fd = open(arg->out_file.c_str(), O_WRONLY | O_CREAT, 0666);
                        dup2(fd, STDOUT_FILENO);
                        close(fd);
                    }

                    //MULTIPE PIPES HANDLING ------ COMPLETED
                    if(i < tknr.commands.size() - 1)
                    {
                        //redirect stdout to the pipe using dup2(fd[1], stdout)
                        dup2(fd[1], STDOUT_FILENO); 
                        //last process dont have to do this
                        close(fd[0]);
                        
                        //execvp
                        // arg->args.size() iterate over them and char*
                        
                    }
                    

                    if (execvp(args[0], args) < 0) {  // error check
                        perror("execvp");
                        exit(2);
                    }
                }
                else {  // if parent, wait for child to finish
                    if(isBg == false)
                    {
                        int status = 0;
                    
                        waitpid(pid, &status, 0);
                        if (status > 1) {  // exit if child didn't exec properly
                            exit(status);
                        }
                        dup2(fd[0], STDIN_FILENO);
                        close(fd[1]);
                    }
                    
                }
            }
            //restore stdin/stdout (variable would be outside the loop)
            dup2(saved_stdin, STDIN_FILENO);
            dup2(saved_stdout, STDOUT_FILENO);
        }

        //delete[] cwd;
        //delete[] previous_dir;
        //delete[] buff;

    }
    delete[] buff;

}
