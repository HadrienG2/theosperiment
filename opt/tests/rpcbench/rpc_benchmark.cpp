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

#include <address.h>
#include <fake_syscall.h>
#include <kmaths.h>
#include <mem_allocator.h>
#include <kstring.h>
#include <new.h>
#include <rpc_benchmark.h>
#include <test_display.h>


namespace Tests {
    void* default_ptr_value = (void*) 0xffffffffffffffff;

    size_t ServerParamDescriptor::heap_size() {
        size_t to_be_allocd = type.heap_size();
        if(default_value != NULL) {
            to_be_allocd+= value_size;
        }
        return to_be_allocd;
    }

    ServerParamDescriptor& ServerParamDescriptor::operator=(const ServerParamDescriptor& source) {
        if(&source == this) return *this;

        type = source.type;
        value_size = source.value_size;
        if(source.default_value == NULL) {
            default_value = NULL;
        } else {
            default_value = kalloc(source.value_size, PID_KERNEL, VMEM_FLAGS_RW, true);
            memcpy(default_value, source.default_value, source.value_size);
        }

        return *this;
    }

    size_t ServerCallDescriptor::heap_size() {
        //Use "fake new" to measure the overhead of array allocation
        size_t to_be_allocd = call_name.heap_size();

        start_faking_allocation();
            to_be_allocd+=
              (size_t) new(PID_KERNEL, VMEM_FLAGS_RW, true) ServerParamDescriptor[params_amount];
        stop_faking_allocation();

        for(unsigned int i = 0; i < params_amount; ++i) {
            to_be_allocd+= params[i].heap_size();
        }

        return to_be_allocd;
    }

    ServerCallDescriptor& ServerCallDescriptor::operator=(const ServerCallDescriptor& source) {
        if(&source == this) return *this;

        function_ptr = source.function_ptr;
        call_name = source.call_name;
        params_amount = source.params_amount;
        params = new(PID_KERNEL, VMEM_FLAGS_RW, true) ServerParamDescriptor[source.params_amount];
        for(uint32_t i = 0; i < source.params_amount; ++i) {
            params[i] = source.params[i];
        }

        return *this;
    }

    size_t ClientParamDescriptor::heap_size() {
        return type.heap_size();
    }

    ClientParamDescriptor& ClientParamDescriptor::operator=(const ServerParamDescriptor& source) {
        type = source.type;
        value_size = source.value_size;

        return *this;
    }

    bool ClientParamDescriptor::compatible_with(const ServerParamDescriptor& request) const {
        if(request.type != type) return false;
        if(request.value_size != value_size) return false;

        return true;
    }

    ClientCallDescriptor::ClientCallDescriptor(const KString& sv_name,
                                               const ServerCallDescriptor& server_desc) {
        server_name = sv_name;
        call_name = server_desc.call_name;
        params_amount = server_desc.params_amount;
        params = new(PID_KERNEL, VMEM_FLAGS_RW, true) ClientParamDescriptor[params_amount];
        for(unsigned int i = 0; i < params_amount; ++i) {
            params[i] = server_desc.params[i];
        }
    }

    size_t ClientCallDescriptor::heap_size() {
        size_t to_be_allocd = server_name.heap_size();
        to_be_allocd+= call_name.heap_size();

        start_faking_allocation();
            to_be_allocd+= (size_t)
              new(PID_KERNEL, VMEM_FLAGS_RW, true) ClientParamDescriptor[params_amount];
        stop_faking_allocation();

        for(unsigned int i = 0; i < params_amount; ++i) {
            to_be_allocd+= params[i].heap_size();
        }

        return to_be_allocd;
    }

    ClientCallDescriptor& ClientCallDescriptor::operator=(const ClientCallDescriptor& source) {
        if(&source == this) return *this;

        server_name = source.server_name;
        call_name = source.call_name;
        params_amount = source.params_amount;
        params = new(PID_KERNEL, VMEM_FLAGS_RW, true) ClientParamDescriptor[source.params_amount];
        for(uint32_t i = 0; i < source.params_amount; ++i) {
            params[i] = source.params[i];
        }

        return *this;
    }

