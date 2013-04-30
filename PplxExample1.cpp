// Copyright (c) 2013 John R. Bandela
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#define PPL_HELPER_OUTPUT_ENTER_EXIT
#include <pplxtasks.h>
#include <iostream>
#include <string>
#include "pplx_helper.hpp"
#include <cstdio>

#pragma comment(lib,"casablanca110.lib")

int main(){
    try{

    pplx::task<int> t ([]()->int{
        return 5;
    });

    auto t3 = pplx_helper::do_async([t](pplx_helper::async_helper<int> helper)->int{

        auto sum = 0;
        for(int i = 0; i < 5; ++i){
            sum+= helper.await(t);

        }
        return sum;

    });   
    auto t4 = pplx_helper::do_async([t,t3](pplx_helper::async_helper<int> helper)->int{

        auto sum = 0;
        for(int i = 0; i < 5; ++i){
            sum+= helper.await(t3);
        }
        return sum;

    });

    std::printf("Called do_async\n");

       //(t && t3 && t4).wait();
        std::cout << t4.get();
    }
    catch(std::exception& e){
        std::printf(e.what());
    }
    return 0;


}