#ifndef ENTT_META_FACTORY_HPP
#define ENTT_META_FACTORY_HPP


#include <array>
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
    template<typename>
    struct CommonFuncHelper;

    template<typename Ret, typename... Args>
    struct CommonFuncHelper<Ret(Args...)> {
        using return_type = Ret;
        static constexpr auto size = sizeof...(Args);

        static auto arg(typename internal::MetaFuncNode::size_type index) ENTT_NOEXCEPT {
            return std::array<internal::MetaTypeNode *, sizeof...(Args)>{{internal::MetaInfo::resolve<Args>()...}}[index];
        }

        static auto accept(const internal::MetaTypeNode ** const types) ENTT_NOEXCEPT {
            std::array<internal::MetaTypeNode *, sizeof...(Args)> args{{internal::MetaInfo::resolve<Args>()...}};
            return std::equal(args.cbegin(), args.cend(), types);
        }
    };

    template<typename Class, typename Type, Type Class:: *>
    class MemberFuncHelper;

    template<typename Class, typename Ret, typename... Args, Ret(Class:: *Member)(Args...)>
    class MemberFuncHelper<Class, Ret(Args...), Member>: public CommonFuncHelper<Ret(Args...)> {
        template<std::size_t... Indexes>
        static auto execute(int, void *instance, const MetaAny *any, std::index_sequence<Indexes...>)
        -> decltype(MetaAny{(static_cast<Class *>(instance)->*Member)((any+Indexes)->to<std::decay_t<Args>>()...)}, MetaAny{})
        {
            return MetaAny{(static_cast<Class *>(instance)->*Member)((any+Indexes)->to<std::decay_t<Args>>()...)};
        }

        template<std::size_t... Indexes>
        static auto execute(char, void *instance, const MetaAny *any, std::index_sequence<Indexes...>) {
            (static_cast<Class *>(instance)->*Member)((any+Indexes)->to<std::decay_t<Args>>()...);
            return MetaAny{};
        }

        template<std::size_t... Indexes>
        static auto execute(char, const void *, const MetaAny *, std::index_sequence<Indexes...>) {
            assert(false);
            return MetaAny{};
        }

    public:
        static auto invoke(void *instance, const MetaAny *any) {
            return execute(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
        }

        static auto cinvoke(const void *instance, const MetaAny *any) {
            return execute(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
        }
    };

    template<typename Class, typename Ret, typename... Args, Ret(Class:: *Member)(Args...) const>
    class MemberFuncHelper<Class, Ret(Args...) const, Member>: public CommonFuncHelper<Ret(Args...)> {
        template<std::size_t... Indexes>
        static auto execute(int, const void *instance, const MetaAny *any, std::index_sequence<Indexes...>)
        -> decltype(MetaAny{(static_cast<const Class *>(instance)->*Member)((any+Indexes)->to<std::decay_t<Args>>()...)}, MetaAny{})
        {
            return MetaAny{(static_cast<const Class *>(instance)->*Member)((any+Indexes)->to<std::decay_t<Args>>()...)};
        }

        template<std::size_t... Indexes>
        static auto execute(char, const void *instance, const MetaAny *any, std::index_sequence<Indexes...>) {
            (static_cast<const Class *>(instance)->*Member)((any+Indexes)->to<std::decay_t<Args>>()...);
            return MetaAny{};
        }

    public:
        static auto invoke(void *instance, const MetaAny *any) {
            return execute(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
        }

        static auto cinvoke(const void *instance, const MetaAny *any) {
            return execute(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
        }
    };

    template<typename, typename Type, Type *>
    class FreeFuncHelper;

    template<typename Class, typename Ret, typename... Args, Ret(*Func)(Class &, Args...)>
    class FreeFuncHelper<Class, Ret(Class &, Args...), Func>: public CommonFuncHelper<Ret(Args...)> {
        template<std::size_t... Indexes>
        static auto execute(int, void *instance, const MetaAny *any, std::index_sequence<Indexes...>)
        -> decltype(MetaAny{(*Func)(*static_cast<Class *>(instance), (any+Indexes)->to<std::decay_t<Args>>()...)}, MetaAny{})
        {
            return MetaAny{(*Func)(*static_cast<Class *>(instance), (any+Indexes)->to<std::decay_t<Args>>()...)};
        }

        template<std::size_t... Indexes>
        static auto execute(char, void *instance, const MetaAny *any, std::index_sequence<Indexes...>) {
            (*Func)(*static_cast<Class *>(instance), (any+Indexes)->to<std::decay_t<Args>>()...);
            return MetaAny{};
        }

        template<std::size_t... Indexes>
        static auto execute(char, const void *, const MetaAny *, std::index_sequence<Indexes...>) {
            assert(false);
            return MetaAny{};
        }

    public:
        static auto invoke(void *instance, const MetaAny *any) {
            return execute(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
        }

        static auto cinvoke(const void *instance, const MetaAny *any) {
            return execute(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
        }
    };

    template<typename Class, typename Ret, typename... Args, Ret(*Func)(const Class &, Args...)>
    class FreeFuncHelper<Class, Ret(const Class &, Args...), Func>: public CommonFuncHelper<Ret(Args...)> {
        template<std::size_t... Indexes>
        static auto execute(int, const void *instance, const MetaAny *any, std::index_sequence<Indexes...>)
        -> decltype(MetaAny{(*Func)(*static_cast<const Class *>(instance), (any+Indexes)->to<std::decay_t<Args>>()...)}, MetaAny{})
        {
            return MetaAny{(*Func)(*static_cast<const Class *>(instance), (any+Indexes)->to<std::decay_t<Args>>()...)};
        }

        template<std::size_t... Indexes>
        static auto execute(char, const void *instance, const MetaAny *any, std::index_sequence<Indexes...>) {
            (*Func)(*static_cast<const Class *>(instance), (any+Indexes)->to<std::decay_t<Args>>()...);
            return MetaAny{};
        }

    public:
        static auto invoke(void *instance, const MetaAny *any) {
            return execute(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
        }

        static auto cinvoke(const void *instance, const MetaAny *any) {
            return execute(0, instance, any, std::make_index_sequence<sizeof...(Args)>{});
        }
    };

    template<typename Name, typename Node>
    inline static bool duplicate(const Name &name, const Node *node) ENTT_NOEXCEPT {
        return node ? node->name == name || duplicate(name, node->next) : false;
    }

    inline static bool duplicate(const MetaAny &key, const internal::MetaPropNode *node) ENTT_NOEXCEPT {
        return node ? node->key() == key || duplicate(key, node->next) : false;
    }

    template<typename Type>
    static void destroy(Type &instance) {
        instance.~Type();
    }

    template<typename...>
    static internal::MetaPropNode * properties() {
        return nullptr;
    }

    template<typename... Dispatch, typename Property, typename... Other>
    static internal::MetaPropNode * properties(const Property &property, const Other &... other) {
        static internal::MetaPropImpl<Dispatch..., Property, Other...> impl;
        static const MetaAny key{property.first};
        static const MetaAny value{property.second};
        static internal::MetaPropNode node{
            &impl,
            properties<Dispatch...>(other...),
            []() ENTT_NOEXCEPT -> const MetaAny & {
                return key;
            },
            []() ENTT_NOEXCEPT -> const MetaAny & {
                return value;
            }
        };

        assert(!duplicate(key, node.next));
        assert((!internal::MetaInfo::prop<Dispatch..., Property, Other...>));
        internal::MetaInfo::prop<Dispatch..., Property, Other...> = &node;
        return &node;
    }

    template<typename, typename = void>
    struct MetaFactory final {};

    template<typename Class>
    class MetaFactory<Class, std::enable_if_t<std::is_class<Class>::value>> {
        template<typename... Args, std::size_t... Indexes>
        static MetaAny constructor(const MetaAny * const any, std::index_sequence<Indexes...>) {
            return MetaAny{Class{(any+Indexes)->to<std::decay_t<Args>>()...}};
        }

        template<bool Const, typename Type, Type Class:: *Member>
        static std::enable_if_t<Const>
        setter(void *, const MetaAny &) {
            assert(false);
        }

        template<bool Const, typename Type, Type Class:: *Member>
        static std::enable_if_t<!Const>
        setter(void *instance, const MetaAny &any) {
            static_cast<Class *>(instance)->*Member = any.to<std::decay_t<Type>>();
        }

    public:
        template<typename... Args, typename... Property>
        static auto ctor(Property &&... property) ENTT_NOEXCEPT {
            auto *type = internal::MetaInfo::type<Class>;

            internal::MetaTypeNode *(* const arg)(typename internal::MetaCtorNode::size_type) ENTT_NOEXCEPT = [](typename internal::MetaCtorNode::size_type index) ENTT_NOEXCEPT {
                return std::array<internal::MetaTypeNode *, sizeof...(Args)>{{internal::MetaInfo::resolve<Args>()...}}[index];
            };

            bool(* const accept)(const internal::MetaTypeNode ** const) ENTT_NOEXCEPT = [](const internal::MetaTypeNode ** const types) ENTT_NOEXCEPT {
                std::array<internal::MetaTypeNode *, sizeof...(Args)> args{{internal::MetaInfo::resolve<Args>()...}};
                return std::equal(args.cbegin(), args.cend(), types);
            };

            MetaAny(* const invoke)(const MetaAny * const) = [](const MetaAny * const any) {
                return constructor<Args...>(any, std::make_index_sequence<sizeof...(Args)>{});
            };

            static internal::MetaCtorImpl<Class, Args...> impl;
            static internal::MetaCtorNode node{
                &impl,
                type->ctor,
                properties<Class, Args...>(std::forward<Property>(property)...),
                sizeof...(Args),
                arg, accept, invoke
            };

            assert((!internal::MetaInfo::ctor<Class, Args...>));
            internal::MetaInfo::ctor<Class, Args...> = &node;
            type->ctor = &node;
            return MetaFactory<Class>{};
        }

        template<void(*Func)(Class &), typename... Property>
        static auto dtor(Property &&... property) ENTT_NOEXCEPT {
            static internal::MetaDtorImpl<Class, std::integral_constant<void(*)(Class &), Func>> impl;
            static internal::MetaDtorNode node{
                &impl,
                properties<Class, std::integral_constant<void(*)(Class &), Func>>(std::forward<Property>(property)...),
                [](void *instance) {
                    (*Func)(*static_cast<Class *>(instance));
                }
            };

            assert((internal::MetaInfo::type<Class>->dtor == internal::MetaInfo::dtor<Class, std::integral_constant<void(*)(Class &), &destroy<Class>>>));
            assert((!internal::MetaInfo::dtor<Class, std::integral_constant<void(*)(Class &), Func>>));
            internal::MetaInfo::dtor<Class, std::integral_constant<void(*)(Class &), Func>> = &node;
            internal::MetaInfo::type<Class>->dtor = &node;
            return MetaFactory<Class>{};
        }

        template<typename Type, Type Class:: *Member, typename... Property>
        static auto data(const char *str, Property &&... property) ENTT_NOEXCEPT {
            static internal::MetaDataImpl<Class, std::integral_constant<Type Class:: *, Member>> impl;
            static internal::MetaDataNode node{
                &impl,
                HashedString{str},
                internal::MetaInfo::type<Class>->data,
                properties<Class, std::integral_constant<Type Class:: *, Member>>(std::forward<Property>(property)...),
                std::is_const<Type>::value,
                &internal::MetaInfo::resolve<Type>,
                &setter<std::is_const<Type>::value, Type, Member>,
                [](const void *instance) ENTT_NOEXCEPT {
                    return MetaAny{static_cast<const Class *>(instance)->*Member};
                },
                [](const internal::MetaTypeNode * const other) ENTT_NOEXCEPT {
                    return other == internal::MetaInfo::resolve<Type>();
                }
            };

            assert(!duplicate(HashedString{str}, internal::MetaInfo::type<Class>->data));
            assert((!internal::MetaInfo::data<Class, std::integral_constant<Type Class:: *, Member>>));
            internal::MetaInfo::data<Class, std::integral_constant<Type Class:: *, Member>> = &node;
            internal::MetaInfo::type<Class>->data = &node;
            return MetaFactory<Class>{};
        }

        template<typename Type, Type Class:: *Member, typename... Property>
        static auto func(const char *str, Property &&... property) ENTT_NOEXCEPT {
            using helper_type = MemberFuncHelper<Class, Type, Member>;
            static internal::MetaFuncImpl<Class, std::integral_constant<Type Class:: *, Member>> impl;
            static internal::MetaFuncNode node{
                &impl,
                HashedString{str},
                internal::MetaInfo::type<Class>->func,
                properties<Class, std::integral_constant<Type Class:: *, Member>>(std::forward<Property>(property)...),
                helper_type::size,
                &internal::MetaInfo::resolve<typename helper_type::return_type>,
                &helper_type::arg,
                &helper_type::accept,
                &helper_type::cinvoke,
                &helper_type::invoke
            };

            assert(!duplicate(HashedString{str}, internal::MetaInfo::type<Class>->func));
            assert((!internal::MetaInfo::func<Class, std::integral_constant<Type Class:: *, Member>>));
            internal::MetaInfo::func<Class, std::integral_constant<Type Class:: *, Member>> = &node;
            internal::MetaInfo::type<Class>->func = &node;
            return MetaFactory<Class>{};
        }

        template<typename Type, Type *Func, typename... Property>
        static auto func(const char *str, Property &&... property) ENTT_NOEXCEPT {
            using helper_type = FreeFuncHelper<Class, Type, Func>;
            static internal::MetaFuncImpl<Class, std::integral_constant<Type *, Func>> impl;
            static internal::MetaFuncNode node{
                &impl,
                HashedString{str},
                internal::MetaInfo::type<Class>->func,
                properties<Class, std::integral_constant<Type *, Func>>(std::forward<Property>(property)...),
                helper_type::size,
                &internal::MetaInfo::resolve<typename helper_type::return_type>,
                &helper_type::arg,
                &helper_type::accept,
                &helper_type::cinvoke,
                &helper_type::invoke
            };

            assert(!duplicate(HashedString{str}, internal::MetaInfo::type<Class>->func));
            assert((!internal::MetaInfo::func<Class, std::integral_constant<Type *, Func>>));
            internal::MetaInfo::func<Class, std::integral_constant<Type *, Func>> = &node;
            internal::MetaInfo::type<Class>->func = &node;
            return MetaFactory<Class>{};
        }
    };

    template<typename Type, typename... Property>
    static MetaFactory<Type> reflect(HashedString name, Property &&... property) ENTT_NOEXCEPT {
        static internal::MetaTypeImpl<Type> impl;
        static internal::MetaTypeNode node{
            &impl,
            name,
            internal::MetaInfo::type<>,
            properties<Type>(std::forward<Property>(property)...)
        };

        assert(!duplicate(name, internal::MetaInfo::type<>));
        assert(!internal::MetaInfo::type<Type>);
        internal::MetaInfo::type<Type> = &node;
        internal::MetaInfo::type<> = &node;
        return MetaFactory<Type>{}.template dtor<&destroy<Type>>();
    }

public:
    template<typename Type>
    using factory_type = MetaFactory<Type>;

    template<typename Type, typename... Property>
    inline static factory_type<Type> reflect(const char *str, Property &&... property) ENTT_NOEXCEPT {
        return reflect<Type>(HashedString{str}, std::forward<Property>(property)...);
    }

    template<typename Type>
    inline static MetaType * resolve() ENTT_NOEXCEPT {
        return internal::MetaInfo::resolve<Type>()->meta;
    }

    inline static MetaType * resolve(const char *str) ENTT_NOEXCEPT {
        return internal::Utils::meta(HashedString{str}, internal::MetaInfo::type<>);
    }
};


}


#endif // ENTT_META_FACTORY_HPP
