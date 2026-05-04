#include <cstddef>
#include <optional>
#include <vector>

#include <rice/rice.hpp>

#include "stl.hpp"

Rice::Array to_a(const std::vector<float>& x) {
  Rice::Array a;
  for (const auto v : x) {
    a.push(v, false);
  }
  return a;
}

template<typename T>
static void set_optional(std::optional<T>& field, Rice::Object value) {
  if (value.is_nil()) {
    field = std::nullopt;
  } else {
    field = Rice::detail::From_Ruby<T>().convert(value.value());
  }
}

template<typename T>
static Rice::Object get_optional(const std::optional<T>& field) {
  if (!field.has_value()) {
    return Rice::Object();
  }
  return Rice::Object(Rice::detail::To_Ruby<T>().convert(*field));
}

#define DEFINE_OPTIONAL_ATTR(klass, field) \
  klass.define_method(#field, \
    [](const stl::StlParams& self) { return get_optional(self.field); }); \
  klass.define_method(#field "=", \
    [](stl::StlParams& self, Rice::Object value) { set_optional(self.field, value); })

extern "C"
void Init_ext() {
  Rice::Module rb_mStl = Rice::define_module("Stl");

  auto rb_cStlParams = Rice::define_class_under<stl::StlParams>(rb_mStl, "StlParams")
    .define_constructor(Rice::Constructor<stl::StlParams>())
    .define_attr("seasonal_degree", &stl::StlParams::seasonal_degree)
    .define_attr("trend_degree", &stl::StlParams::trend_degree)
    .define_attr("robust", &stl::StlParams::robust);

  DEFINE_OPTIONAL_ATTR(rb_cStlParams, seasonal_length);
  DEFINE_OPTIONAL_ATTR(rb_cStlParams, trend_length);
  DEFINE_OPTIONAL_ATTR(rb_cStlParams, low_pass_length);
  DEFINE_OPTIONAL_ATTR(rb_cStlParams, low_pass_degree);
  DEFINE_OPTIONAL_ATTR(rb_cStlParams, seasonal_jump);
  DEFINE_OPTIONAL_ATTR(rb_cStlParams, trend_jump);
  DEFINE_OPTIONAL_ATTR(rb_cStlParams, low_pass_jump);
  DEFINE_OPTIONAL_ATTR(rb_cStlParams, inner_loops);
  DEFINE_OPTIONAL_ATTR(rb_cStlParams, outer_loops);

  rb_mStl
    .define_singleton_function(
      "_decompose",
      [](Rice::Array rb_series, size_t period, const stl::StlParams& params, bool weights) {
        std::vector<float> series = rb_series.to_vector<float>();
        stl::Stl fit{series, period, params};

        Rice::Hash ret;
        ret[Rice::Symbol("seasonal")] = to_a(fit.seasonal());
        ret[Rice::Symbol("trend")] = to_a(fit.trend());
        ret[Rice::Symbol("remainder")] = to_a(fit.remainder());
        if (weights) {
          ret[Rice::Symbol("weights")] = to_a(fit.weights());
        }
        return ret;
      });
}
