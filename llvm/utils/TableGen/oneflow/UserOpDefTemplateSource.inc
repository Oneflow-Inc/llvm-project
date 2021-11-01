R"(
{% if include != "" %}#include "{{ include }}"
{% else if filename != "" %}#include "{{ to_header(filename) }}"
{% endif %}
namespace oneflow {

namespace user_op {
{% for name, op in ops %}
const HashMap<StringView, std::size_t> {{ name }}::in_map_ {
    {% for elem in op.in %}{ "{{ elem.name }}", offsetof({{ name }}, {{ elem.name }}) },
    {% endfor %}
};{% endfor %}
{% for name, op in ops %}
const HashMap<StringView, std::size_t> {{ name }}::out_map_ {
    {% for elem in op.out %}{ "{{ elem.name }}", offsetof({{ name }}, {{ elem.name }}) },
    {% endfor %}
};{% endfor %}
{% for name, op in ops %}
const HashMap<StringView, std::pair<std::size_t, std::type_index>> {{ name }}::attr_map_ {
    {% for elem in op.attr %}{ "{{ elem.name }}", { offsetof({{ name }}, {{ elem.name }}), typeof({{ elem.type }}) } },
    {% endfor %}
};{% endfor %}
} // namespace user_op
} // namespace oneflow
)"