    bool ClientCallDescriptor::compatible_with(const ServerCallDescriptor& request) const {
        unsigned int i;

        //Names must match
        if(request.call_name != call_name) return false;
        //Common parameters must match
        for(i = 0; i < min(params_amount, request.params_amount); ++i) {
            if(params[i].compatible_with(request.params[i]) == false) return false;
        }
        //Extra server parameters (if any) must have a default value
        for(; i < request.params_amount; ++i) {
            if(request.params[i].default_value == NULL) return false;
        }

        return true;
    }

    RemoteCallConnection::RemoteCallConnection(ClientCallDescriptor& cl_desc,
                                               ServerCallDescriptor& sv_desc) {
        client_desc = &cl_desc;
        server_desc = &sv_desc;

        //Create a compatibility stack if needed
        if(cl_desc.params_amount < sv_desc.params_amount) {
            compat_stack_size = 0;
            for(unsigned int i = cl_desc.params_amount; i < sv_desc.params_amount; ++i) {
                compat_stack_size+= sv_desc.params[i].value_size;
            }
            compat_stack = kalloc(compat_stack_size, PID_KERNEL, VMEM_FLAGS_RW, true);
            void* moving_ptr = compat_stack;
            //WARNING : What follows is specific of an x86 cdecl calling convention (last parameter
            //pushed first, stack growing by reducing stack pointer), and is only correct for
            //testing and benchmarking purposes. Even on x86_64, it doesn't work cleanly, because
            //performance optimizations goes in our way by using registers and GCC won't accept
            //any other calling convention (see x86_64.org for more details)
            //DO NOT IMPLEMENT VERBATIM IN FINAL RPC CODE !!!
            for(unsigned int i = cl_desc.params_amount; i < sv_desc.params_amount; ++i) {
                memcpy(moving_ptr,
                       sv_desc.params[i].default_value,
                       sv_desc.params[i].value_size);
                moving_ptr = (void*) (((size_t) moving_ptr) + sv_desc.params[i].value_size);
            }
        } else {
            compat_stack_size = 0;
            compat_stack = NULL;
        }
    }

    AsyncQueue::AsyncQueue(const size_t q_size) {
        queue_size = q_size;
        queue_start = kalloc(queue_size, PID_KERNEL, VMEM_FLAGS_RW, true);
        write_pos = queue_start;
        read_pos = NULL; //Indicates that there's no data to be read
    }

    AsyncQueue::~AsyncQueue() {
        kfree(queue_start);
    }

    bool AsyncQueue::find_space(const size_t amount) {
        size_t write_pos_addr = (size_t) write_pos;
        size_t read_pos_addr = (size_t) read_pos;

        if(read_pos_addr < write_pos_addr) {
            //We must make sure that the write pointer doesn't reach the end of the queue.
            //Should it happen, there should be room left for a NULL function pointer,
            //indicating the situation to the queue reader.
            size_t queue_start_addr = (size_t) queue_start;
            size_t space_before_end = queue_size - (write_pos_addr - queue_start_addr);

            if(space_before_end >= amount + sizeof(RemoteCallConnection*)) return true;

            //Write NULL at the end of the queue, go back to the beginning, and start again.
            void** queue_writer = (void**) write_pos;
            *queue_writer = NULL;
            write_pos = queue_start;
            return find_space(amount);
        } else {
            //The write pointer can collide with the read pointer. Make sure this cannot happen.
            return (read_pos_addr-write_pos_addr >= amount);
        }
    }

