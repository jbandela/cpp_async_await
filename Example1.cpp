#include <iostream>
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/context/all.hpp>
#include <boost/coroutine/stack_allocator.hpp>
#include <memory>
#include <exception>
#include <type_traits>
#include <assert.h>


struct context_holder{
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

 struct simple_async_function_helper{
    std::shared_ptr<context_holder> context_;

    simple_async_function_helper(std::shared_ptr<context_holder> c)
        :context_(c)
    {

    }

    template<class R>
    R await(){
        assert(context_->fc_original_);
        auto ret = reinterpret_cast<ret_type*>(boost::context::jump_fcontext(context_->fc_,context_->fc_original_,reinterpret_cast<intptr_t>(nullptr)));
        if(ret->eptr_){
            std::rethrow_exception(ret->eptr_);
        }
        else{
            return *static_cast<R*>(ret->pv_);
        }
    }
    
    template<class F>
    callback<F> make_callback(F f){
        callback<F> ret(context_,f);
        return ret;
    }

};

template<class F>
struct simple_async_function{

    std::shared_ptr<context_holder> context_;
    F f_;


    static void context_function(intptr_t p){
        auto pthis = reinterpret_cast<simple_async_function*>(p);
         try{
             auto ptr = pthis->context_;
             simple_async_function_helper helper(ptr);
             pthis->f_(helper);


        }
        catch(...){

        }
        boost::context::jump_fcontext(pthis->context_->fc_,pthis->context_->fc_original_,reinterpret_cast<intptr_t>(nullptr));

    };
    simple_async_function(F f):f_(f){
        context_ = std::make_shared<context_holder>(&context_function);

    }

    void operator()(){
        boost::context::fcontext_t mycontext;
        context_->fc_original_ = &mycontext;
        boost::context::jump_fcontext(&mycontext,context_->fc_,reinterpret_cast<intptr_t>(this));
        context_->fc_original_ = nullptr;

    }



};


template<class F>
 simple_async_function<F> make_async(F f){
    simple_async_function<F> ret(f);

   return ret;
}

void print(const boost::system::error_code& /*e*/)
{
  std::cout << "Hello, world!\n";
}

int main()
{
  boost::asio::io_service io;
  boost::asio::deadline_timer t(io, boost::posix_time::seconds(5));


auto f = make_async([&](simple_async_function_helper helper){
      
      for(int i = 0; i < 5; i++){
          auto cb = helper.make_callback([&](const boost::system::error_code&)->int{std::cerr << "Inside callback\n";return 0;});

        t.async_wait(cb);
        helper.await<int>();
        t.expires_from_now(boost::posix_time::seconds(5));
        std::cout << "Timer went off " << (i+1) << " times " << std::endl;
      }


  });

  //t.async_wait(&print);
 f();
  io.run();

  return 0;
}
