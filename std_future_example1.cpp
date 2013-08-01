//#define PPL_HELPER_OUTPUT_ENTER_EXIT
#include <iostream>
#include <string>
#include "std_future_helper.hpp"

int main(){

	std::shared_future<int> t(std::async([]()->int{
		return 5;
	}));

	auto t3 = future_helper::do_async([t](future_helper::async_helper<int> helper)->int{

		auto sum = 0;
		for (int i = 0; i < 5; ++i){
			sum += helper.await(t);

		}
		return sum;

	});
	auto t4 = future_helper::do_async([t, t3](future_helper::async_helper<int> helper)->int{

		auto sum = 0;
		for (int i = 0; i < 5; ++i){
			sum += helper.await(t3);
		}
		return sum;

	});

	std::cerr << "Called do_async\n";

	try{
		std::cout << t4.get();
	}
	catch (std::exception& e){
		std::cerr << e.what();
	}
	return 0;


}