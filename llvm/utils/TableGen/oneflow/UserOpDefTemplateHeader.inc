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
  {{ elem.type }}& get_{{ elem.name }}() { return {{ elem.name }}; }
  {% endfor %}
  // output parameters ({{ length(op.out) }})
  {% for elem in op.out %}{{ elem.type }} {{ elem.name }};
  {{ elem.type }}& get_{{ elem.name }}() { return {{ elem.name }}; }
  {% endfor %}
  // attributes ({{ length(op.attr) }})
  {% for elem in op.attr %}{{ elem.type }} {{ elem.name }};
  {{ elem.type }}& get_{{ elem.name }}() { return {{ elem.name }}; }
  void *_get_void_ptr_{{ elem.name }}() { return reinterpret_cast<void *>(&{{ elem.name }}); }
  {% endfor %}

  static constexpr const StringView Name = "{{ op.name }}";
  StringView Name() const override { return Name; }
  static constexpr const StringView Description = {{ quoted(op.desc) }};
  StringView Description() const override { return Description; }

protected:
  static const ParamMap in_map_;
  static const ParamMap out_map_;
  static const AttrMap  attr_map_;
};{% endfor %}
} // namespace user_op

} // namespace oneflow
)"
