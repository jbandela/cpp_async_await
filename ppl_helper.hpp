#pragma once
#ifndef ASYNC_HELPER_PPL_HELPER_INCLUDED_04_24_2013
#define ASYNC_HELPER_PPL_HELPER_INCLUDED_04_24_2013

#include <ppltasks.h>
#include <boost/coroutine/all.hpp>
#include <memory>
#include <exception>
#include <utility>
#include <assert.h>
#include <type_traits>
#include <functional>

#ifdef PPL_HELPER_OUTPUT_ENTER_EXIT
#define PPL_HELPER_OUTPUT_ENTER_EXIT ::asio_helper::detail::EnterExit asio_helper_enter_exit_var;
#else
#define PPL_HELPER_OUTPUT_ENTER_EXIT
#endif

namespace ppl_helper{
    namespace detail{
#ifdef PPL_HELPER_OUTPUT_ENTER_EXIT
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
            PPL_HELPER_OUTPUT_ENTER_EXIT
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
    class async_helper{
        typedef detail::coroutine_holder* co_ptr;
        typedef std::function<concurrency::task<T>()> func_type;
        co_ptr co_;
    public:
        async_helper(co_ptr c)
            :co_(c)
        {

        }

        async_helper(detail::convertible_to_async_helper);


        template<class R>
        R await(concurrency::task<R> t){
            PPL_HELPER_OUTPUT_ENTER_EXIT
                assert(co_->coroutine_caller_);
            auto co = co_;
            func_type retfunc([co,t](){
                return t.then([co](concurrency::task<R> et)->concurrency::task<T>{
                    detail::ret_type ret;
                    ret.eptr_ = nullptr;
                    ret.pv_ = nullptr;
                    ret.pv_ = &et;
                    (*co->coroutine_)(&ret);
                    auto f = *static_cast<func_type*>(co->coroutine_->get());
                    return f();
                });
            });

            (*co_->coroutine_caller_)(&retfunc);
            return static_cast<detail::ret_type*>(co_->coroutine_caller_->get())->get<concurrency::task<R>>().get();
        }

    };
    namespace detail{
        template<class F>
        class simple_async_function_holder:public coroutine_holder{


            F f_;
            typedef typename std::result_of<F(convertible_to_async_helper)>::type return_type;
            typedef std::function<concurrency::task<return_type>()> func_type;

            template<class T>
            struct ret_holder{
                T value_;
                template<class F>
                ret_holder(F& f,async_helper<return_type> h):value_(f(h)){}
                const T& get()const{return value_;}
            };
            template<>
            struct ret_holder<void>{
                template<class F>
                ret_holder(F& f,async_helper<return_type> h){
                    f(h);
                }
                void get()const{}
            };   
            static void coroutine_function(coroutine_holder::co_type::caller_type& ca){
                PPL_HELPER_OUTPUT_ENTER_EXIT;
                // Need to call back to run so that coroutine_ gets set
                ca(nullptr);

                auto p = ca.get();
                auto pthis = reinterpret_cast<simple_async_function_holder*>(p);
                auto sptr = pthis->shared_from_this();
                pthis->coroutine_caller_ = &ca;
                try{
                    PPL_HELPER_OUTPUT_ENTER_EXIT;
                    async_helper<return_type> helper(pthis);
                    ret_holder<return_type> ret(pthis->f_,helper);
                    func_type retfunc([&sptr,ret](){
                        sptr.reset();
                        return concurrency::task<return_type>([ret](){return ret.get();});   
                    });
                    ca(&retfunc);
                }
                catch(std::exception&){
                    auto eptr = std::current_exception();
                    func_type retfunc([&sptr,eptr](){
                        sptr.reset();
                        concurrency::task<return_type> ret;
                        std::rethrow_exception(eptr);
                        return ret;
                    });
                    ca(&retfunc);
                }
            }
        public:
            simple_async_function_holder(F f):f_(f){}

            concurrency::task<return_type> run(){
                coroutine_.reset(new coroutine_holder::co_type(&coroutine_function,nullptr));
                // This makes sure coroutine_ gets set;
                (*coroutine_)(this);
                auto f = *static_cast<func_type*>(coroutine_->get());
                return f();
            }
        };
    }

    template<class R,class F>
    auto do_async(F f)->concurrency::task<R>{
        auto ret = std::make_shared<detail::simple_async_function_holder<F>>(f);
        return ret->run();
    }
}

#undef PPL_HELPER_OUTPUT_ENTER_EXIT
#endif