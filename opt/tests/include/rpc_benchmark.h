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

namespace Tests {
    void benchmark_rpc(); //RPC performance benchmark
    
    //Individual tests
    void rpc_server_init_bench(); //Used to evaluate the time taken by the initialization of many
                                  //"heavy" servers
    
    //Support structures and globals
    extern KString type_normal;
    extern uint32_t value_size_normal;
    extern uint64_t default_value_normal;
    
    extern KString type_ptr;
    extern uint32_t value_size_ptr;
    extern addr_t default_value_ptr;
    
    struct ParamDescriptor {
        KString type;
        uint32_t value_size;
        void* default_value;
        ParamDescriptor() : type(type_normal),
                            value_size(value_size_normal),
                            default_value((void*) &default_value_normal) {}
    };

    extern void* function_pointer_normal;
    extern KString name_normal;
    extern uint32_t params_amount_normal;
    extern ParamDescriptor* params_normal;
    
    struct ServerCallDescriptor {
        void* function_pointer;
        KString name;
        uint32_t params_amount;
        ParamDescriptor* params;
        ServerCallDescriptor() : function_pointer(function_pointer_normal),
                                 name(name_normal),
                                 params_amount(params_amount_normal),
                                 params(params_normal) {}
    };
}

#endif
