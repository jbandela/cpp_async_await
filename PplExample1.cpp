// Copyright (c) 2013 John R. Bandela
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <ppltasks.h>
#include <iostream>
#include <string>
#include "ppl_helper.hpp"

int main(){

    concurrency::task<int> t ([]()->int{
        return 5;
    });

    auto t3 = ppl_helper::do_async([t](ppl_helper::async_helper<int> helper)->int{

        auto sum = 0;
        for(int i = 0; i < 5; ++i){
            sum+= helper.await(t);

        }
        return sum;

    });   
    auto t4 = ppl_helper::do_async([t,t3](ppl_helper::async_helper<int> helper)->int{

        auto sum = 0;
        for(int i = 0; i < 5; ++i){
            sum+= helper.await(t3);
        }
        return sum;

    });

    std::cerr << "Called do_async\n";

    try{
        std::cout << t4.get();
    }
    catch(std::exception& e){
        std::cerr << e.what();
    }
    return 0;


}