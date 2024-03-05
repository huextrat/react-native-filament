//
// Created by Marc Rousavy on 21.02.24.
//

#pragma once

#include "EnumMapper.h"
#include "HybridObject.h"
#include <array>
#include <jsi/jsi.h>
#include <memory>
#include <type_traits>
#include <unordered_map>

namespace margelo {

using namespace facebook;

// Unknown type (error)
template <typename ArgType, typename Enable = void> struct JSIConverter {
  static ArgType fromJSI(jsi::Runtime&, const jsi::Value&) {
    static_assert(always_false<ArgType>::value, "This type is not supported by the JSIConverter!");
    return ArgType();
  }
  static jsi::Value toJSI(jsi::Runtime&, ArgType) {
    static_assert(always_false<ArgType>::value, "This type is not supported by the JSIConverter!");
    return jsi::Value::undefined();
  }

private:
  template <typename> struct always_false : std::false_type {};
};

// int <> number
template <> struct JSIConverter<int> {
  static int fromJSI(jsi::Runtime&, const jsi::Value& arg) {
    return static_cast<int>(arg.asNumber());
  }
  static jsi::Value toJSI(jsi::Runtime&, int arg) {
    return jsi::Value(arg);
  }
};

// double <> number
template <> struct JSIConverter<double> {
  static double fromJSI(jsi::Runtime&, const jsi::Value& arg) {
    return arg.asNumber();
  }
  static jsi::Value toJSI(jsi::Runtime&, double arg) {
    return jsi::Value(arg);
  }
};

// int64_t <> BigInt
template <> struct JSIConverter<int64_t> {
  static double fromJSI(jsi::Runtime& runtime, const jsi::Value& arg) {
    return arg.asBigInt(runtime).asInt64(runtime);
  }
  static jsi::Value toJSI(jsi::Runtime& runtime, int64_t arg) {
    return jsi::BigInt::fromInt64(runtime, arg);
  }
};

// uint64_t <> BigInt
template <> struct JSIConverter<uint64_t> {
  static double fromJSI(jsi::Runtime& runtime, const jsi::Value& arg) {
    return arg.asBigInt(runtime).asUint64(runtime);
  }
  static jsi::Value toJSI(jsi::Runtime& runtime, uint64_t arg) {
    return jsi::BigInt::fromUint64(runtime, arg);
  }
};

// bool <> boolean
template <> struct JSIConverter<bool> {
  static bool fromJSI(jsi::Runtime&, const jsi::Value& arg) {
    return arg.asBool();
  }
  static jsi::Value toJSI(jsi::Runtime&, bool arg) {
    return jsi::Value(arg);
  }
};

// std::string <> string
template <> struct JSIConverter<std::string> {
  static std::string fromJSI(jsi::Runtime& runtime, const jsi::Value& arg) {
    return arg.asString(runtime).utf8(runtime);
  }
  static jsi::Value toJSI(jsi::Runtime& runtime, const std::string& arg) {
    return jsi::String::createFromUtf8(runtime, arg);
  }
};

// std::optional<T> <> T | undefined
template <typename TInner> struct JSIConverter<std::optional<TInner>> {
  static std::optional<TInner> fromJSI(jsi::Runtime& runtime, const jsi::Value& arg) {
    if (arg.isUndefined() || arg.isNull()) {
      return std::nullopt;
    } else {
      return JSIConverter<TInner>::fromJSI(runtime, std::move(arg));
    }
  }
  static jsi::Value toJSI(jsi::Runtime& runtime, const std::optional<TInner>& arg) {
    if (arg == std::nullopt) {
      return jsi::Value::undefined();
    } else {
      return JSIConverter<TInner>::toJSI(runtime, arg.value());
    }
  }
};

// Enum <> Union
template <typename TEnum> struct JSIConverter<TEnum, std::enable_if_t<std::is_enum<TEnum>::value>> {
  static TEnum fromJSI(jsi::Runtime& runtime, const jsi::Value& arg) {
    std::string string = arg.asString(runtime).utf8(runtime);
    TEnum outEnum;
    EnumMapper::convertJSUnionToEnum(string, &outEnum);
    return outEnum;
  }
  static jsi::Value toJSI(jsi::Runtime& runtime, const TEnum& arg) {
    std::string outUnion;
    EnumMapper::convertEnumToJSUnion(arg, &outUnion);
    return jsi::String::createFromUtf8(runtime, outUnion);
  }
};

// [](Args...) -> T {} <> (Args...) => T
template <typename ReturnType, typename... Args> struct JSIConverter<std::function<ReturnType(Args...)>> {
  static std::function<ReturnType(Args...)> fromJSI(jsi::Runtime& runtime, const jsi::Value& arg) {
    jsi::Function function = arg.asObject(runtime).asFunction(runtime);
    std::shared_ptr<jsi::Function> sharedFunction = std::make_shared<jsi::Function>(std::move(function));
    return [&runtime, sharedFunction](Args... args) -> ReturnType {
      jsi::Value result = sharedFunction->call(runtime, JSIConverter<Args>::toJSI(runtime, args)...);
      if constexpr (std::is_same_v<ReturnType, void>) {
        // it is a void function (returns undefined)
        return;
      } else {
        // it returns a custom type, parse it from the JSI value.
        return JSIConverter<ReturnType>::fromJSI(runtime, std::move(result));
      }
    };
  }

  template <size_t... Is>
  static jsi::Value callHybridFunction(const std::function<ReturnType(Args...)>& function, jsi::Runtime& runtime, const jsi::Value* args,
                                       std::index_sequence<Is...>) {
    if constexpr (std::is_same_v<ReturnType, void>) {
      function(JSIConverter<Args>::fromJSI(runtime, args[Is])...);
      return jsi::Value::undefined();
    } else {
      ReturnType result = function(JSIConverter<Args>::fromJSI(runtime, args[Is])...);
      return JSIConverter<ReturnType>::toJSI(runtime, result);
    }
  }
  static jsi::Value toJSI(jsi::Runtime& runtime, const std::function<ReturnType(Args...)>& function) {
    jsi::HostFunctionType jsFunction = [function = std::move(function)](jsi::Runtime& runtime, const jsi::Value& thisValue,
                                                                        const jsi::Value* args, size_t count) -> jsi::Value {
      if (count != sizeof...(Args)) {
        [[unlikely]];
        throw jsi::JSError(runtime, "Function expected " + std::to_string(sizeof...(Args)) + " arguments, but received " +
                                        std::to_string(count) + "!");
      }
      return callHybridFunction(function, runtime, args, std::index_sequence_for<Args...>{});
    };
    return jsi::Function::createFromHostFunction(runtime, jsi::PropNameID::forUtf8(runtime, "hostFunction"), sizeof...(Args), jsFunction);
  }
};

// std::vector<T> <> T[]
template <typename ElementType> struct JSIConverter<std::vector<ElementType>> {
  static std::vector<ElementType> fromJSI(jsi::Runtime& runtime, const jsi::Value& arg) {
    jsi::Array array = arg.asObject(runtime).asArray(runtime);
    size_t length = array.size(runtime);

    std::vector<ElementType> vector;
    vector.reserve(length);
    for (size_t i = 0; i < length; ++i) {
      jsi::Value elementValue = array.getValueAtIndex(runtime, i);
      vector.emplace_back(JSIConverter<ElementType>::fromJSI(runtime, elementValue));
    }
    return vector;
  }
  static jsi::Value toJSI(jsi::Runtime& runtime, const std::vector<ElementType>& vector) {
    jsi::Array array(runtime, vector.size());
    for (size_t i = 0; i < vector.size(); i++) {
      jsi::Value value = JSIConverter<ElementType>::toJSI(runtime, vector[i]);
      array.setValueAtIndex(runtime, i, std::move(value));
    }
    return array;
  }
};

// std::unordered_map<std::string, T> <> Record<string, T>
template <typename ValueType> struct JSIConverter<std::unordered_map<std::string, ValueType>> {
  static std::unordered_map<std::string, ValueType> fromJSI(jsi::Runtime& runtime, const jsi::Value& arg) {
    jsi::Object object = arg.asObject(runtime);
    jsi::Array propertyNames = object.getPropertyNames(runtime);
    size_t length = propertyNames.size(runtime);

    std::unordered_map<std::string, ValueType> map;
    map.reserve(length);
    for (size_t i = 0; i < length; ++i) {
      std::string key = propertyNames.getValueAtIndex(runtime, i).asString(runtime).utf8(runtime);
      jsi::Value value = object.getProperty(runtime, key.c_str());
      map.emplace(key, JSIConverter<ValueType>::fromJSI(runtime, value));
    }
    return map;
  }
  static jsi::Value toJSI(jsi::Runtime& runtime, const std::unordered_map<std::string, ValueType>& map) {
    jsi::Object object(runtime);
    for (const auto& pair : map) {
      jsi::Value value = JSIConverter<ValueType>::toJSI(runtime, pair.second);
      jsi::String key = jsi::String::createFromUtf8(runtime, pair.first);
      object.setProperty(runtime, key, std::move(value));
    }
    return object;
  }
};

// HybridObject <> {}
template <typename T> struct is_shared_ptr_to_host_object : std::false_type {};

template <typename T> struct is_shared_ptr_to_host_object<std::shared_ptr<T>> : std::is_base_of<jsi::HostObject, T> {};

template <typename T> struct JSIConverter<T, std::enable_if_t<is_shared_ptr_to_host_object<T>::value>> {
  static T fromJSI(jsi::Runtime& runtime, const jsi::Value& arg) {
    return arg.asObject(runtime).asHostObject<typename T::element_type>(runtime);
  }
  static jsi::Value toJSI(jsi::Runtime& runtime, const T& arg) {
    return jsi::Object::createFromHostObject(runtime, arg);
  }
};

} // namespace margelo
