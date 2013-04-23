#include <boost/asio.hpp>
#include <boost/coroutine/all.hpp>
#include <memory>
#include <exception>
#include <type_traits>
#include <assert.h>

struct EnterExit{
    EnterExit():n_(++number_){ std::cerr << "==" << n_ << " Entering\n";}
    int n_;
    std::string s_;
    static int number_;
    ~EnterExit(){std::cerr << "==" << n_ << " Exiting\n";}

};
int EnterExit::number_ = 0;

namespace asio_helper{
    namespace detail{
        struct coroutine_holder:std::enable_shared_from_this<coroutine_holder>{
            EnterExit e;
            typedef boost::coroutines::coroutine<void*(void*)> co_type;
            std::unique_ptr<co_type> coroutine_;
            co_type::caller_type* coroutine_caller_;
            coroutine_holder():coroutine_(nullptr),coroutine_caller_(nullptr){}

        };

        struct ret_type{
            std::exception_ptr eptr_;
            void* pv_;

        };

        template<class F>
        struct callback{

            F f_;

            std::shared_ptr<coroutine_holder> co_;

            callback(std::shared_ptr<coroutine_holder> c,F f):f_(f),co_(c){}

            void operator()(){
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

                co_->fc_original_ = &mycontext;
                boost::context::jump_fcontext(&mycontext,co_->fc_,reinterpret_cast<intptr_t>(&r));
                co_->fc_original_ = nullptr;
            }

            template<class T0>
            void operator()(T0&& t0){
                EnterExit e;
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


        };
    }
    struct async_helper{
        std::shared_ptr<detail::coroutine_holder> co_;

        async_helper(std::shared_ptr<detail::coroutine_holder> c)
            :co_(c)
        {

        }

        template<class R>
        R await(){
            assert(co_->coroutine_caller_);
            (*co_->coroutine_caller_)(nullptr);
            auto ret = reinterpret_cast<detail::ret_type*>(co_->coroutine_caller_->get());
            if(ret->eptr_){
                std::rethrow_exception(ret->eptr_);
            }
            else{
                auto r = std::move( *static_cast<R*>(ret->pv_));
                return r;
            }
        }

        template<class F>
        detail::callback<F> make_callback(F f){
            detail::callback<F> ret(co_,f);
            return ret;
        }

    };
    namespace detail{
        template<class F>
        struct simple_async_function_holder:public coroutine_holder{

            F f_;
            static void coroutine_function(coroutine_holder::co_type::caller_type& ca){
                EnterExit e;
                auto p = ca.get();
                auto pthis = reinterpret_cast<simple_async_function_holder*>(p);
                pthis->coroutine_caller_ = &ca;
                auto ptr = pthis->shared_from_this();
                    async_helper helper(ptr);
                    pthis->f_(helper);
                return;
            }
            simple_async_function_holder(F f):f_(f){

            }
            void run(){
                coroutine_.reset(new coroutine_holder::co_type(&coroutine_function,this));


            }

        };

    }
    template<class F>
    void do_async(F f){
        auto ret = std::make_shared<detail::simple_async_function_holder<F>>(f);

        ret->run();

    }






}