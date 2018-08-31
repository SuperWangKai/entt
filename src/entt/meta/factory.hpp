#ifndef ENTT_META_FACTORY_HPP
#define ENTT_META_FACTORY_HPP


#include <array>
#include <tuple>
#include <cassert>
#include <cstddef>
#include <utility>
#include <algorithm>
#include <type_traits>
#include "../core/hashed_string.hpp"
#include "meta.hpp"


namespace entt {


template<typename Key, typename Value>
inline auto property(Key &&key, Value &&value) {
    return std::make_pair(key, value);
}


class Meta final {
    struct MetaChain;

    template<typename Type, typename = void>
    struct MetaFactory {
        template<typename Func, Func *Ptr, typename... Property>
        static auto ctor(Property &&... property) ENTT_NOEXCEPT {
            using helper_type = internal::ReflectionHelper<std::integral_constant<Func *, Ptr>>;
            static_assert(std::is_same<typename helper_type::return_type, Type>::value, "!");
            auto * const type = internal::MetaInfo<std::decay_t<Type>>::type;

            static internal::MetaCtorNode node{
                type->ctor,
                properties<Type, std::integral_constant<Func *, Ptr>>(std::forward<Property>(property)...),
                helper_type::size,
                &helper_type::arg,
                &helper_type::accept,
                [](const MetaAny * const any) {
                    return helper_type::invoke(0, nullptr, any, std::make_index_sequence<helper_type::size>{});
                },
                []() {
                    static MetaCtor meta{&node};
                    return &meta;
                }
            };

            assert((!internal::MetaInfo<std::decay_t<Type>>::template ctor<std::integral_constant<Func *, Ptr>>));
            internal::MetaInfo<std::decay_t<Type>>::template ctor<std::integral_constant<Func *, Ptr>> = &node;
            type->ctor = &node;
            return MetaFactory<Type>{};
        }

        template<typename... Args, typename... Property>
        static auto ctor(Property &&... property) ENTT_NOEXCEPT {
            using helper_type = internal::FunctionHelper<void(Args...)>;
            auto * const type = internal::MetaInfo<std::decay_t<Type>>::type;

            static internal::MetaCtorNode node{
                type->ctor,
                properties<Type, std::tuple<Args...>>(std::forward<Property>(property)...),
                helper_type::size,
                &helper_type::arg,
                &helper_type::accept,
                [](const MetaAny * const any) {
                    return internal::CtorHelper<Type, Args...>::construct(any, std::make_index_sequence<helper_type::size>{});
                },
                []() {
                    static MetaCtor meta{&node};
                    return &meta;
                }
            };

            assert((!internal::MetaInfo<std::decay_t<Type>>::template ctor<std::tuple<Args...>>));
            internal::MetaInfo<std::decay_t<Type>>::template ctor<std::tuple<Args...>> = &node;
            type->ctor = &node;
            return MetaFactory<Type>{};
        }

        template<void(*Func)(Type &), typename... Property>
        static auto dtor(Property &&... property) ENTT_NOEXCEPT {
            static internal::MetaDtorNode node{
                properties<Type, std::integral_constant<void(*)(Type &), Func>>(std::forward<Property>(property)...),
                [](void *instance) {
                    (*Func)(*static_cast<Type *>(instance));
                },
                []() {
                    static MetaDtor meta{&node};
                    return &meta;
                }
            };

            assert(!internal::MetaInfo<std::decay_t<Type>>::type->dtor);
            assert((!internal::MetaInfo<std::decay_t<Type>>::template dtor<std::integral_constant<void(*)(Type &), Func>>));
            internal::MetaInfo<std::decay_t<Type>>::template dtor<std::integral_constant<void(*)(Type &), Func>> = &node;
            internal::MetaInfo<std::decay_t<Type>>::type->dtor = &node;
            return MetaFactory<Type>{};
        }

