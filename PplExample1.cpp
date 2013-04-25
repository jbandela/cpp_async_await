#define ASIO_HELPER_OUTPUT_ENTER_EXIT

#include <ppltasks.h>
#include <iostream>
#include <string>
#include "ppl_helper.hpp"

int main(){

    concurrency::task<int> t ([]()->int{
        return 5;
    });

    auto t3 = ppl_helper::do_async<int>([t](ppl_helper::async_helper<int> helper)->int{

        auto sum = 0;
        for(int i = 0; i < 5; ++i){
            sum+= helper.await(t);
            std::cerr << "Iteration " << i << "\n";
            if(i==3)throw std::exception("Exception");

        }
        return sum;

    });
        auto t4 = ppl_helper::do_async<void>([t](ppl_helper::async_helper<void> helper){

        auto sum = 0;
        for(int i = 0; i < 5; ++i){
            sum+= helper.await(t);
            std::cerr << "Void Iteration " << i << "\n";

        }
        //return sum;

    });
    std::cerr << "Called do_async\n";

    try{
        t4.get();
        std::cout << t3.get();
    }
    catch(std::exception& e){
        std::cerr << e.what();
    }
    return 0;
    int i;
    std::cin >> i;

}