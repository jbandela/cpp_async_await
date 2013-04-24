#pragma once
#ifndef ASIO_HELPER_ASIO_HELPER_INCLUDED_04_23_2013
#define ASIO_HELPER_ASIO_HELPER_INCLUDED_04_23_2013

#include <ppltasks.h>
#include <boost/coroutine/all.hpp>
#include <memory>
#include <exception>
#include <utility>
#include <assert.h>
#include <type_traits>

#ifdef ASIO_HELPER_OUTPUT_ENTER_EXIT
#define ASIO_HELPER_ENTER_EXIT ::asio_helper::detail::EnterExit asio_helper_enter_exit_var;
#else
#define ASIO_HELPER_ENTER_EXIT
#endif

namespace asio_helper{
    namespace detail{
#ifdef ASIO_HELPER_OUTPUT_ENTER_EXIT
        // Used for debugging to make sure all functions are exiting correctly
        struct EnterExit{
            EnterExit():n_(++number()){ std::cerr << "==" << n_ << " Entering\n";}
            int n_;
            std::string s_;
            static int& number(){static int number = 0; return number;}
            ~EnterExit(){std::cerr << "==" << n_ << " Exiting\n";}

        };
#endif
        struct coroutine_holder:std::enable_shared_from_this<coroutine_holder>{
            ASIO_HELPER_ENTER_EXIT
                typedef boost::coroutines::coroutine<void*(void*)> co_type;
            std::unique_ptr<co_type> coroutine_;
            co_type::caller_type* coroutine_caller_;
            coroutine_holder():coroutine_(nullptr),coroutine_caller_(nullptr){}

        };

        struct ret_type{
            std::exception_ptr eptr_;
            void* pv_;

            template<class T>
            T& get(){
                if(eptr_){
                    std::rethrow_exception(eptr_);
                }
                return *static_cast<T*>(pv_);
            }
        };

        struct convertible_to_async_helper{};

    }

    template<class T>
    struct async_helper{
        std::shared_ptr<detail::coroutine_holder> co_;
        async_helper(std::shared_ptr<detail::coroutine_holder> c)
            :co_(c)
        {

        }

        async_helper(detail::convertible_to_async_helper);


        template<class R>
        R await(concurrency::task<R> t){
            ASIO_HELPER_ENTER_EXIT
            assert(co_->coroutine_caller_);
            auto c = co_->coroutine_.get();
            auto rettask = t.then([c](concurrency::task<R> t){
                detail::ret_type ret;
                ret.eptr_ = nullptr;
                ret.pv_ = nullptr;
                try{
                    auto v = t.get();
                    ret.pv_ = &v;
                }
                catch(std::exception&){
                    ret.eptr_ = std::current_exception();
                }
                (*c)(&ret);
                return *static_cast<concurrency::task<T>*>(c->get());
            });

            (*co_->coroutine_caller_)(&rettask);
            auto r = static_cast<detail::ret_type*>(co_->coroutine_caller_->get())->get<R>();
            return r;
        }




    };
    namespace detail{
        template<class F>
        struct simple_async_function_holder:public coroutine_holder{

            F f_;
            typedef typename std::result_of<F(convertible_to_async_helper)>::type return_type;
            static void coroutine_function(coroutine_holder::co_type::caller_type& ca){
                ASIO_HELPER_ENTER_EXIT
                auto p = ca.get();
                auto pthis = reinterpret_cast<simple_async_function_holder*>(p);
                pthis->coroutine_caller_ = &ca;
                auto ptr = pthis->shared_from_this();
                try{
                    async_helper<return_type> helper(ptr);
                    auto ret = pthis->f_(helper);
                    concurrency::task<return_type> rettask([ptr,ret]{
                        return ret;   
                    });
                    ptr.reset();
                    ca(&rettask);
                }
                catch(std::exception&){
                    auto eptr = std::current_exception();
                    concurrency::task<return_type> rettask([ptr,eptr]{
                        concurrency::task<return_type> ret;
                        std::rethrow_exception(eptr);
                        // Never reached
                        return ret;
                    });
                    ptr.reset();
                    ca(&rettask);

                }
            }
            simple_async_function_holder(F f):f_(f){}

            concurrency::task<return_type> run(){
                coroutine_.reset(new coroutine_holder::co_type(&coroutine_function,this));
                return *static_cast<concurrency::task<return_type>*>(coroutine_->get());
            }
        };
    }

    template<class R,class F>
    auto do_async(F f)->concurrency::task<R>{
        auto ret = std::make_shared<detail::simple_async_function_holder<F>>(f);
        return ret->run();
    }
}

#undef ASIO_HELPER_ENTER_EXIT
#endif