        template<typename Data, Data *Ptr, typename... Property>
        static auto data(const char *str, Property &&... property) ENTT_NOEXCEPT {
            using helper_type = internal::ReflectionHelper<std::integral_constant<Data *, Ptr>>;
            auto * const type = internal::MetaInfo<std::decay_t<Type>>::type;

            static internal::MetaDataNode node{
                HashedString{str},
                type->data,
                properties<Type, std::integral_constant<Data *, Ptr>>(std::forward<Property>(property)...),
                helper_type::readonly,
                helper_type::shared,
                &internal::MetaInfo<std::decay_t<Data>>::resolve,
                &helper_type::setter,
                [](const void *) {
                    return MetaAny{*Ptr};
                },
                [](const internal::MetaTypeNode * const other) {
                    return other == internal::MetaInfo<std::decay_t<Data>>::resolve();
                },
                []() {
                    static MetaData meta{&node};
                    return &meta;
                }
            };

            assert(!duplicate(HashedString{str}, node.next));
            assert((!internal::MetaInfo<std::decay_t<Type>>::template data<std::integral_constant<Data *, Ptr>>));
            internal::MetaInfo<std::decay_t<Type>>::template data<std::integral_constant<Data *, Ptr>> = &node;
            type->data = &node;
            return MetaFactory<Type>{};
        }

        template<typename Func, Func *Ptr, typename... Property>
        static auto func(const char *str, Property &&... property) ENTT_NOEXCEPT {
            using helper_type = internal::ReflectionHelper<std::integral_constant<Func *, Ptr>>;
            auto * const type = internal::MetaInfo<std::decay_t<Type>>::type;

            static internal::MetaFuncNode node{
                HashedString{str},
                type->func,
                properties<Type, std::integral_constant<Func *, Ptr>>(std::forward<Property>(property)...),
                helper_type::size,
                helper_type::constant,
                helper_type::shared,
                &internal::MetaInfo<std::decay_t<typename helper_type::return_type>>::resolve,
                &helper_type::arg,
                &helper_type::accept,
                [](const void *instance, const MetaAny *any) {
                    return helper_type::invoke(0, instance, any, std::make_index_sequence<helper_type::size>{});
                },
                [](void *instance, const MetaAny *any) {
                    return helper_type::invoke(0, instance, any, std::make_index_sequence<helper_type::size>{});
                },
                []() {
                    static MetaFunc meta{&node};
                    return &meta;
                }
            };

            assert(!duplicate(HashedString{str}, node.next));
            assert((!internal::MetaInfo<std::decay_t<Type>>::template func<std::integral_constant<Func *, Ptr>>));
            internal::MetaInfo<std::decay_t<Type>>::template func<std::integral_constant<Func *, Ptr>> = &node;
            type->func = &node;
            return MetaFactory<Type>{};
        }
    };

    template<typename Class>
    struct MetaFactory<Class, std::enable_if_t<std::is_class<Class>::value>>: MetaFactory<Class, Class> {
        using MetaFactory<Class, Class>::func;

        template<typename Type, Type Class:: *Member, typename... Property>
        static std::enable_if_t<std::is_member_object_pointer<Type Class:: *>::value, MetaFactory<Class>>
        member(const char *str, Property &&... property) ENTT_NOEXCEPT {
            using helper_type = internal::ReflectionHelper<std::integral_constant<Type Class:: *, Member>>;
            auto * const type = internal::MetaInfo<std::decay_t<Class>>::type;

            static internal::MetaDataNode node{
                HashedString{str},
                type->data,
                properties<Class, std::integral_constant<Type Class:: *, Member>>(std::forward<Property>(property)...),
                helper_type::readonly,
                helper_type::shared,
                &internal::MetaInfo<std::decay_t<Type>>::resolve,
                &helper_type::setter,
                [](const void *instance) {
                    return MetaAny{static_cast<const Class *>(instance)->*Member};
                },
                [](const internal::MetaTypeNode * const other) {
                    return other == internal::MetaInfo<std::decay_t<Type>>::resolve();
                },
                []() {
                    static MetaData meta{&node};
                    return &meta;
                }
            };

            assert(!duplicate(HashedString{str}, node.next));
            assert((!internal::MetaInfo<std::decay_t<Class>>::template data<std::integral_constant<Type Class:: *, Member>>));
            internal::MetaInfo<std::decay_t<Class>>::template data<std::integral_constant<Type Class:: *, Member>> = &node;
            type->data = &node;
            return MetaFactory<Class>{};
        }

