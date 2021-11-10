/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Oneflow User Op Definitions                                                *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/


#include "new_user_op.h"

namespace oneflow {

namespace user_op {

// operator `hello`: just a demo
struct HelloOp : OpConf {
public:
  // input parameters (2)
  TensorTuple a;
  TensorTuple b;
  
  TensorTuple& get_a() { return a; }
  TensorTuple& get_b() { return b; }
  
  // output parameters (1)
  TensorTuple c;
  
  TensorTuple& get_c() { return c; }
  
  // attributes (2)
  int num;
  std::vector<float> seeds;
  
  int& get_num() { return num; }
  std::vector<float>& get_seeds() { return seeds; }
  
  void *_get_void_ptr_num() { return reinterpret_cast<void *>(&num); }
  void *_get_void_ptr_seeds() { return reinterpret_cast<void *>(&seeds); }
  

  static constexpr const StringView name = "hello";
  StringView Name() const override { return name; }
  static constexpr const StringView description = "just a demo";
  StringView Description() const override { return description; }

  ParamIter InBegin() override { return ParamIter{in_map_.begin(), this}; }
  ParamIter InEnd() override { return ParamIter{in_map_.end(), this}; }
  ConstParamIter InBegin() const override { return ConstParamIter{in_map_.end(), this}; }
  ConstParamIter InEnd() const override { return ConstParamIter{in_map_.end(), this}; }
  size_t InSize() const override { return in_map_.size(); }
  Optional<TensorTuple&> In(StringView name) override {
    auto iter = in_map_.find(name);
    if (iter != in_map_.end()) {
      return iter->second(this);
    }

    return NullOpt;
  }
  Optional<const TensorTuple&> In(StringView) const override {
    auto iter = in_map_.find(name);
    if (iter != in_map_.end()) {
      return iter->second(const_cast<HelloOp*>(this));
    }

    return NullOpt;
  }

  ParamIter OutBegin() override { return ParamIter{out_map_.begin(), this}; }
  ParamIter OutEnd() override { return ParamIter{out_map_.end(), this}; }
  ConstParamIter OutBegin() const override { return ConstParamIter{out_map_.end(), this}; }
  ConstParamIter OutEnd() const override { return ConstParamIter{out_map_.end(), this}; }
  size_t OutSize() const override { return out_map_.size(); }
  Optional<TensorTuple&> Out(StringView name) override {
    auto iter = out_map_.find(name);
    if (iter != out_map_.end()) {
      return iter->second(this);
    }

    return NullOpt;
  }
  Optional<const TensorTuple&> Out(StringView) const override {
    auto iter = out_map_.find(name);
    if (iter != out_map_.end()) {
      return iter->second(const_cast<HelloOp*>(this));
    }

    return NullOpt;
  }

  AttrIter AttrBegin() override { return AttrIter{attr_map_.begin(), this}; }
  AttrIter AttrEnd() override { return AttrIter{attr_map_.end(), this}; }
  ConstAttrIter AttrBegin() const override { return ConstAttrIter{attr_map_.end(), this}; }
  ConstAttrIter AttrEnd() const override { return ConstAttrIter{attr_map_.end(), this}; }
  size_t AttrSize() const override { return attr_map_.size(); }
  std::pair<void*, std::type_index> AttrImpl(StringView name) override {
    auto iter = attr_map_.find(name);
    if (iter != attr_map_.end()) {
      return {iter->second.first(this), iter->second.second};
    }

    return {nullptr, typeid(void)};
  }
  std::pair<const void*, std::type_index> AttrImpl(StringView name) const override {
    auto iter = attr_map_.find(name);
    if (iter != attr_map_.end()) {
      return {iter->second.first(const_cast<HelloOp*>(this)), iter->second.second};
    }

    return {nullptr, typeid(void)};
  }

protected:
  static const ParamMap in_map_;
  static const ParamMap out_map_;
  static const AttrMap  attr_map_;
};
} // namespace user_op

} // namespace oneflow