    bool AsyncQueue::find_valid_descriptor() {
        //Check that there is something to read
        if(read_pos == NULL) return false;

        //Check that the descriptor is valid, if it is, everything is okay.
        //If the descriptor is NULL, it means we have to go back to the beginning of the queue.
        void** queue_reader = (void**) read_pos;
        if(*queue_reader != NULL) return true;
        read_pos = queue_start;

        //Check that there is something to read there. If not, everything is alright.
        if(read_pos == write_pos) {
            read_pos = NULL;
            return false;
        }

        return true;
    }

    void benchmark_rpc() {
        test_beginning("RPC performance");

        reset_title();
        test_title("Server startup impact");
        rpc_server_init_bench();

        test_title("Client startup impact");
        rpc_client_init_bench();

        test_title("Threaded RPC overhead benchmark");
        rpc_threaded_bench();

        test_title("Asynchronous RPC overhead benchmark");
        rpc_async_bench();
    }

    void rpc_server_init_bench() {
        //Define benchmark parameters here
        const unsigned int NUMBER_OF_SERVERS = 50;
        const unsigned int NUMBER_OF_CALLS = 30;
        ServerCallDescriptor& dummy_desc = generate_dummy_server_desc();

        bench_start();

        for(unsigned int i = 0; i < NUMBER_OF_SERVERS; ++i) {
            //Emulate system call overhead
            fake_syscall();

            //First, fully parse the server's management structures once to determine how much
            //memory must be allocated as a whole. Use "fake new" to measure the overhead of array
            //allocation.
            start_faking_allocation();
                size_t to_be_allocd = (size_t) new ServerCallDescriptor[NUMBER_OF_CALLS];
            stop_faking_allocation();

            for(unsigned int current_call = 0; current_call < NUMBER_OF_CALLS; ++current_call) {
                to_be_allocd+= dummy_desc.heap_size();
            }

            //Then allocate all dynamic data at once. This memory will never be liberated, it is
            //not a bug, but rather an attempt to best emulate system startup.
            void* allocd_buffer = kinit_pool(to_be_allocd);
            if(!allocd_buffer) {
                test_failure("Could not allocate server data");
                return;
            }

            //Allocate and fill server call descriptors
            ServerCallDescriptor* call_descs = new ServerCallDescriptor[NUMBER_OF_CALLS]();
            for(unsigned int j = 0; j < NUMBER_OF_CALLS; ++j) {
                call_descs[j] = dummy_desc;
            }

            kleave_pool();
            //kfree(allocd_buffer);
        }

        bench_stop();
    }

    void rpc_client_init_bench() {
        //Define benchmark parameters here
        const unsigned int NUMBER_OF_CLIENTS = 50;
        const unsigned int SERVERS_PER_CLIENT = 10;
        const unsigned int RUNNING_PIDS = 100;
        const unsigned int REGIS_CALLS_PER_SERVER = 10;
        const unsigned int TOTAL_CALLS_PER_SERVER = 20;
        KString server_name("magic_processes_which_everyone_wants.bin");

        ProcessDescriptor* process_table = generate_process_table(SERVERS_PER_CLIENT,
                                                                  RUNNING_PIDS,
                                                                  server_name);
        ServerCallDescriptor* serv_descs = generate_server_descriptor_table(REGIS_CALLS_PER_SERVER,
                                                                            TOTAL_CALLS_PER_SERVER);

        ClientCallDescriptor client_desc(server_name, serv_descs[0]);

        bench_start();

        //For each client...
        for(unsigned int i = 0; i < NUMBER_OF_CLIENTS; ++i) {
            //For each connected server...
            for(unsigned int j = 0; j < SERVERS_PER_CLIENT; ++j) {
                fake_syscall();

                //Server PID lookup
                find_server(server_name, process_table, j, RUNNING_PIDS);

                //For each remote call...
                for(unsigned int k = 0; k < REGIS_CALLS_PER_SERVER; ++k) {
                    fake_syscall();

                    //Call lookup with find_descriptor()
                    ServerCallDescriptor& server_desc = find_descriptor(client_desc,
                                                                        serv_descs,
                                                                        k,
                                                                        REGIS_CALLS_PER_SERVER);

                    //Create a connection between both
                    RemoteCallConnection connection(client_desc, server_desc);
                }
            }
        }

        bench_stop();
    }

