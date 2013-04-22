#include <boost/asio.hpp>
#include <boost/context/all.hpp>
#include <boost/coroutine/stack_allocator.hpp>
#include <memory>
#include <exception>
#include <type_traits>
#include <assert.h>

namespace asio_helper{
namespace detail{
struct context_holder:std::enable_shared_from_this<context_holder>{
    boost::context::fcontext_t* fc_;
    boost::context::fcontext_t* fc_original_;
    boost::coroutines::stack_allocator stack_;
    void* sp_;
    std::size_t size_;

    typedef void(*fptr)(intptr_t);


    context_holder(fptr f){
        size_ = boost::coroutines::stack_allocator::default_stacksize();
        sp_ = stack_.allocate(size_);
        fc_ = boost::context::make_fcontext(sp_,size_,f);
        fc_original_ = nullptr;
    }
    ~context_holder(){
        stack_.deallocate(sp_,size_);
    }

};

struct ret_type{
    std::exception_ptr eptr_;
    void* pv_;

};

template<class F>
struct callback{

    F f_;

    std::shared_ptr<context_holder> context_;

    callback(std::shared_ptr<context_holder> c,F f):f_(f),context_(c){}

    void operator()(){
     boost::context::fcontext_t mycontext;
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

        context_->fc_original_ = &mycontext;
        boost::context::jump_fcontext(&mycontext,context_->fc_,reinterpret_cast<intptr_t>(&r));
        context_->fc_original_ = nullptr;
    }

    template<class T0>
    void operator()(T0&& t0){
       boost::context::fcontext_t mycontext;
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

        context_->fc_original_ = &mycontext;
        boost::context::jump_fcontext(&mycontext,context_->fc_,reinterpret_cast<intptr_t>(&r));
        context_->fc_original_ = nullptr;
    }    
    template<class T0,class T1>
    void operator()(T0&& t0,T1&& t1){
       boost::context::fcontext_t mycontext;
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

        context_->fc_original_ = &mycontext;
        boost::context::jump_fcontext(&mycontext,context_->fc_,reinterpret_cast<intptr_t>(&r));
        context_->fc_original_ = nullptr;
    }  
    template<class T0,class T1,class T2>
    void operator()(T0&& t0,T1&& t1,T2&& t2){
             boost::context::fcontext_t mycontext;

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
        context_->fc_original_ = &mycontext;
        boost::context::jump_fcontext(&mycontext,context_->fc_,reinterpret_cast<intptr_t>(&r));
        context_->fc_original_ = nullptr;
    }


};
}
 struct async_helper{
    std::shared_ptr<detail::context_holder> context_;

    async_helper(std::shared_ptr<detail::context_holder> c)
        :context_(c)
    {

    }

    template<class R>
    R await(){
        assert(context_->fc_original_);
        auto ret = reinterpret_cast<detail::ret_type*>(boost::context::jump_fcontext(context_->fc_,context_->fc_original_,reinterpret_cast<intptr_t>(nullptr)));
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
        detail::callback<F> ret(context_,f);
        return ret;
    }

};
 namespace detail{
template<class F>
struct simple_async_function_holder:public context_holder{

    F f_;
    bool done_;
    std::exception_ptr eptr_;

    static void context_function(intptr_t p){
        auto pthis = reinterpret_cast<simple_async_function_holder*>(p);
        auto ptr = pthis->shared_from_this();
        try{
             async_helper helper(ptr);
             pthis->f_(helper);
        }
        catch(...){
            pthis->eptr_ = std::current_exception();
        }
        pthis->done_ = true;
        auto fc = pthis->fc_;
        auto fc_original = pthis->fc_original_;
        ptr.reset();
        boost::context::jump_fcontext(fc,fc_original,reinterpret_cast<intptr_t>(nullptr));
    }
    simple_async_function_holder(F f):context_holder(&context_function),f_(f),done_(false),eptr_(nullptr){

    }
    bool done(){return done_;}
    void run(){
        boost::context::fcontext_t mycontext;
        this->fc_original_ = &mycontext;
        boost::context::jump_fcontext(&mycontext,this->fc_,reinterpret_cast<intptr_t>(this));
        this->fc_original_ = nullptr;

    }

    ~simple_async_function_holder(){
        std::cerr << "\nDestructor\n";
    }

};

template<class F>
struct simple_async_function:public std::unary_function<boost::asio::io_service&,void>{

    simple_async_function(F f):holder_( std::make_shared<simple_async_function_holder<F>>(f)){}

    void operator()(boost::asio::io_service& io){
        holder_->run();
        while(!holder_->done()){
            io.run_one();
        }
        if(holder_->eptr_){
            std::rethrow_exception(holder_->eptr_);

        }
    }

private:
    std::shared_ptr<simple_async_function_holder<F>> holder_;

};
 }
template<class F>
void do_async(boost::asio::io_service& s, F f){
    auto ret = std::make_shared<detail::simple_async_function_holder<F>>(f);

   ret->run();
   while(!ret->done()){
       s.run_one();
   }
   if(ret->eptr_){
       std::rethrow_exception(ret->eptr_);

   }
}




template<class F>
detail::simple_async_function<F> get_async_function(F f){
    return detail::simple_async_function<F>(f);

}

}