R"(
#include "new_user_op.h"

namespace oneflow {

namespace user_op {
{% for name, op in ops %}
// operator `{{ op.name }}`: {{ op.desc }}
struct {{ name }} : OpConf {
public:
  // input parameters ({{ length(op.in) }})
  {% for elem in op.in %}{{ elem.type }} {{ elem.name }};
  {% endfor %}
  {% for elem in op.in %}{{ elem.type }}& get_{{ elem.name }}() { return {{ elem.name }}; }
  {% endfor %}
  // output parameters ({{ length(op.out) }})
  {% for elem in op.out %}{{ elem.type }} {{ elem.name }};
  {% endfor %}
  {% for elem in op.out %}{{ elem.type }}& get_{{ elem.name }}() { return {{ elem.name }}; }
  {% endfor %}
  // attributes ({{ length(op.attr) }})
  {% for elem in op.attr %}{{ elem.type }} {{ elem.name }};
  {% endfor %}
  {% for elem in op.attr %}{{ elem.type }}& get_{{ elem.name }}() { return {{ elem.name }}; }
  {% endfor %}
  {% for elem in op.attr %}void *_get_void_ptr_{{ elem.name }}() { return reinterpret_cast<void *>(&{{ elem.name }}); }
  {% endfor %}

  static constexpr const StringView name = "{{ op.name }}";
  StringView Name() const override { return name; }
  static constexpr const StringView description = {{ quoted(op.desc) }};
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
      return iter->second(const_cast<{{ name }}*>(this));
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
      return iter->second(const_cast<{{ name }}*>(this));
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
      return {iter->second.first(const_cast<{{ name }}*>(this)), iter->second.second};
    }

    return {nullptr, typeid(void)};
  }

protected:
  static const ParamMap in_map_;
  static const ParamMap out_map_;
  static const AttrMap  attr_map_;
};{% endfor %}
} // namespace user_op

} // namespace oneflow
)"