    void rpc_threaded_bench() {
        //Define benchmark parameters here
        const unsigned int NUMBER_OF_CALLS = 10000000;
        KString server_name("magic_processes_which_everyone_wants.bin");

        ServerCallDescriptor& server_desc = generate_dummy_server_desc();

        ClientCallDescriptor client_desc(server_name, server_desc);
        client_desc.params_amount = 10;
        const uint64_t int_param = 0xdeadbeefbadb002e;
        void* ptr_param = kalloc_shareable(1024);
        RemoteCallConnection connec(client_desc, server_desc);

        bench_start();

        for(unsigned int i = 0; i < NUMBER_OF_CALLS; ++i) {
            //Fake remote call ! We'll emulate...
            // * A syscall
            // * Allocation of a 1MB "stack" on server side
            // * Copying of client parameters + optional compatibility stub to the new stack.
            //   Sharing of pointer parameters and pointer conversion.
            // * <The server thread should run from there>
            // * Freeing of the stack on server side when the call is over
            remote_call_client_stub_threaded(connec,
                                             int_param,
                                             int_param,
                                             int_param,
                                             int_param,
                                             int_param,
                                             int_param,
                                             int_param,
                                             int_param,
                                             ptr_param);
        }

        bench_stop();

        kfree(ptr_param);
    }

    void rpc_async_bench() {
        //Define benchmark parameters here
        const unsigned int NUMBER_OF_CALLS = 10000000;
        const size_t ASYNC_QUEUE_SIZE = 1024*1024;
        KString server_name("magic_processes_which_everyone_wants.bin");
        ServerCallDescriptor& server_desc = generate_dummy_server_desc();

        ClientCallDescriptor client_desc(server_name, server_desc);
        client_desc.params_amount = 10;
        const uint64_t int_param = 0xdeadbeefbadb002e;
        void* ptr_param = kalloc_shareable(1024);
        if(!ptr_param) {
            test_failure("Storage space allocation failed !");
            return;
        }
        RemoteCallConnection connec(client_desc, server_desc);

        //Queue where asynchronous calls are added
        AsyncQueue bench_queue(ASYNC_QUEUE_SIZE);

        //Stack used by the async handler
        void* stack = kalloc(1024*1024);

        bench_start();

        for(unsigned int i = 0; i < NUMBER_OF_CALLS; ++i) {
            //Fake remote call ! We'll emulate...
            // * A syscall
            // * A copy of parameters to the asynchronous queue
            // * <At this point we should wait for the server to be ready>
            remote_call_client_stub_async(bench_queue,
                                          connec,
                                          int_param,
                                          int_param,
                                          int_param,
                                          int_param,
                                          int_param,
                                          int_param,
                                          int_param,
                                          int_param,
                                          ptr_param);

            //Once the server is ready, we'll do...
            // * A second syscall
            // * Copying of client parameters + optional compatibility stub to the stack
            //   Sharing of pointer parameters and pointer conversion
            // * <The server thread should run from there>
            run_call_from_queue(bench_queue, stack);
        }

        bench_stop();

        kfree(stack);
        kfree(ptr_param);
    }

