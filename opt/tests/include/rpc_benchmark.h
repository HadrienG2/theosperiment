 /* Benchmark code used to test the performance of the RPC model for
    inter-process communication

      Copyright (C) 2011  Hadrien Grasland

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA */

#ifndef _RPCBENCH_H_
#define _RPCBENCH_H_

#include <address.h>
#include <hack_stdint.h>
#include <kstring.h>
#include <pid.h>

namespace Tests {
    extern void* default_ptr_value;
    
    //Support structures
    struct ServerParamDescriptor {
        KString type;
        uint32_t value_size;
        void* default_value;
        
        size_t heap_size();
        ServerParamDescriptor& operator=(const ServerParamDescriptor& source);
    };
    
    struct ServerCallDescriptor {
        void* function_ptr;
        KString call_name;
        uint32_t params_amount;
        ServerParamDescriptor* params;
        
        size_t heap_size();
        ServerCallDescriptor& operator=(const ServerCallDescriptor& source);
    };
    
    struct ClientParamDescriptor {
        KString type;
        uint32_t value_size;
        
        size_t heap_size();
        ClientParamDescriptor& operator=(const ServerParamDescriptor& source);
        bool compatible_with(const ServerParamDescriptor& request) const;
    };
    
    struct ClientCallDescriptor {
        KString server_name;
        KString call_name;
        uint32_t params_amount;
        ClientParamDescriptor* params;
        
        ClientCallDescriptor(const KString& server_name, const ServerCallDescriptor& server_desc);
        size_t heap_size();
        ClientCallDescriptor& operator=(const ClientCallDescriptor& source);
        bool compatible_with(const ServerCallDescriptor& request) const;
    };
    
    struct RemoteCallConnection {
        ClientCallDescriptor* client_desc;
        ServerCallDescriptor* server_desc;
        size_t compat_stack_size;
        void* compat_stack;
        
        RemoteCallConnection(ClientCallDescriptor& client_desc,
                             ServerCallDescriptor& server_desc);
    };
    
    struct AsyncQueue {
        //Requests in the queue have the function pointer + parameters form. A null pointer
        //is used for invalid requests, used at the end of the queue to request a come-back to the
        //start of the queue
        void* queue_start;
        size_t queue_size;
        void* write_pos;
        void* read_pos;
        
        AsyncQueue(const size_t queue_size);
        ~AsyncQueue();
        bool find_space(const size_t amount); //Adjusts the write_pos pointer so that there's enough
                                              //room to write a request of "amount" bytes, plus a
                                              //NULL pointer if needed
        bool find_valid_descriptor(); //Adjust the read_pos pointer so that it reaches a valid,
                                      //non-NULL call in the queue, if possible
    };
    
    struct ProcessDescriptor { //Used to represent elements of a future "process table"
        bool active; //If this entry is false, no process is currently associated to this PID
        KString name;
        
        ProcessDescriptor() : active(true), name("dummy_process_name_of_flaming_deaths.bin") {}
    };
        
    void benchmark_rpc(); //RPC performance benchmark
    
    //Individual tests
    void rpc_server_init_bench(); //Used to evaluate the time taken by the initialization of many
                                  //"heavy" servers
    void rpc_client_init_bench(); //Used to evaluate the time taken by the initialization of many
                                  //"heavy" clients
    void rpc_threaded_bench(); //Used to evaluate the overhead of remote calls in threaded operation
    void rpc_async_bench(); //Used to evaluate the overhead in asynchronous operation
    
    //Auxiliary functions
    // * Call descriptor generators
    ServerCallDescriptor& generate_dummy_server_desc();
    ServerCallDescriptor& generate_wanted_server_desc();
    
    // * Fake process table generator
    ProcessDescriptor* generate_process_table(const unsigned int server_amount,
                                              const unsigned int running_pids,
                                              const KString& server_name);
    
    // * Emulate locating a server in a process table
    PID find_server(const KString& server_name,
                    const ProcessDescriptor* process_table,
                    const unsigned int server_number,
                    const unsigned int running_pids);
    
    // * Fake server call descriptor table generator
    ServerCallDescriptor* generate_server_descriptor_table(const unsigned int reg_calls_per_server,
                                                           const unsigned int calls_per_server);
    
    // * Emulate locating a descriptor in a server descriptor table
    ServerCallDescriptor& find_descriptor(const ClientCallDescriptor& request,
                                          ServerCallDescriptor* serv_descs,
                                          const unsigned int descriptor_number,
                                          const unsigned int calls_per_server);
    
    // * Fake client-side stub for threaded RPC
    bool remote_call_client_stub_threaded(RemoteCallConnection& connec,
                                          uint64_t param1,
                                          uint64_t param2 = 0xDEADBEEFBADB002E,
                                          uint64_t param3 = 0xDEADBEEFBADB002E,
                                          uint64_t param4 = 0xDEADBEEFBADB002E,
                                          uint64_t param5 = 0xDEADBEEFBADB002E,
                                          uint64_t param6 = 0xDEADBEEFBADB002E,
                                          uint64_t param7 = 0xDEADBEEFBADB002E,
                                          uint64_t param8 = 0xDEADBEEFBADB002E,
                                          void* param9 = default_ptr_value);
    
    // * Putting a call in the asynchronous RPC queue (async client stub)
    bool remote_call_client_stub_async(AsyncQueue& queue,
                                       RemoteCallConnection& connec,
                                       uint64_t param1,
                                       uint64_t param2 = 0xDEADBEEFBADB002E,
                                       uint64_t param3 = 0xDEADBEEFBADB002E,
                                       uint64_t param4 = 0xDEADBEEFBADB002E,
                                       uint64_t param5 = 0xDEADBEEFBADB002E,
                                       uint64_t param6 = 0xDEADBEEFBADB002E,
                                       uint64_t param7 = 0xDEADBEEFBADB002E,
                                       uint64_t param8 = 0xDEADBEEFBADB002E,
                                       void* param9 = default_ptr_value);
    
    // * Run a remote call from the asynchronous queue
    void run_call_from_queue(AsyncQueue& queue, void* stack);
    
    // * Fake server-side stub for both approaches
    void remote_call_server_stub(void* function_ptr, void* stack);
    
    // * Dummy call which checks its parameters
    void dummy_remote_call(uint64_t param1,
                           uint64_t param2 = 0xDEADBEEFBADB002E,
                           uint64_t param3 = 0xDEADBEEFBADB002E,
                           uint64_t param4 = 0xDEADBEEFBADB002E,
                           uint64_t param5 = 0xDEADBEEFBADB002E,
                           uint64_t param6 = 0xDEADBEEFBADB002E,
                           uint64_t param7 = 0xDEADBEEFBADB002E,
                           uint64_t param8 = 0xDEADBEEFBADB002E,
                           void* param9 = (void*) 0x10000000,
                           void* param10 = (void*) 0x10000000);
}

#endif
