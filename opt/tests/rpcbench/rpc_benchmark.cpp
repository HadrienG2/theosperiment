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
 
#include <fake_syscall.h>
#include <kmem_allocator.h>
#include <kstring.h>
#include <new.h>
#include <rpc_benchmark.h>
#include <test_display.h>

namespace Tests {
    void benchmark_rpc() {
        test_beginning("RPC performance");

        reset_title();
        test_title("Server startup impact");
        rpc_server_init_bench();
        
        test_title("Client startup impact");
        fail_notimpl();
    }
    
    const char* type_normal = "supa_dupa_octa_lengthy_uint64_t";
    uint32_t value_size_normal = 8;
    uint64_t default_value_normal = 0xDEADBEEFBADB002E;
    
    const char* type_ptr = "crazy_silly_nutty_zombie_void*";
    uint32_t value_size_ptr = sizeof(size_t);
    size_t default_value_ptr = 0x10000000;
    
    void* function_pointer_normal = NULL;
    const char* name_normal = "InsanelyLongReturnType insanely_long_function_name";
    uint32_t params_amount_normal = 10;
    ParamDescriptor* params_normal = NULL;
    
    void rpc_server_init_bench() {
        ParamDescriptor params_buffer[10];
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
        params_normal = params_buffer;
        
        ServerCallDescriptor dummy_desc;
        dummy_desc.function_pointer = function_pointer_normal;
        dummy_desc.name = name_normal;
        dummy_desc.params_amount = params_amount_normal;
        dummy_desc.params = params_normal;
        
        const unsigned int NUMBER_OF_SERVERS = 50;
        const unsigned int NUMBER_OF_CALLS = 30;
        
        bench_start();
        
        for(unsigned int i = 0; i < NUMBER_OF_SERVERS; ++i) {
            //Emulate system call overhead
            fake_syscall();
            
            //First, fully parse the server's management structures once to determine how much
            //memory must be allocated as a whole. Use "fake new" to measure the overhead of array
            //allocation.
            start_faking_allocation();
                ServerCallDescriptor* not_really_allocd = new ServerCallDescriptor[NUMBER_OF_CALLS];
                size_t to_be_allocd = would_be_allocd;
                not_really_allocd = NULL;
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
        }
        
        bench_stop();
    }
    
    size_t ParamDescriptor::heap_size() {
        size_t to_be_allocd = type.heap_size();
        if(default_value != NULL) {
            to_be_allocd+= value_size;
        }
        return to_be_allocd;
    }
    
    ParamDescriptor& ParamDescriptor::operator=(ParamDescriptor& source) {
        if(&source == this) return *this;
        
        type = source.type;
        value_size = source.value_size;
        if(source.default_value == NULL) {
            default_value = NULL;
        } else {
            default_value = kalloc(source.value_size);
            memcpy(default_value, source.default_value, source.value_size);
        }
        
        return *this;
    }
    
    size_t ServerCallDescriptor::heap_size() {
        //Use "fake new" to measure the overhead of array allocation
        size_t to_be_allocd = name.heap_size();
        
        start_faking_allocation();
            ParamDescriptor* not_really_allocd = new ParamDescriptor[params_amount];
            to_be_allocd+= would_be_allocd;
            not_really_allocd = NULL;
        stop_faking_allocation();
        
        for(uint32_t i = 0; i < params_amount; ++i) {
            to_be_allocd+= params[i].heap_size();
        }
        
        return to_be_allocd;
    }
    
    ServerCallDescriptor& ServerCallDescriptor::operator=(ServerCallDescriptor& source) {
        if(&source == this) return *this;
        
        function_pointer = source.function_pointer;
        name = source.name;
        params_amount = source.params_amount;
        params = new ParamDescriptor[source.params_amount];
        for(uint32_t i = 0; i < source.params_amount; ++i) {
            params[i] = source.params[i];
        }
        
        return *this;
    }
}
