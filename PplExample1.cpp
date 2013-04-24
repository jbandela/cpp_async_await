#define ASIO_HELPER_OUTPUT_ENTER_EXIT

#include <ppltasks.h>
#include <iostream>
#include <string>
#include "ppl_helper.hpp"

int main(){

    concurrency::task<int> t ([]()->int{
        return 5;
    });

    auto t3 = asio_helper::do_async<int>([t](asio_helper::async_helper<int> helper)->int{
        auto i = helper.await(t);
        throw std::exception("Exception");
        return 5+i;

    });
    

    try{
    std::cout << t3.get();
    }
    catch(std::exception& e){
        std::cerr << e.what();
    }

    int i;
    std::cin >> i;

}