    ServerCallDescriptor& generate_dummy_server_desc() {
        //Define the basic "dummy_desc" server call descriptor that will be endlessly copied in
        //the server startup benchmark
        static KString type_normal("supa_dupa_octa_longer_uint64_t");
        static uint32_t value_size_normal = 8;
        static uint64_t default_value_normal = 0xDEADBEEFBADB002E;

        static KString type_ptr("crazy_silly_nutty_zombie_void*");
        static uint32_t value_size_ptr = sizeof(size_t);
        static size_t default_value_ptr = 0x10000000;

        static void* function_ptr = (void*) dummy_remote_call;
        static KString call_name("mega_super_long_function_name_with_speedling_wings");
        static uint32_t params_amount = 10;

        static ServerParamDescriptor params_buffer[10];
        for(int i=0; i<8; ++i) {
            params_buffer[i].type = type_normal;
            params_buffer[i].value_size = value_size_normal;
            params_buffer[i].default_value = (void*) &default_value_normal;
        }
        params_buffer[0].default_value = NULL;
        params_buffer[8].type = type_ptr;
        params_buffer[8].value_size = value_size_ptr;
        params_buffer[8].default_value = (void*) &default_value_ptr;
        params_buffer[9].type = type_ptr;
        params_buffer[9].value_size = value_size_ptr;
        params_buffer[9].default_value = (void*) &default_value_ptr;

        static ServerCallDescriptor dummy_desc;
        dummy_desc.function_ptr = function_ptr;
        dummy_desc.call_name = call_name;
        dummy_desc.params_amount = params_amount;
        dummy_desc.params = params_buffer;

        return dummy_desc;
    }

    ServerCallDescriptor& generate_wanted_server_desc() {
        //Define the basic "wanted_desc" server call descriptor that all clients look for in the
        //client connexion benchmark
        static KString type_normal("supa_dupa_octa_longer_uint64_t");
        static uint32_t value_size_normal = 8;
        static uint64_t default_value_normal = 0xDEADBEEFBADB002E;

        static KString type_ptr("crazy_silly_nutty_zombie_void*");
        static uint32_t value_size_ptr = sizeof(size_t);
        static size_t default_value_ptr = 0x10000000;

        static void* function_ptr_wanted = NULL;
        static KString call_name_wanted("the_super_mega_long_function_which_even_girls_want");
        static uint32_t params_amount_wanted = 10;

        static ServerParamDescriptor params_buffer[10];
        for(int i=0; i<8; ++i) {
            params_buffer[i].type = type_normal;
            params_buffer[i].value_size = value_size_normal;
            params_buffer[i].default_value = (void*) &default_value_normal;
        }
        params_buffer[0].default_value = NULL;
        params_buffer[8].type = type_ptr;
        params_buffer[8].value_size = value_size_ptr;
        params_buffer[8].default_value = (void*) &default_value_ptr;
        params_buffer[9].type = type_ptr;
        params_buffer[9].value_size = value_size_ptr;
        params_buffer[9].default_value = (void*) &default_value_ptr;

        static ServerCallDescriptor dummy_desc;
        dummy_desc.function_ptr = function_ptr_wanted;
        dummy_desc.call_name = call_name_wanted;
        dummy_desc.params_amount = params_amount_wanted;
        dummy_desc.params = params_buffer;

        return dummy_desc;
    }

    ProcessDescriptor* generate_process_table(const unsigned int server_amount,
                                              const unsigned int running_pids,
                                              const KString& server_name) {
        static ProcessDescriptor* process_table = new ProcessDescriptor[running_pids];
        static unsigned int space_btw_servers = running_pids/server_amount;

        for(unsigned int i = 0; i < running_pids; i+= space_btw_servers) {
            process_table[i].name = server_name;
        }

        return process_table;
    }

    PID find_server(const KString& server_name,
                    const ProcessDescriptor* process_table,
                    const unsigned int server_number,
                    const unsigned int running_pids) {
        unsigned int serv = 0;

        for(unsigned int i = 0; i < running_pids; ++i) {
            if(process_table[i].name != server_name) continue;
            if(serv==server_number) return i+1;
            ++serv;
        }

        return PID_INVALID;
    }

    ServerCallDescriptor* generate_server_descriptor_table(const unsigned int reg_calls_per_server,
                                                           const unsigned int calls_per_server) {
        static ServerCallDescriptor* desc_table = new ServerCallDescriptor[calls_per_server];
        static unsigned int space_btw_real_calls = calls_per_server/reg_calls_per_server;

        for(unsigned int i = 0; i < calls_per_server; ++i) {
            if(i%space_btw_real_calls) {
                desc_table[i] = generate_dummy_server_desc();
            } else {
                desc_table[i] = generate_wanted_server_desc();
            }
        }

        return desc_table;
    }

