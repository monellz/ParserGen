#pragma once

#include <memory>
#include <type_traits>

namespace parsergen {

// llvm's riit

template <typename From>
struct simplify_type {
  using SimpleType = From;  // The real type this represents...

  // An accessor to get the real value...
  static SimpleType &getSimplifiedvalue(From &val) { return val; }
};

template <typename To, typename From, typename Enabler = void>
struct isa_impl {
  static inline bool doit(const From &val) { return To::classof(&val); }
};

template <typename To, typename From>
struct isa_impl<To, From, std::enable_if_t<std::is_base_of<To, From>::value>> {
  static inline bool doit(const From &) { return true; }
};

template <typename To, typename From>
struct isa_impl_cl {
  static inline bool doit(const From &val) {
    return isa_impl<To, From>::doit(val);
  }
};

template <typename To, typename From>
struct isa_impl_cl<To, const From> {
  static inline bool doit(const From &val) {
    return isa_impl<To, From>::doit(val);
  }
};

template <typename To, typename From>
struct isa_impl_cl<To, const std::unique_ptr<From>> {
  static inline bool doit(const std::unique_ptr<From> &val) {
    assert(val && "isa<> used on a null pointer");
    return isa_impl_cl<To, From>::doit(*val);
  }
};

template <typename To, typename From>
struct isa_impl_cl<To, From *> {
  static inline bool doit(const From *val) {
    assert(val && "isa<> used on a null pointer");
    return isa_impl<To, From>::doit(*val);
  }
};

template <typename To, typename From>
struct isa_impl_cl<To, From *const> {
  static inline bool doit(const From *val) {
    assert(val && "isa<> used on a null pointer");
    return isa_impl<To, From>::doit(*val);
  }
};

template <typename To, typename From>
struct isa_impl_cl<To, const From *> {
  static inline bool doit(const From *val) {
    assert(val && "isa<> used on a null pointer");
    return isa_impl<To, From>::doit(*val);
  }
};

template <typename To, typename From>
struct isa_impl_cl<To, const From *const> {
  static inline bool doit(const From *val) {
    assert(val && "isa<> used on a null pointer");
    return isa_impl<To, From>::doit(*val);
  }
};

template <typename To, typename From, typename SimpleFrom>
struct isa_impl_wrap {
  // When From != SimplifiedType, we can simplify the type some more by using
  // the simplify_type template.
  static bool doit(const From &val) {
    return isa_impl_wrap<To, SimpleFrom,
                         typename simplify_type<SimpleFrom>::SimpleType>::
        doit(simplify_type<const From>::getSimplifiedvalue(val));
  }
};

template <typename To, typename FromTy>
struct isa_impl_wrap<To, FromTy, FromTy> {
  // When From == SimpleType, we are as simple as we are going to get.
  static bool doit(const FromTy &val) {
    return isa_impl_cl<To, FromTy>::doit(val);
  }
};

template <class X, class Y>
inline bool isa(const Y &val) {
  return isa_impl_wrap<X, const Y,
                       typename simplify_type<const Y>::SimpleType>::doit(val);
}

template <typename First, typename Second, typename... Rest, typename Y>
inline bool isa(const Y &val) {
  return isa<First>(val) || isa<Second, Rest...>(val);
}

}  // namespace parsergen
