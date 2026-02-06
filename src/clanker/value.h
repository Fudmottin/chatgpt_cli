// src/clanker/value.h
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace clanker {

// Forward declarations for future growth.
struct Proc;      // callable closure (AST + env), defined later
struct JsonValue; // your own JSON type, or an adapter around Boost.JSON

class Value {
 public:
   using Int = std::int64_t;

   struct Null final {};

   using String = std::string;
   using List = std::vector<Value>;

   // Use shared_ptr to keep Value move/copy cheap and avoid giant variants.
   using Json = std::shared_ptr<const JsonValue>;
   using ProcPtr = std::shared_ptr<const Proc>;

   using Storage = std::variant<Null, bool, Int, String, List, Json, ProcPtr>;

   Value()
      : v_(Null{}) {}
   explicit Value(Null)
      : v_(Null{}) {}
   explicit Value(bool b)
      : v_(b) {}
   explicit Value(Int i)
      : v_(i) {}
   explicit Value(String s)
      : v_(std::move(s)) {}
   explicit Value(List xs)
      : v_(std::move(xs)) {}
   explicit Value(Json j)
      : v_(std::move(j)) {}
   explicit Value(ProcPtr p)
      : v_(std::move(p)) {}

   [[nodiscard]] bool is_null() const noexcept {
      return std::holds_alternative<Null>(v_);
   }
   [[nodiscard]] bool is_bool() const noexcept {
      return std::holds_alternative<bool>(v_);
   }
   [[nodiscard]] bool is_int() const noexcept {
      return std::holds_alternative<Int>(v_);
   }
   [[nodiscard]] bool is_string() const noexcept {
      return std::holds_alternative<String>(v_);
   }
   [[nodiscard]] bool is_list() const noexcept {
      return std::holds_alternative<List>(v_);
   }
   [[nodiscard]] bool is_json() const noexcept {
      return std::holds_alternative<Json>(v_);
   }
   [[nodiscard]] bool is_proc() const noexcept {
      return std::holds_alternative<ProcPtr>(v_);
   }

   [[nodiscard]] const Storage& storage() const noexcept { return v_; }

   [[nodiscard]] const bool* as_bool() const noexcept {
      return std::get_if<bool>(&v_);
   }
   [[nodiscard]] const Int* as_int() const noexcept {
      return std::get_if<Int>(&v_);
   }
   [[nodiscard]] const String* as_string() const noexcept {
      return std::get_if<String>(&v_);
   }
   [[nodiscard]] const List* as_list() const noexcept {
      return std::get_if<List>(&v_);
   }
   [[nodiscard]] const Json* as_json() const noexcept {
      return std::get_if<Json>(&v_);
   }
   [[nodiscard]] const ProcPtr* as_proc() const noexcept {
      return std::get_if<ProcPtr>(&v_);
   }

 private:
   Storage v_;
};

// A tiny “type tag” for diagnostics and conversions.
enum class ValueKind {
   Null,
   Bool,
   Int,
   String,
   List,
   Json,
   Proc,
};

[[nodiscard]] inline ValueKind kind_of(const Value& v) noexcept {
   return std::visit(
      [](const auto& x) noexcept -> ValueKind {
         using T = std::decay_t<decltype(x)>;
         if constexpr (std::is_same_v<T, Value::Null>)
            return ValueKind::Null;
         else if constexpr (std::is_same_v<T, bool>)
            return ValueKind::Bool;
         else if constexpr (std::is_same_v<T, Value::Int>)
            return ValueKind::Int;
         else if constexpr (std::is_same_v<T, Value::String>)
            return ValueKind::String;
         else if constexpr (std::is_same_v<T, Value::List>)
            return ValueKind::List;
         else if constexpr (std::is_same_v<T, Value::Json>)
            return ValueKind::Json;
         else
            return ValueKind::Proc;
      },
      v.storage());
}

[[nodiscard]] inline const char* kind_name(ValueKind k) noexcept {
   switch (k) {
   case ValueKind::Null:
      return "null";
   case ValueKind::Bool:
      return "bool";
   case ValueKind::Int:
      return "int";
   case ValueKind::String:
      return "string";
   case ValueKind::List:
      return "list";
   case ValueKind::Json:
      return "json";
   case ValueKind::Proc:
      return "proc";
   }
   return "unknown";
}

template<class T>
struct Result {
   T value;
   std::string error;
   SourceLoc loc{};
   bool ok() const noexcept { return error.empty(); }
};

} // namespace clanker

