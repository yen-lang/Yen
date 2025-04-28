#pragma once
#include <variant>
#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

// Primeiro, define a struct que tava faltando
struct ObjectInstance {
    std::unordered_map<std::string, struct Value> fields;
};

struct ClassInstance {
    std::unordered_map<std::string, struct Value> fields;
};


// Agora define o ValueVariant
using ValueVariant = std::variant<
    std::monostate,
    int,
    double,
    float,
    bool,
    std::string,
    std::vector<struct Value>,
    std::unordered_map<std::string, struct Value>,
    std::shared_ptr<ClassInstance>,
    std::shared_ptr<ObjectInstance>
>;

// Agora finalmente o struct Value
struct Value : ValueVariant {
    using ValueVariant::ValueVariant;

    bool operator==(const Value& other) const {
        return static_cast<const ValueVariant&>(*this) == static_cast<const ValueVariant&>(other);
    }

    bool operator<(const Value& other) const {
        if (this->index() != other.index()) return this->index() < other.index();
        return std::visit([](const auto& a, const auto& b) -> bool {
            using A = std::decay_t<decltype(a)>;
            using B = std::decay_t<decltype(b)>;
            if constexpr (std::is_same_v<A, B> &&
                          !std::is_same_v<A, std::monostate> &&
                          !std::is_same_v<A, std::vector<Value>> &&
                          !std::is_same_v<A, std::unordered_map<std::string, Value>>) {
                return a < b;
            }
            return false;
        }, static_cast<const ValueVariant&>(*this), static_cast<const ValueVariant&>(other));
    }
};
