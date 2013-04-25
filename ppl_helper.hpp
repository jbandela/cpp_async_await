#pragma once
#ifndef ASYNC_HELPER_PPL_HELPER_INCLUDED_04_25_2013
#define ASYNC_HELPER_PPL_HELPER_INCLUDED_04_25_2013
#include <ppltasks.h>
#define CPP_ASYNC_AWAIT_PPL_NAMESPACE ppl_helper

namespace CPP_ASYNC_AWAIT_PPL_NAMESPACE{
    namespace detail{
        template<class T>
        struct task_type{
            typedef concurrency::task<T> type;
        };

    }


}

#define CPP_ASYNC_AWAIT_PPL_TASK(R) concurrency::task<R>

#include "ppl_helper_imp.hpp"


#endif