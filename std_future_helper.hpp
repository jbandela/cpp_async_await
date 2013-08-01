// Copyright (c) 2013 John R. Bandela
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#ifndef ASYNC_HELPER_PPL_HELPER_IMP_INCLUDED_04_24_2013
#define ASYNC_HELPER_PPL_HELPER_IMP_INCLUDED_04_24_2013

#include <boost/coroutine/all.hpp>
#include <memory>
#include <exception>
#include <utility>
#include <assert.h>
#include <type_traits>
#include <functional>
#include <future>

#ifdef PPL_HELPER_OUTPUT_ENTER_EXIT
#include <cstdio>
#include <atomic>
#define PPL_HELPER_ENTER_EXIT ::future_helper::detail::EnterExit ppl_helper_enter_exit_var;
#else
#define PPL_HELPER_ENTER_EXIT
#endif

namespace future_helper{


	namespace detail{
		template<typename F, typename W, typename R>
		struct helper
		{
			F f_;
			W w_;

			helper(F f, W w)
				: f_(f)
				, w_(w)
			{
			}

			helper(const helper& other)
				: f_(other.f_)
				, w_(other.w_)
			{
			}

			helper(helper && other)
				: f_(std::move(other.f_))
				, w_(std::move(other.w_))
			{
			}

			helper& operator=(helper other)
			{

				f_ = std::move(other.f_);
				w_ = std::move(other.w_);
				return *this;
			}

			R operator()()
			{
				f_.wait();
				return w_(f_);
			}
		};

	}

	template<typename F, typename W>
	auto then(F f, W w) -> std::shared_future<decltype(w(f))>
	{
		std::shared_future < decltype(w(f))> ret = std::async(std::launch::async, detail::helper<F, W, decltype(w(f))>(f, w));
		return ret;
	}
	namespace detail{
#ifdef PPL_HELPER_OUTPUT_ENTER_EXIT
		// Used for debugging to make sure all functions are exiting correctly
		struct EnterExit{
			EnterExit() : n_(++number()){ increment(); std::printf("==%d Entering\n", n_); }
			int n_;
			std::string s_;
			static int& number(){ static int number = 0; return number; }
			static std::atomic<int>& counter(){ static std::atomic<int> counter_ = 0; return counter_; }
			static void increment(){ ++counter(); }
			static void decrement(){ --counter(); }
			static void check_all_destroyed(){ assert(counter() == 0); if (counter())throw std::exception("Not all EnterExit destroyed"); };
			~EnterExit(){ decrement(); std::printf("==%d Exiting\n", n_); }

		};
#endif
#undef PPL_HELPER_OUTPUT_ENTER_EXIT


		struct coroutine_holder : std::enable_shared_from_this<coroutine_holder>{
			PPL_HELPER_ENTER_EXIT;
			typedef boost::coroutines::coroutine<void*(void*) > co_type;
			std::unique_ptr<co_type> coroutine_;
			co_type::caller_type* coroutine_caller_;
			coroutine_holder() : coroutine_(), coroutine_caller_(nullptr){}

		};

		struct ret_type{
			std::exception_ptr eptr_;
			void* pv_;

			template<class T>
			T& get(){
				if (eptr_){
					std::rethrow_exception(eptr_);
				}
				return *static_cast<T*>(pv_);
			}
		};

		struct convertible_to_async_helper{};


	}

	template<class T>
	class async_helper{
		typedef detail::coroutine_holder* co_ptr;
		typedef std::function<std::shared_future<T>()> func_type;
		co_ptr co_;
	public:
		async_helper(co_ptr c)
			: co_(c)
		{

		}

		async_helper(detail::convertible_to_async_helper);


		template<class R>
		R await(std::shared_future<R> t){
			PPL_HELPER_ENTER_EXIT;
			assert(co_->coroutine_caller_);
			auto co = co_;
			func_type retfunc([co, t](){
				auto sptr = co->shared_from_this();
				return then(t,[sptr](std::shared_future<R> et)->T{
					detail::ret_type ret;
					ret.eptr_ = nullptr;
					ret.pv_ = nullptr;
					ret.pv_ = &et;
					(*sptr->coroutine_)(&ret);
					try{
						func_type f(std::move(*static_cast<func_type*>(sptr->coroutine_->get())));
						return f().get();
					}
					catch (std::exception&){
						ret.eptr_ = std::current_exception();
						ret.pv_ = nullptr;
						(*sptr->coroutine_)(&ret);
						throw;
					}
				});
			});

			(*co_->coroutine_caller_)(&retfunc);
			return static_cast<detail::ret_type*>(co_->coroutine_caller_->get())->get<std::shared_future<R>>().get();
		}

	};
	namespace detail{

		template<class T>
		struct ret_holder{
			T value_;
			template<class FT>
			ret_holder(FT& f, async_helper<T> h) : value_(f(h)){}
			const T& get()const{ return value_; }
		};
		template<>
		struct ret_holder<void>{
			template<class FT>
			ret_holder(FT& f, async_helper<void> h){
				f(h);
			}
			void get()const{}
		};

		template<class F>
		class simple_async_function_holder : public coroutine_holder{


			F f_;
			typedef typename std::result_of<F(convertible_to_async_helper)>::type return_type;
			typedef typename std::shared_future<return_type> future_t;
			typedef std::function<future_t()> func_type;


			static void coroutine_function(coroutine_holder::co_type::caller_type& ca){
				PPL_HELPER_ENTER_EXIT;;

				auto p = ca.get();
				auto pthis = reinterpret_cast<simple_async_function_holder*>(p);
				pthis->coroutine_caller_ = &ca;
				try{
					PPL_HELPER_ENTER_EXIT;
					async_helper<return_type> helper(pthis);
					ret_holder<return_type> ret(pthis->f_, helper);
					func_type retfunc([ret](){
						return future_t(std::async([ret](){return ret.get(); }));
					});
					ca(&retfunc);
				}
				catch (std::exception&){
					auto eptr = std::current_exception();
					func_type retfunc([eptr](){
						future_t ret;
						std::rethrow_exception(eptr);
						return ret;
					});
					ca(&retfunc);
				}
			}
		public:
			simple_async_function_holder(F f) : f_(f){}

			std::shared_future<return_type> run(){
				coroutine_.reset(new coroutine_holder::co_type(&coroutine_function, this));
				func_type f(std::move(*static_cast<func_type*>(coroutine_->get())));
				return f();

			}
		};

		template<class F>
		struct return_helper{};

		template<class R>
		struct return_helper<R(async_helper<R>)>{
			typedef R type;
		};
	}

	template<class F>
	typename std::shared_future<typename std::result_of<F(detail::convertible_to_async_helper)>::type> do_async(F f){
		auto ret = std::make_shared<detail::simple_async_function_holder<F>>(f);
		return ret->run();
	}
}


#endif