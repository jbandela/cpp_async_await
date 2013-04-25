
#pragma once
#ifndef ASYNC_HELPER_PPLX_HELPER_INCLUDED_04_25_2013
#define ASYNC_HELPER_PPLX_HELPER_INCLUDED_04_25_2013
#include<pplxtasks.h>
#define CPP_ASYNC_AWAIT_PPL_NAMESPACE pplx_helper

namespace CPP_ASYNC_AWAIT_PPL_NAMESPACE{
    namespace detail{
        template<class T>
        struct task_type{
            typedef pplx::task<T> type;
        };

    }


}

#define CPP_ASYNC_AWAIT_PPL_TASK(R) pplx::task<R>

#include "ppl_helper_imp.hpp"


#endif