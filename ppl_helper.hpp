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
#include <concurrent_queue.h>
#include <functional>

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

            static void add_to_global(std::shared_ptr<coroutine_holder> ptr){
                static concurrency::concurrent_queue<std::shared_ptr<coroutine_holder>> q_;
                q_.push(ptr);

            }

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
        //typedef std::shared_ptr<detail::coroutine_holder>
        typedef detail::coroutine_holder* co_ptr;
        typedef std::function<concurrency::task<T>> func_type;
        co_ptr co_;
        async_helper(co_ptr c)
            :co_(c)
        {

        }

        async_helper(detail::convertible_to_async_helper);


        template<class R>
        R await(concurrency::task<R> t){
            ASIO_HELPER_ENTER_EXIT
            assert(co_->coroutine_caller_);
            auto co = co_;
           auto rettask = t.then([co](concurrency::task<R> t){
                detail::ret_type ret;
                ret.eptr_ = nullptr;
                ret.pv_ = nullptr;
                ret.pv_ = &t;
                (*co->coroutine_)(&ret);
                auto r = *static_cast<concurrency::task<T>*>(co->coroutine_->get());
                return r;
            });

            (*co_->coroutine_caller_)(&rettask);
            auto r = static_cast<detail::ret_type*>(co_->coroutine_caller_->get())->get<concurrency::task<R>>().get();
            return r;
        }




    };
    namespace detail{
        template<class F>
        struct simple_async_function_holder:public coroutine_holder{

            F f_;
            typedef typename std::result_of<F(convertible_to_async_helper)>::type return_type;
                    typedef std::function<concurrency::task<T>> func_type;

            static void coroutine_function(coroutine_holder::co_type::caller_type& ca){
                ASIO_HELPER_ENTER_EXIT;
                // Need to call back to run so that coroutine_ gets set
                ca(nullptr);

                auto p = ca.get();
                auto pthis = reinterpret_cast<simple_async_function_holder*>(p);
                pthis->coroutine_caller_ = &ca;
                try{
                    ASIO_HELPER_ENTER_EXIT;
                    async_helper<return_type> helper(pthis);
                    auto ret = pthis->f_(helper);
                    concurrency::task<return_type> rettask([pthis,ret](){
                        return ret;   
                    });
                    ca(&rettask);
                }
                catch(std::exception&){
                    auto eptr = std::current_exception();
                    concurrency::task<return_type> rettask([pthis,eptr](){
                    delete pthis;
                       concurrency::task<return_type> ret;
                        std::rethrow_exception(eptr);
                        return ret;
                    });
                    ca(&rettask);
               }
            }
            simple_async_function_holder(F f):f_(f){}

            concurrency::task<return_type> run(){
                coroutine_.reset(new coroutine_holder::co_type(&coroutine_function,nullptr));
                // This makes sure coroutine_ gets set;
                (*coroutine_)(this);
                return *static_cast<concurrency::task<return_type>*>(coroutine_->get());
                
            }
        };
    }

    template<class R,class F>
    auto do_async(F f)->concurrency::task<R>{
        //auto ret = std::make_shared<detail::simple_async_function_holder<F>>(f);
        //detail::coroutine_holder::add_to_global(ret);
        auto ret = new detail::simple_async_function_holder<F>(f);
        return ret->run();
    }
}

#undef ASIO_HELPER_ENTER_EXIT
#endif