#include <ppltasks.h>
#include <iostream>
#include <string>
#include "ppl_helper.hpp"

int main(){

    concurrency::task<int> t ([]()->int{
        return 5;
    });

    auto t3 = asio_helper::do_async([t](asio_helper::async_helper helper)->std::string{
        return std::string("hui");

    });
    


    std::cout << t3.get();


}