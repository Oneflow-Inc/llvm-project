/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Oneflow User Op Definitions                                                *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/


#include "hello.h"

namespace oneflow {

namespace user_op {

const ParamMap HelloOp::in_map_ {
    { "a", static_cast<ParamMemFn>(&HelloOp::get_a) },
    { "b", static_cast<ParamMemFn>(&HelloOp::get_b) },
    
};

const ParamMap HelloOp::out_map_ {
    { "c", static_cast<ParamMemFn>(&HelloOp::get_c) },
    
};

const AttrMap HelloOp::attr_map_ {
    { "num", { static_cast<AttrMemFn>(&HelloOp::_get_void_ptr_num), typeid(int) } },
    { "seeds", { static_cast<AttrMemFn>(&HelloOp::_get_void_ptr_seeds), typeid(std::vector<float>) } },
    
};
} // namespace user_op
} // namespace oneflow