    ServerCallDescriptor& find_descriptor(const ClientCallDescriptor& request,
                                          ServerCallDescriptor* serv_descs,
                                          const unsigned int descriptor_number,
                                          const unsigned int calls_per_server) {
        static unsigned int call = 0;

        for(unsigned int i = 0; i < calls_per_server; ++i) {
            if(request.compatible_with(serv_descs[i]) == false) continue;
            if(call==descriptor_number) return serv_descs[i];
            ++call;
        }

        return serv_descs[0];
    }

    bool remote_call_client_stub_threaded(RemoteCallConnection& connec,
                                          uint64_t param1,
                                          uint64_t param2,
                                          uint64_t param3,
                                          uint64_t param4,
                                          uint64_t param5,
                                          uint64_t param6,
                                          uint64_t param7,
                                          uint64_t param8,
                                          void* param9) {
        fake_syscall();

        //Allocate a stack
        void* stack = kalloc(1024*1024);

        //Copy integer parameters on the stack
        uint64_t* buff = (uint64_t*) stack;
        buff[0] = param1;
        buff[1] = param2;
        buff[2] = param3;
        buff[3] = param4;
        buff[4] = param5;
        buff[5] = param6;
        buff[6] = param7;
        buff[7] = param8;

        //Share non-default pointer parameters + copy them on the stack
        void** buff2 = (void**) (((size_t) buff) + 8*sizeof(uint64_t));
        if(param9 != default_ptr_value) {
            buff2[0] = kowneradd(param9, PID_KERNEL, 42);
            if(buff2[0] == NULL) {
                kfree(stack);
                return false;
            }
        } else {
            buff2[0] = (void*) 0x10000000;
        }

        //Copy "compatibility stack"
        void* buff3 = (void*) (((size_t) buff2) + sizeof(void*));
        memcpy(buff3, connec.compat_stack, connec.compat_stack_size);

        //Run the remote call
        remote_call_server_stub(connec.server_desc->function_ptr, stack);

        //Clean things up
        if(buff2[0] != (void*) 0x10000000) kfree(buff2[0], 42);
        kfree(stack);

        return true;
    }

    bool remote_call_client_stub_async(AsyncQueue& queue,
                                       RemoteCallConnection& connec,
                                       uint64_t param1,
                                       uint64_t param2,
                                       uint64_t param3,
                                       uint64_t param4,
                                       uint64_t param5,
                                       uint64_t param6,
                                       uint64_t param7,
                                       uint64_t param8,
                                       void* param9) {
        fake_syscall();

        //Find enough space in the queue
        size_t space_required = sizeof(void*)+8*sizeof(uint64_t)+2*sizeof(void*);
        if(queue.find_space(space_required) == false) return false;

        //Copy pointer to the remote call on the stack
        void** buff = (void**) queue.write_pos;
        *buff = connec.server_desc->function_ptr;

        //Copy integer remote call parameters on the stack
        uint64_t* buff2 = (uint64_t*) (((size_t) buff) + sizeof(void*));
        buff2[0] = param1;
        buff2[1] = param2;
        buff2[2] = param3;
        buff2[3] = param4;
        buff2[4] = param5;
        buff2[5] = param6;
        buff2[6] = param7;
        buff2[7] = param8;

        //Share non-default pointer parameters + copy them on the stack
        void** buff3 = (void**) (((size_t) buff2) + 8*sizeof(uint64_t));
        if(param9 != default_ptr_value) {
            buff3[0] = kowneradd(param9, PID_KERNEL, 42);
            if(buff3[0] == NULL) return false;
        } else {
            buff3[0] = (void*) 0x10000000;
        }

        //Copy "compatibility stack"
        if(connec.compat_stack_size) {
            void* buff4 = (void*) (((size_t) buff3) + sizeof(void*));
            memcpy(buff4, connec.compat_stack, connec.compat_stack_size);
        }

        //Update queue information
        if(queue.read_pos == NULL) {
            queue.read_pos = queue.write_pos;
        }
        queue.write_pos = (void*) (((size_t) queue.write_pos) + space_required);

        return true;
    }

