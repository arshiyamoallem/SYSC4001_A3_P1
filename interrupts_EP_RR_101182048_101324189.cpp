/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include<interrupts_101182048_101324189.hpp>

void FCFS(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.arrival_time > second.arrival_time); 
                } 
            );
}

void EP(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.PID < second.PID); 
                } 
            );
}

std::tuple<std::string /* add std::string for bonus mark */ > run_simulation(std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).

    unsigned int current_time = 0;
    const unsigned int QUANTUM = 100; // quantum for RR scheduling
    unsigned int time_slice = 0; // time slice counter
    PCB running;

    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

    //make the output table (the header row)
    execution_status = print_exec_header();

    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(!all_process_terminated(job_list) || job_list.empty()) {

        //Inside this loop, there are three things you must do:
        // 1) Populate the ready queue with processes as they arrive
        // 2) Manage the wait queue
        // 3) Schedule processes from the ready queue

        //Population of ready queue is given to you as an example.
        //Go through the list of proceeses
        for(auto &process : list_processes) {
            if(process.arrival_time == current_time) {//check if the AT = current time
                //if so, assign memory and put the process into the ready queue
                assign_memory(process);

                process.state = READY;  //Set the process state to READY
                ready_queue.push_back(process); //Add the process to the ready queue
                job_list.push_back(process); //Add it to the list of processes

                execution_status += print_exec_status(current_time, process.PID, NEW, READY);
            }
        }

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        //This mainly involves keeping track of how long a process must remain in the ready queue

        for (int i = 0; i< wait_queue.size();) {
            PCB p = wait_queue[i];

            if (p.io_termination_time <= (int)current_time) {
                states old_state = p.state; // old state of the process 
                p.state = READY; 
                ready_queue.push_back(p);
                sync_queue(job_list, p);

                execution_status += print_exec_status(current_time, p.PID, old_state, READY);
                
                wait_queue.erase(wait_queue.begin() + i);
            } else {
                i++;
            }
        }
        EP(ready_queue);

        /////////////////////////////////////////////////////////////////

        //////////////////////////SCHEDULER//////////////////////////////
        // FCFS(ready_queue); //example of FCFS is shown here

        // step 1 if CPU is idle
        if (running.state == NOT_ASSIGNED && !ready_queue.empty()) {
            running = ready_queue.front();
            ready_queue.erase(ready_queue.begin());
            
            states old_state = running.state;
            running.state = RUNNING;

            // first time the process runs
            if (running.start_time < 0) {
                running.start_time = current_time; 
            }
            
            time_slice = 0; // reset RR quantum timer

            sync_queue(job_list, running);
            execution_status += print_exec_status(current_time, running.PID, old_state, RUNNING);
        }

        // step 2 if CPU is running
        if (running.state == RUNNING) {
            running.remaining_time--;
            time_slice++;
            
            // check if process has an I/O request
            if (running.io_freq > 0 && running.remaining_time > 0 && 
                (running.processing_time - running.remaining_time) > 0 && 
                (running.processing_time - running.remaining_time) % running.io_freq == 0) {
                states old_state = running.state;
                running.state = WAITING;
                
                execution_status += print_exec_status(current_time, running.PID, old_state, WAITING);

                PCB temp = running;
                temp.io_termination_time = current_time + running.io_duration;

                wait_queue.push_back(temp);
                sync_queue(job_list, temp);
                idle_CPU(running);
            }
            // check if process has been terminated
            else if (running.remaining_time == 0) {
                states old_state = running.state;
                running.state = TERMINATED;
                execution_status += print_exec_status(current_time, running.PID, old_state, TERMINATED);
                sync_queue(job_list, running);

                free_memory(running);
                idle_CPU(running);
            }
            // check for preemption
            else if (!ready_queue.empty()) {
                PCB top = ready_queue.front(); // highest priority process
                
                if (top.PID < running.PID) {
                    states old_state = running.state;
                    running.state = READY;
                    
                    execution_status += print_exec_status(current_time, running.PID, old_state, READY);
                    
                    ready_queue.push_back(running);
                    sync_queue(job_list, running);
                    EP(ready_queue);

                    PCB p_next = ready_queue.front();
                    ready_queue.erase(ready_queue.begin());
                    
                    states old_state_p_next = p_next.state;
                    running = p_next;
                    running.state = RUNNING;
                    execution_status += print_exec_status(current_time, running.PID, old_state_p_next, RUNNING);
                    sync_queue(job_list, running);

                    time_slice = 0; // reset RR quantum timer
                }
            } 
            // checl if quantum has expired
            else if (time_slice == QUANTUM) {
                states old_state = running.state;
                running.state = READY;
                execution_status += print_exec_status(current_time, running.PID, old_state, READY);

                ready_queue.push_back(running);
                sync_queue(job_list, running);
                idle_CPU(running);
                time_slice = 0;
            }
        }
        current_time++;
        /////////////////////////////////////////////////////////////////

    }
    
    //Close the output table
    execution_status += print_exec_footer();

    return std::make_tuple(execution_status);
}


int main(int argc, char** argv) {

    //Get the input file from the user
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_input_file.txt>" << std::endl;
        return -1;
    }

    //Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    //Ensure that the file actually opens
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    //Parse the entire input file and populate a vector of PCBs.
    //To do so, the add_process() helper function is used (see include file).
    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    //With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process);

    write_output(exec, "execution.txt");

    return 0;
}