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
        return 5+i;

    });
    


    std::cout << t3.get();


}