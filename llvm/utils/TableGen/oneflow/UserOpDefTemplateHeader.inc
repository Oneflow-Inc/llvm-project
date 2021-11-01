R"(
#include <typeindex>
#include "oneflow/core/framework/new_user_op.h"
#include "oneflow/core/common/string_view.h"

namespace oneflow {

namespace user_op {
{% for name, op in ops %}
// operator `{{ op.name }}`: {{ op.desc }}
struct {{ name }} : OpConf {
public:
  // input parameters ({{ length(op.in) }})
  {% for elem in op.in %}{{ elem.type }} {{ elem.name }};
  {% endfor %}
  // output parameters ({{ length(op.out) }})
  {% for elem in op.out %}{{ elem.type }} {{ elem.name }};
  {% endfor %}
  // attributes ({{ length(op.attr) }})
  {% for elem in op.attr %}{{ elem.type }} {{ elem.name }};
  {% endfor %}

  static constexpr const StringView Name = "{{ op.name }}";
  static constexpr const StringView Description = {{ quoted(op.desc) }};

protected:
  static const HashMap<StringView, std::size_t> in_map_;
  static const HashMap<StringView, std::size_t> out_map_;
  static const HashMap<StringView, std::pair<std::size_t, std::type_index>> attr_map_;
};{% endfor %}
} // namespace user_op

} // namespace oneflow
)"