    void run_call_from_queue(AsyncQueue& queue, void* stack) {
        fake_context_switch();

        //Extract function pointer from the queue
        if(queue.find_valid_descriptor() == false) return;
        void** buff = (void**) queue.read_pos;
        void* function_ptr = *buff;

        //Copy parameters from the queue to the stack
        uint64_t* params_a_in = (uint64_t*) (((size_t) buff) + sizeof(void*));
        uint64_t* params_a_out = (uint64_t*) stack;
        for(unsigned int i = 0; i < 8; ++i) params_a_out[i] = params_a_in[i];
        void** params_b_in = (void**) (((size_t) params_a_in) + 8*sizeof(uint64_t));
        void** params_b_out = (void**) (((size_t) params_a_out) + 8*sizeof(uint64_t));
        for(unsigned int i = 0; i < 2; ++i) params_b_out[i] = params_b_in[i];

        //Move queue's read_pos to next call, or NULL if there's nothing next
        queue.read_pos = (void**) (((size_t) params_b_in) + 2*sizeof(void*));
        if(queue.read_pos == queue.write_pos) {
            queue.read_pos = NULL;
        }

        //Run the remote call
        remote_call_server_stub(function_ptr, stack);
    }

    void remote_call_server_stub(void* function_ptr, void* stack) {
        void (*remote_call)(uint64_t,
                            uint64_t,
                            uint64_t,
                            uint64_t,
                            uint64_t,
                            uint64_t,
                            uint64_t,
                            uint64_t,
                            void*,
                            void*);
        remote_call = (void (*)(uint64_t,
                                uint64_t,
                                uint64_t,
                                uint64_t,
                                uint64_t,
                                uint64_t,
                                uint64_t,
                                uint64_t,
                                void*,
                                void*)) function_ptr;

        uint64_t* params_a = (uint64_t*) stack;
        void** params_b = (void**) (((size_t) params_a) + 8*sizeof(uint64_t));

        remote_call(params_a[0],
                    params_a[1],
                    params_a[2],
                    params_a[3],
                    params_a[4],
                    params_a[5],
                    params_a[6],
                    params_a[7],
                    params_b[0],
                    params_b[1]);
    }

    void dummy_remote_call(uint64_t param1, uint64_t param2, uint64_t param3, uint64_t param4,
                           uint64_t param5, uint64_t param6, uint64_t param7, uint64_t param8,
                           void* param9, void* param10) {
        if(param1 != 0xDEADBEEFBADB002E) {
            test_failure("Unexpected param1 value");
        }
        if(param2 != 0xDEADBEEFBADB002E) {
            test_failure("Unexpected param2 value");
        }
        if(param3 != 0xDEADBEEFBADB002E) {
            test_failure("Unexpected param3 value");
        }
        if(param4 != 0xDEADBEEFBADB002E) {
            test_failure("Unexpected param4 value");
        }
        if(param5 != 0xDEADBEEFBADB002E) {
            test_failure("Unexpected param5 value");
        }
        if(param6 != 0xDEADBEEFBADB002E) {
            test_failure("Unexpected param6 value");
        }
        if(param7 != 0xDEADBEEFBADB002E) {
            test_failure("Unexpected param7 value");
        }
        if(param8 != 0xDEADBEEFBADB002E) {
            test_failure("Unexpected param8 value");
        }
        if(param9 != (void*) 0x10000000) {
            test_failure("Unexpected param9 value");
        }
        if(param10 != (void*) 0x10000000) {
            test_failure("Unexpected param10 value");
        }
    }
}
