#pragma once
#ifndef ASIO_HELPER_ASIO_HELPER_INCLUDED_04_23_2013
#define ASIO_HELPER_ASIO_HELPER_INCLUDED_04_23_2013

#include <boost/asio.hpp>
#include <boost/coroutine/all.hpp>
#include <memory>
#include <exception>
#include <utility>
#include <assert.h>

#ifdef ASIO_HELPER_OUTPUT_ENTER_EXIT
#define ASIO_HELPER_ENTER_EXIT ::asio_helper::detail::EnterExit asio_helper_enter_exit_var;
#include <iostream>
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
                        boost::asio::io_service& io_;
            boost::asio::strand strand_;

            coroutine_holder(boost::asio::io_service& io):coroutine_(nullptr),coroutine_caller_(nullptr),
            io_(io),strand_(io){}

        };

        struct ret_type{
            std::exception_ptr eptr_;
            void* pv_;
        };

        template<class F>
        class callback{

            F f_;
            std::shared_ptr<coroutine_holder> co_;
        public:
            callback(std::shared_ptr<coroutine_holder> c,F f):f_(f),co_(c){}

            void operator()(){
                ASIO_HELPER_ENTER_EXIT
                ret_type r;
                try{
                    auto ret = f_();
                    r.eptr_ = nullptr;
                    r.pv_ = &ret;
                }
                catch(...){
                    r.eptr_ = std::current_exception();
                    r.pv_ = nullptr;
                }

                (*co_->coroutine_)(&r);
            }

            template<class T0>
            void operator()(T0&& t0){
                ASIO_HELPER_ENTER_EXIT
                ret_type r;
                try{
                    auto ret = f_(std::forward<T0>(t0));
                    r.eptr_ = nullptr;
                    r.pv_ = &ret;
                }
                catch(...){
                    r.eptr_ = std::current_exception();
                    r.pv_ = nullptr;
                }
                (*co_->coroutine_)(&r);
            }    

            template<class T0,class T1>
            void operator()(T0&& t0,T1&& t1){
                ASIO_HELPER_ENTER_EXIT 
                ret_type r;
                try{
                    auto ret = f_(std::forward<T0>(t0),std::forward<T1>(t1));
                    r.eptr_ = nullptr;
                    r.pv_ = &ret;
                }
                catch(...){
                    r.eptr_ = std::current_exception();
                    r.pv_ = nullptr;
                }

                (*co_->coroutine_)(&r);
            }  

            template<class T0,class T1,class T2>
            void operator()(T0&& t0,T1&& t1,T2&& t2){
                ASIO_HELPER_ENTER_EXIT
                ret_type r;
                try{
                    auto ret = f_(std::forward<T0>(t0),std::forward<T1>(t1),std::forward<T2>(t2));
                    r.eptr_ = nullptr;
                    r.pv_ = &ret;
                }
                catch(...){
                    r.eptr_ = std::current_exception();
                    r.pv_ = nullptr;
                }
                (*co_->coroutine_)(&r);
            }

            template<class T0,class T1,class T2,class T3>
            void operator()(T0&& t0,T1&& t1,T2&& t2,T3&& t3){
                ASIO_HELPER_ENTER_EXIT
                ret_type r;
                try{
                    auto ret = f_(std::forward<T0>(t0),std::forward<T1>(t1),std::forward<T2>(t2),std::forward<T3>(t3));
                    r.eptr_ = nullptr;
                    r.pv_ = &ret;
                }
                catch(...){
                    r.eptr_ = std::current_exception();
                    r.pv_ = nullptr;
                }
                (*co_->coroutine_)(&r);
            }

            template<class T0,class T1,class T2,class T3,class T4>
            void operator()(T0&& t0,T1&& t1,T2&& t2,T3&& t3,T4&& t4){
                ASIO_HELPER_ENTER_EXIT
                ret_type r;
                try{
                    auto ret = f_(std::forward<T0>(t0),std::forward<T1>(t1),std::forward<T2>(t2),std::forward<T3>(t3)
                        ,std::forward<T4>(t4));
                    r.eptr_ = nullptr;
                    r.pv_ = &ret;
                }
                catch(...){
                    r.eptr_ = std::current_exception();
                    r.pv_ = nullptr;
                }
                (*co_->coroutine_)(&r);
            }

            template<class T0,class T1,class T2,class T3,class T4,class T5>
            void operator()(T0&& t0,T1&& t1,T2&& t2,T3&& t3,T4&& t4,T5&& t5){
                ASIO_HELPER_ENTER_EXIT
                ret_type r;
                try{
                    auto ret = f_(std::forward<T0>(t0),std::forward<T1>(t1),std::forward<T2>(t2),std::forward<T3>(t3)
                        ,std::forward<T4>(t4),std::forward<T5>(t5));
                    r.eptr_ = nullptr;
                    r.pv_ = &ret;
                }
                catch(...){
                    r.eptr_ = std::current_exception();
                    r.pv_ = nullptr;
                }
                (*co_->coroutine_)(&r);
            }

        };
    }

    namespace handlers{
        struct read_handler
        {
            typedef  std::pair<boost::system::error_code,std::size_t> return_type;
            std::pair<boost::system::error_code,std::size_t> operator()(
                const boost::system::error_code& ec,
                std::size_t bytes_transferred)
            {
                return std::make_pair(ec,bytes_transferred);
            }
        };       

        typedef read_handler write_handler;

        struct completion_handler
        {
            typedef int return_type;

            int operator()()
            {
                return 0;
            }
        };

        struct accept_handler
        {
            typedef boost::system::error_code return_type;
            boost::system::error_code operator()(
                const boost::system::error_code& ec)
            {
                return ec;
            }
        };

        typedef accept_handler connect_handler;
        typedef accept_handler sslhandshake_handler;
        typedef accept_handler sslshutdown_handler;
        typedef accept_handler wait_handler;

        struct composedconnect_handler
        {
            typedef std::pair<boost::system::error_code,boost::asio::ip::tcp::resolver::iterator> return_type;
            std::pair<boost::system::error_code,boost::asio::ip::tcp::resolver::iterator> operator()(
                const boost::system::error_code& ec,
                boost::asio::ip::tcp::resolver::iterator iterator)
            {
                return std::make_pair(ec,iterator);
            }
        };

        typedef composedconnect_handler resolve_handler;

        struct signal_handler
        {
            typedef std::pair<boost::system::error_code,int> return_type;
            std::pair<boost::system::error_code,int> operator()(
                const boost::system::error_code& ec,
                int signal_number)
            {
                return std::make_pair(ec,signal_number);
            }
        };
    }

    class async_helper{
        std::shared_ptr<detail::coroutine_holder> co_;

        template<class R>
        R await(){
            ASIO_HELPER_ENTER_EXIT
            assert(co_->coroutine_caller_);
            (*co_->coroutine_caller_)(nullptr);
            auto ret = reinterpret_cast<detail::ret_type*>(co_->coroutine_caller_->get());
            if(ret->eptr_){
                std::rethrow_exception(ret->eptr_);
            }
            auto r = std::move( *static_cast<R*>(ret->pv_));
            return r;
        }

    public:
        async_helper(std::shared_ptr<detail::coroutine_holder> c)
            :co_(c)
        {

        }


        template<class F>
        typename F::return_type await(boost::asio::detail::wrapped_handler<boost::asio::strand, detail::callback<F>>&){
            return await<typename F::return_type>();

        }

        template<class F>
       boost::asio::detail::wrapped_handler<boost::asio::strand, detail::callback<F>> make_callback(F f){
            detail::callback<F> ret(co_,f);
            return co_->strand_.wrap(ret);
        }

    };
    namespace detail{
        template<class F>
        struct simple_async_function_holder:public coroutine_holder{

            F f_;
            static void coroutine_function(coroutine_holder::co_type::caller_type& ca){
                ASIO_HELPER_ENTER_EXIT
                auto p = ca.get();
                auto pthis = reinterpret_cast<simple_async_function_holder*>(p);
                pthis->coroutine_caller_ = &ca;
                auto ptr = pthis->shared_from_this();
                try{
                    async_helper helper(ptr);
                    pthis->f_(helper);
                }
                catch(std::exception&){
                    auto eptr = std::current_exception();
                    pthis->io_.post(pthis->strand_.wrap([eptr](){
                        ASIO_HELPER_ENTER_EXIT 
                        std::rethrow_exception(eptr);}));
                }
            }
            simple_async_function_holder(boost::asio::io_service& io,F f):coroutine_holder(io),f_(f){}

            void run(){
                coroutine_.reset(new coroutine_holder::co_type(&coroutine_function,this));
            }
        };
    }

    template<class F>
    void do_async(boost::asio::io_service& io,F f){
        auto ret = std::make_shared<detail::simple_async_function_holder<F>>(io,f);
        io.post(ret->strand_.wrap([ret](){ret->run();}));
    }
}

#undef ASIO_HELPER_ENTER_EXIT
#endif