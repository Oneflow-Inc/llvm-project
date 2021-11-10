R"(
{% if include != "" %}#include "{{ include }}"
{% else if filename != "" %}#include "{{ to_header(filename) }}"
{% endif %}
namespace oneflow {

namespace user_op {
{% for name, op in ops %}
const ParamMap {{ name }}::in_map_ {
    {% for elem in op.in %}{ "{{ elem.name }}", static_cast<ParamMemFn>(&{{ name }}::get_{{ elem.name }}) },
    {% endfor %}
};{% endfor %}
{% for name, op in ops %}
const ParamMap {{ name }}::out_map_ {
    {% for elem in op.out %}{ "{{ elem.name }}", static_cast<ParamMemFn>(&{{ name }}::get_{{ elem.name }}) },
    {% endfor %}
};{% endfor %}
{% for name, op in ops %}
const AttrMap {{ name }}::attr_map_ {
    {% for elem in op.attr %}{ "{{ elem.name }}", { static_cast<AttrMemFn>(&{{ name }}::_get_void_ptr_{{ elem.name }}), typeid({{ elem.type }}) } },
    {% endfor %}
};{% endfor %}
} // namespace user_op
} // namespace oneflow
)"
