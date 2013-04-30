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

pplx::task<int> get_t(){

    pplx::task<int> t ([]()->int{
        return 5;
    });

    return t;

}

pplx::task<int> get_t3(){
    return pplx_helper::do_async([](pplx_helper::async_helper<int> helper)->int{

        auto sum = 0;
        for(int i = 0; i < 5; ++i){
            sum+= helper.await(get_t());

        }
        return sum;

    });  

}

int main(){
    try{



        auto t4 = pplx_helper::do_async([](pplx_helper::async_helper<int> helper)->int{

            auto sum = 0;
            for(int i = 0; i < 5; ++i){
                sum+= helper.await(get_t3());
                if(i==3)throw std::exception("Exception 2 ");
            }
            return sum;

        });

        std::printf("Called do_async\n");

        std::cout << t4.get();
    }
    catch(std::exception& e){
        std::printf(e.what());
    }
    pplx_helper::detail::EnterExit::check_all_destroyed();
    return 0;



}