        template<typename Type, Type Class:: *Member, typename... Property>
        static std::enable_if_t<std::is_member_function_pointer<Type Class:: *>::value, MetaFactory<Class>>
        member(const char *str, Property &&... property) ENTT_NOEXCEPT {
            using helper_type = internal::ReflectionHelper<std::integral_constant<Type Class:: *, Member>>;
            auto * const type = internal::MetaInfo<std::decay_t<Class>>::type;

            static internal::MetaFuncNode node{
                HashedString{str},
                type->func,
                properties<Class, std::integral_constant<Type Class:: *, Member>>(std::forward<Property>(property)...),
                helper_type::size,
                helper_type::constant,
                helper_type::shared,
                &internal::MetaInfo<std::decay_t<typename helper_type::return_type>>::resolve,
                &helper_type::arg,
                &helper_type::accept,
                [](const void *instance, const MetaAny *any) {
                    return helper_type::invoke(0, instance, any, std::make_index_sequence<helper_type::size>{});
                },
                [](void *instance, const MetaAny *any) {
                    return helper_type::invoke(0, instance, any, std::make_index_sequence<helper_type::size>{});
                },
                []() {
                    static MetaFunc meta{&node};
                    return &meta;
                }
            };

            assert(!duplicate(HashedString{str}, node.next));
            assert((!internal::MetaInfo<std::decay_t<Class>>::template func<std::integral_constant<Type Class:: *, Member>>));
            internal::MetaInfo<std::decay_t<Class>>::template func<std::integral_constant<Type Class:: *, Member>> = &node;
            type->func = &node;
            return MetaFactory<Class>{};
        }
    };

    template<typename Name, typename Node>
    inline static bool duplicate(const Name &name, const Node *node) ENTT_NOEXCEPT {
        return node ? node->name == name || duplicate(name, node->next) : false;
    }

    inline static bool duplicate(const MetaAny &key, const internal::MetaPropNode *node) ENTT_NOEXCEPT {
        return node ? node->key() == key || duplicate(key, node->next) : false;
    }

    template<typename...>
    static internal::MetaPropNode * properties() {
        return nullptr;
    }

    template<typename Type, typename Owner, typename Property, typename... Other>
    static internal::MetaPropNode * properties(const Property &property, const Other &... other) {
        static const MetaAny key{property.first};
        static const MetaAny value{property.second};

        static internal::MetaPropNode node{
            properties<Type, Owner>(other...),
            []() -> const MetaAny & {
                return key;
            },
            []() -> const MetaAny & {
                return value;
            },
            []() {
                static MetaProp meta{&node};
                return &meta;
            }
        };

        assert(!duplicate(key, node.next));
        assert((!internal::MetaInfo<std::decay_t<Type>>::template prop<Owner, Property, Other...>));
        internal::MetaInfo<std::decay_t<Type>>::template prop<Owner, Property, Other...> = &node;
        return &node;
    }

    template<typename Type, typename... Property>
    static MetaFactory<Type> reflect(HashedString name, Property &&... property) ENTT_NOEXCEPT {
        static internal::MetaTypeNode node{
            name,
            internal::MetaInfo<MetaChain>::type,
            properties<Type, Type>(std::forward<Property>(property)...),
            internal::TypeHelper<Type>::destroy,
            []() {
                static MetaType meta{&node};
                return &meta;
            }
        };

        assert(!duplicate(name, node.next));
        assert(!internal::MetaInfo<std::decay_t<Type>>::type);
        internal::MetaInfo<std::decay_t<Type>>::type = &node;
        internal::MetaInfo<MetaChain>::type = &node;
        return MetaFactory<Type>{};
    }

public:
    template<typename Type>
    using factory_type = MetaFactory<Type>;

    template<typename Type, typename... Property>
    inline static factory_type<std::decay_t<Type>> reflect(const char *str, Property &&... property) ENTT_NOEXCEPT {
        return reflect<std::decay_t<Type>>(HashedString{str}, std::forward<Property>(property)...);
    }

    template<typename Type>
    inline static MetaType * resolve() ENTT_NOEXCEPT {
        return internal::MetaInfo<std::decay_t<Type>>::resolve()->meta();
    }

    inline static MetaType * resolve(const char *str) ENTT_NOEXCEPT {
        return internal::Utils::meta(HashedString{str}, internal::MetaInfo<MetaChain>::type);
    }
};


}


#endif // ENTT_META_FACTORY_HPP
