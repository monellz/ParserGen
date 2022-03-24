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

//===----------------------------------------------------------------------===//
//                          cast<x> Support Templates
//===----------------------------------------------------------------------===//

template <class To, class From>
struct cast_retty;

// Calculate what type the 'cast' function should return, based on a requested
// type of To and a source type of From.
template <class To, class From>
struct cast_retty_impl {
  using ret_type = To &;  // Normal case, return Ty&
};
template <class To, class From>
struct cast_retty_impl<To, const From> {
  using ret_type = const To &;  // Normal case, return Ty&
};

template <class To, class From>
struct cast_retty_impl<To, From *> {
  using ret_type = To *;  // Pointer arg case, return Ty*
};

template <class To, class From>
struct cast_retty_impl<To, const From *> {
  using ret_type = const To *;  // Constant pointer arg case, return const Ty*
};

template <class To, class From>
struct cast_retty_impl<To, const From *const> {
  using ret_type = const To *;  // Constant pointer arg case, return const Ty*
};

template <class To, class From>
struct cast_retty_impl<To, std::unique_ptr<From>> {
 private:
  using PointerType = typename cast_retty_impl<To, From *>::ret_type;
  using ResultType = std::remove_pointer_t<PointerType>;

 public:
  using ret_type = std::unique_ptr<ResultType>;
};

template <class To, class From, class SimpleFrom>
struct cast_retty_wrap {
  // When the simplified type and the from type are not the same, use the type
  // simplifier to reduce the type, then reuse cast_retty_impl to get the
  // resultant type.
  using ret_type = typename cast_retty<To, SimpleFrom>::ret_type;
};

template <class To, class FromTy>
struct cast_retty_wrap<To, FromTy, FromTy> {
  // When the simplified type is equal to the from type, use it directly.
  using ret_type = typename cast_retty_impl<To, FromTy>::ret_type;
};

template <class To, class From>
struct cast_retty {
  using ret_type = typename cast_retty_wrap<
      To, From, typename simplify_type<From>::SimpleType>::ret_type;
};

// Ensure the non-simple values are converted using the simplify_type template
// that may be specialized by smart pointers...
//
template <class To, class From, class SimpleFrom>
struct cast_convert_val {
  // This is not a simple type, use the template to simplify it...
  static typename cast_retty<To, From>::ret_type doit(From &Val) {
    return cast_convert_val<To, SimpleFrom,
                            typename simplify_type<SimpleFrom>::SimpleType>::
        doit(simplify_type<From>::getSimplifiedValue(Val));
  }
};

template <class To, class FromTy>
struct cast_convert_val<To, FromTy, FromTy> {
  // This _is_ a simple type, just cast it.
  static typename cast_retty<To, FromTy>::ret_type doit(const FromTy &Val) {
    typename cast_retty<To, FromTy>::ret_type Res2 =
        (typename cast_retty<To, FromTy>::ret_type) const_cast<FromTy &>(Val);
    return Res2;
  }
};

template <class X>
struct is_simple_type {
  static const bool value =
      std::is_same<X, typename simplify_type<X>::SimpleType>::value;
};

// cast<X> - Return the argument parameter cast to the specified type.  This
// casting operator asserts that the type is correct, so it does not return null
// on failure.  It does not allow a null argument (use cast_or_null for that).
// It is typically used like this:
//
//  cast<Instruction>(myVal)->getParent()
//
template <class X, class Y>
inline std::enable_if_t<!is_simple_type<Y>::value,
                        typename cast_retty<X, const Y>::ret_type>
cast(const Y &Val) {
  assert(isa<X>(Val) && "cast<Ty>() argument of incompatible type!");
  return cast_convert_val<
      X, const Y, typename simplify_type<const Y>::SimpleType>::doit(Val);
}

template <class X, class Y>
inline typename cast_retty<X, Y>::ret_type cast(Y &Val) {
  assert(isa<X>(Val) && "cast<Ty>() argument of incompatible type!");
  return cast_convert_val<X, Y, typename simplify_type<Y>::SimpleType>::doit(
      Val);
}

template <class X, class Y>
inline typename cast_retty<X, Y *>::ret_type cast(Y *Val) {
  assert(isa<X>(Val) && "cast<Ty>() argument of incompatible type!");
  return cast_convert_val<X, Y *,
                          typename simplify_type<Y *>::SimpleType>::doit(Val);
}

template <class X, class Y>
inline typename cast_retty<X, std::unique_ptr<Y>>::ret_type cast(
    std::unique_ptr<Y> &&Val) {
  assert(isa<X>(Val.get()) && "cast<Ty>() argument of incompatible type!");
  using ret_type = typename cast_retty<X, std::unique_ptr<Y>>::ret_type;
  return ret_type(
      cast_convert_val<X, Y *, typename simplify_type<Y *>::SimpleType>::doit(
          Val.release()));
}

// dyn_cast<X> - Return the argument parameter cast to the specified type.  This
// casting operator returns null if the argument is of the wrong type, so it can
// be used to test for a type as well as cast if successful.  This should be
// used in the context of an if statement like this:
//
//  if (const Instruction *I = dyn_cast<Instruction>(myVal)) { ... }
//

template <class X, class Y>
inline std::enable_if_t<!is_simple_type<Y>::value,
                        typename cast_retty<X, const Y>::ret_type>
dyn_cast(const Y &Val) {
  return isa<X>(Val) ? cast<X>(Val) : nullptr;
}

template <class X, class Y>
inline typename cast_retty<X, Y>::ret_type dyn_cast(Y &Val) {
  return isa<X>(Val) ? cast<X>(Val) : nullptr;
}

template <class X, class Y>
inline typename cast_retty<X, Y *>::ret_type dyn_cast(Y *Val) {
  return isa<X>(Val) ? cast<X>(Val) : nullptr;
}

// unique_dyn_cast<X> - Given a unique_ptr<Y>, try to return a unique_ptr<X>,
// taking ownership of the input pointer iff isa<X>(Val) is true.  If the
// cast is successful, From refers to nullptr on exit and the casted value
// is returned.  If the cast is unsuccessful, the function returns nullptr
// and From is unchanged.
template <class X, class Y>
inline auto unique_dyn_cast(std::unique_ptr<Y> &Val) -> decltype(cast<X>(Val)) {
  if (!isa<X>(Val)) return nullptr;
  return cast<X>(std::move(Val));
}

template <class X, class Y>
inline auto unique_dyn_cast(std::unique_ptr<Y> &&Val) {
  return unique_dyn_cast<X, Y>(Val);
}

}  // namespace parsergen
