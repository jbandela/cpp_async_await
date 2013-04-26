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
            ASIO_HELPER_ENTER_EXIT;
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
            callback(coroutine_holder* c,F f):f_(f),co_(c->shared_from_this()){}

            void operator()(){
                ASIO_HELPER_ENTER_EXIT;
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
                ASIO_HELPER_ENTER_EXIT;
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
                ASIO_HELPER_ENTER_EXIT;
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
                ASIO_HELPER_ENTER_EXIT;
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
                ASIO_HELPER_ENTER_EXIT;
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
                ASIO_HELPER_ENTER_EXIT;
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

        };


    }

    namespace handlers{

        namespace detail{
            struct nill_t{};

        }


        template<class T0 = detail::nill_t, class T1 = detail::nill_t, class T2 = detail::nill_t,
        class T3 = detail::nill_t, class T4 = detail::nill_t,class T5 = detail::nill_t>
        struct handler{};

        template<>
        struct handler<>{
            typedef boost::asio::detail::wrapped_handler<boost::asio::strand, asio_helper::detail::callback<handler>> 
                callback_type;
            typedef int return_type;

            return_type operator ()(){
                return 0;
            }
        };          
        template<class T0>
        struct handler<T0>{
            typedef T0 return_type;
            typedef boost::asio::detail::wrapped_handler<boost::asio::strand, asio_helper::detail::callback<handler>> 
                callback_type;
            return_type operator ()(T0 t0){
                return t0;
            }
        };   
        template<class T0,class T1>
        struct handler<T0,T1>{
            typedef std::pair<T0,T1> return_type;
            typedef boost::asio::detail::wrapped_handler<boost::asio::strand, asio_helper::detail::callback<handler>> 
                callback_type;
            return_type operator ()(T0 t0,T1 t1){
                return std::make_pair(std::move(t0),std::move(t1));
            }
        };
        template<class T0,class T1,class T2>
        struct handler<T0,T1,T2>{
            typedef std::tuple<T0,T1,T2> return_type;
            typedef boost::asio::detail::wrapped_handler<boost::asio::strand, asio_helper::detail::callback<handler>> 
                callback_type;
            return_type operator ()(T0 t0,T1 t1,T2 t2){
                return std::make_tuple(std::move(t0),std::move(t1),std::move(t2));
            }
        };

        template<class T0,class T1,class T2,class T3>
        struct handler<T0,T1,T2,T3>{
            typedef std::tuple<T0,T1,T2,T3> return_type;
            typedef boost::asio::detail::wrapped_handler<boost::asio::strand, asio_helper::detail::callback<handler>> 
                callback_type;
            return_type operator ()(T0 t0,T1 t1,T2 t2,T3 t3){
                return std::make_tuple(std::move(t0),std::move(t1),std::move(t2),std::move(t3));
            }
        };

        template<class T0,class T1,class T2,class T3,class T4>
        struct handler<T0,T1,T2,T3,T4>{
            typedef std::tuple<T0,T1,T2,T3,T4> return_type;
            typedef boost::asio::detail::wrapped_handler<boost::asio::strand, asio_helper::detail::callback<handler>> 
                callback_type;
            return_type operator ()(T0 t0,T1 t1,T2 t2,T3 t3,T4 t4){
                return std::make_tuple(std::move(t0),std::move(t1),std::move(t2),std::move(t3),std::move(t4));
            }
        };

        typedef handler<boost::system::error_code,std::size_t> read_handler;
        typedef read_handler write_handler;
        typedef handler<> completion_handler;
        typedef handler<boost::system::error_code> accept_handler;
        typedef accept_handler connect_handler;
        typedef accept_handler ssl_handshake_handler;
        typedef accept_handler ssl_shutdown_handler;
        typedef accept_handler wait_handler;
        typedef handler<boost::system::error_code,boost::asio::ip::tcp::resolver::iterator> composed_connect_handler;
        typedef composed_connect_handler resolve_handler;
        typedef handler<boost::system::error_code,int> signal_handler;
    }

    class async_helper{
        detail::coroutine_holder* co_;

        template<class R>
        R await(){
            ASIO_HELPER_ENTER_EXIT;
            assert(co_->coroutine_caller_);
            (*co_->coroutine_caller_)(nullptr);
            auto ret = reinterpret_cast<detail::ret_type*>(co_->coroutine_caller_->get());
            if(ret->eptr_){
                std::rethrow_exception(ret->eptr_);
            }
            auto r = std::move( *static_cast<R*>(ret->pv_));
            return r;
        }

        template<class F>
        boost::asio::detail::wrapped_handler<boost::asio::strand, detail::callback<F>> make_callback(F f){
            detail::callback<F> ret(co_,f);
            return co_->strand_.wrap(ret);
        }   

    public:
        async_helper(detail::coroutine_holder* c)
            :co_(c)
        {

        }


        template<class Handler,class F>
        typename Handler::return_type await(F f){
            {
                typename Handler::callback_type cb = make_callback(Handler());
                f(cb);
            }
            return await<typename Handler::return_type>();

        }



    };
    namespace detail{
        template<class F>
        struct simple_async_function_holder:public coroutine_holder{

            F f_;
            static void coroutine_function(coroutine_holder::co_type::caller_type& ca){
                ASIO_HELPER_ENTER_EXIT;
                auto p = ca.get();
                auto pthis = reinterpret_cast<simple_async_function_holder*>(p);
                pthis->coroutine_caller_ = &ca;
                // auto ptr = pthis->shared_from_this();
                try{
                    async_helper helper(pthis);
                    pthis->f_(helper);
                }
                catch(std::exception&){
                    auto eptr = std::current_exception();
                    pthis->io_.post(pthis->strand_.wrap([eptr](){
                        ASIO_HELPER_ENTER_EXIT;
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