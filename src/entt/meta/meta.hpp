#ifndef ENTT_META_META_HPP
#define ENTT_META_META_HPP


#include <array>
#include <memory>
#include <cassert>
#include <cstddef>
#include <utility>
#include <algorithm>
#include <type_traits>
#include "../core/hashed_string.hpp"


namespace entt {


struct MetaAny;
class MetaProp;
class MetaCtor;
class MetaDtor;
class MetaData;
class MetaFunc;
class MetaType;


namespace internal {


struct MetaPropNode;
struct MetaCtorNode;
struct MetaDtorNode;
struct MetaDataNode;
struct MetaFuncNode;
struct MetaTypeNode;


struct MetaInfo {
    template<typename...>
    static MetaTypeNode *type;

    template<typename...>
    static MetaCtorNode *ctor;

    template<typename...>
    static MetaDtorNode *dtor;

    template<typename...>
    static MetaDataNode *data;

    template<typename...>
    static MetaFuncNode *func;

    template<typename...>
    static MetaPropNode *prop;

    template<typename Type>
    static internal::MetaTypeNode * resolve() ENTT_NOEXCEPT;
};


template<typename...>
MetaTypeNode * MetaInfo::type = nullptr;


template<typename...>
MetaCtorNode * MetaInfo::ctor = nullptr;


template<typename...>
MetaDtorNode * MetaInfo::dtor = nullptr;


template<typename...>
MetaDataNode * MetaInfo::data = nullptr;


template<typename...>
MetaFuncNode * MetaInfo::func = nullptr;


template<typename...>
MetaPropNode * MetaInfo::prop = nullptr;


struct MetaPropNode final {
    MetaPropNode * const next;
    const MetaAny &(* const key)();
    const MetaAny &(* const value)();
    MetaProp *(* const meta)();
};


struct MetaCtorNode final {
    using size_type = std::size_t;
    MetaCtorNode * const next;
    MetaPropNode * const prop;
    const size_type size;
    MetaTypeNode *(* const arg)(size_type);
    bool(* const accept)(const MetaTypeNode ** const);
    MetaAny(* const invoke)(const MetaAny * const);
    MetaCtor *(* const meta)();
};


struct MetaDtorNode final {
    MetaPropNode * const prop;
    void(* const invoke)(void *);
    MetaDtor *(* const meta)();
};


struct MetaDataNode final {
    const HashedString name;
    MetaDataNode * const next;
    MetaPropNode * const prop;
    const bool readonly;
    const bool shared;
    MetaTypeNode *(* const type)();
    void(* const set)(void *, const MetaAny &);
    MetaAny(* const get)(const void *);
    bool(* const accept)(const MetaTypeNode * const);
    MetaData *(* const meta)();
};


struct MetaFuncNode final {
    using size_type = std::size_t;
    const HashedString name;
    MetaFuncNode * const next;
    MetaPropNode * const prop;
    const size_type size;
    const bool constant;
    const bool shared;
    MetaTypeNode *(* const ret)();
    MetaTypeNode *(* const arg)(size_type);
    bool(* const accept)(const MetaTypeNode ** const);
    MetaAny(* const cinvoke)(const void *, const MetaAny *);
    MetaAny(* const invoke)(void *, const MetaAny *);
    MetaFunc *(* const meta)();
};


struct MetaTypeNode final {
    const HashedString name;
    MetaTypeNode * const next;
    MetaPropNode * const prop;
    void(* const destroy)(void *);
    MetaType *(* const meta)();
    MetaCtorNode *ctor;
    MetaDtorNode *dtor;
    MetaDataNode *data;
    MetaFuncNode *func;
};


struct Holder {
    virtual ~Holder() = default;

    virtual MetaTypeNode * node() const ENTT_NOEXCEPT = 0;
    virtual const void * data() const ENTT_NOEXCEPT = 0;
    virtual bool operator==(const Holder &) const ENTT_NOEXCEPT = 0;

    inline void * data() ENTT_NOEXCEPT {
        return const_cast<void *>(const_cast<const Holder *>(this)->data());
    }
};


template<typename Type>
struct HolderType: public Holder {
    template<typename Object>
    static auto compare(int, const Object &lhs, const Object &rhs)
    -> decltype(lhs == rhs, bool{})
    {
        return lhs == rhs;
    }

    template<typename Object>
    static bool compare(char, const Object &, const Object &) {
        return false;
    }

public:
    template<typename... Args>
    HolderType(Args &&... args)
        : storage{}
    {
        new (&storage) Type{std::forward<Args>(args)...};
    }

    ~HolderType() {
        reinterpret_cast<Type *>(&storage)->~Type();
    }

    MetaTypeNode * node() const ENTT_NOEXCEPT override {
        return MetaInfo::resolve<Type>();
    }

    inline const void * data() const ENTT_NOEXCEPT override {
        return &storage;
    }

    bool operator==(const Holder &other) const ENTT_NOEXCEPT override {
        return node() == other.node() && compare(0, *reinterpret_cast<const Type *>(&storage), *static_cast<const Type *>(other.data()));
    }

private:
    typename std::aligned_storage_t<sizeof(Type), alignof(Type)> storage;
};


struct Utils {
    template<typename Meta, typename Op, typename Node>
    static auto iterate(Op op, const Node *curr) ENTT_NOEXCEPT
    -> decltype(op(std::declval<Meta *>()), void())
    {
        while(curr) {
            op(curr->meta());
            curr = curr->next;
        }
    }

    template<typename Key>
    static MetaProp * property(Key &&key, const MetaPropNode *curr) {
        MetaProp *prop = nullptr;

        iterate<MetaProp>([&prop, key = std::forward<Key>(key)](auto *curr) {
            prop = (curr->key().template convertible<Key>() && curr->key() == key) ? curr : prop;
        }, curr);

        return prop;
    }

    template<typename Node>
    static auto meta(HashedString name, const Node *curr) {
        while(curr && curr->name != name) {
            curr = curr->next;
        }

        return curr ? curr->meta() : nullptr;
    }
};


}


struct MetaAny final {
    MetaAny() ENTT_NOEXCEPT = default;

    template<typename Type>
    MetaAny(Type &&type)
        : actual{std::make_unique<internal::HolderType<std::decay_t<Type>>>(std::forward<Type>(type))}
    {}

    MetaAny(const MetaAny &) = delete;
    MetaAny(MetaAny &&) = default;

    MetaAny & operator=(const MetaAny &other) = delete;
    MetaAny & operator=(MetaAny &&) = default;

    inline bool valid() const ENTT_NOEXCEPT {
        return *this;
    }

    MetaType * meta() const ENTT_NOEXCEPT {
        return actual ? actual->node()->meta() : nullptr;
    }

    template<typename Type>
    bool convertible() const ENTT_NOEXCEPT {
        return internal::MetaInfo::resolve<std::decay_t<Type>>() == actual->node();
    }

    template<typename Type>
    inline const Type & to() const ENTT_NOEXCEPT {
        return *static_cast<const Type *>(actual->data());
    }

    template<typename Type>
    inline Type & to() ENTT_NOEXCEPT {
        return const_cast<Type &>(const_cast<const MetaAny *>(this)->to<Type>());
    }

    template<typename Type>
    inline const Type * data() const ENTT_NOEXCEPT {
        return actual ? static_cast<const Type *>(actual->data()) : nullptr;
    }

    template<typename Type>
    inline Type * data() ENTT_NOEXCEPT {
        return const_cast<Type *>(const_cast<const MetaAny *>(this)->data<Type>());
    }

    inline const void * data() const ENTT_NOEXCEPT {
        return *this;
    }

    inline void * data() ENTT_NOEXCEPT {
        return const_cast<void *>(const_cast<const MetaAny *>(this)->data());
    }

    inline operator const void *() const ENTT_NOEXCEPT {
        return actual ? actual->data() : nullptr;
    }

    inline operator void *() ENTT_NOEXCEPT {
        return const_cast<void *>(const_cast<const MetaAny *>(this)->data());
    }

    inline explicit operator bool() const ENTT_NOEXCEPT {
        return static_cast<bool>(actual);
    }

    inline bool operator==(const MetaAny &other) const ENTT_NOEXCEPT {
        return actual == other.actual || (meta() == other.meta() && actual && other.actual && *actual == *other.actual);
    }

private:
    std::unique_ptr<internal::Holder> actual;
};


class MetaProp {
    friend class Meta;

    MetaProp(internal::MetaPropNode * node)
        : node{node}
    {}

public:
    inline const MetaAny & key() const ENTT_NOEXCEPT {
        return node->key();
    }

    inline const MetaAny & value() const ENTT_NOEXCEPT {
        return node->value();
    }

private:
    internal::MetaPropNode *node;
};


class MetaCtor {
    friend class Meta;

    MetaCtor(internal::MetaCtorNode * node)
        : node{node}
    {}

public:
    using size_type = typename internal::MetaCtorNode::size_type;

    inline size_type size() const ENTT_NOEXCEPT {
        return node->size;
    }

    inline MetaType * arg(size_type index) const ENTT_NOEXCEPT {
        return index < size() ? node->arg(index)->meta() : nullptr;
    }

    template<typename... Args>
    bool accept() const ENTT_NOEXCEPT {
        std::array<const internal::MetaTypeNode *, sizeof...(Args)> args{{internal::MetaInfo::resolve<Args>()...}};
        return sizeof...(Args) == size() ? node->accept(args.data()) : false;
    }

    template<typename... Args>
    MetaAny invoke(Args &&... args) {
        std::array<const MetaAny, sizeof...(Args)> any{{std::forward<Args>(args)...}};
        return accept<Args...>() ? node->invoke(any.data()) : MetaAny{};
    }

    template<typename Op>
    inline void properties(Op op) const ENTT_NOEXCEPT {
        internal::Utils::iterate<MetaProp>(std::move(op), node->prop);
    }

    template<typename Key>
    inline MetaProp * property(Key &&key) const ENTT_NOEXCEPT {
        return internal::Utils::property(std::forward<Key>(key), node->prop);
    }

private:
    internal::MetaCtorNode *node;
};


class MetaDtor {
    friend class Meta;

    MetaDtor(internal::MetaDtorNode * node)
        : node{node}
    {}

public:
    inline void invoke(void *instance) {
        node->invoke(instance);
    }

    template<typename Op>
    inline void properties(Op op) const ENTT_NOEXCEPT {
        internal::Utils::iterate<MetaProp>(std::move(op), node->prop);
    }

    template<typename Key>
    inline MetaProp * property(Key &&key) const ENTT_NOEXCEPT {
        return internal::Utils::property(std::forward<Key>(key), node->prop);
    }

private:
    internal::MetaDtorNode *node;
};


class MetaData {
    friend class Meta;

    MetaData(internal::MetaDataNode * node)
        : node{node}
    {}

public:
    inline const char * name() const ENTT_NOEXCEPT {
        return node->name;
    }

    inline bool readonly() const ENTT_NOEXCEPT {
        return node->readonly;
    }

    inline bool shared() const ENTT_NOEXCEPT {
        return node->shared;
    }

    inline MetaType * type() const ENTT_NOEXCEPT {
        return node->type()->meta();
    }

    template<typename Arg>
    bool accept() const ENTT_NOEXCEPT {
        return node->accept(internal::MetaInfo::resolve<Arg>());
    }

    template<typename Arg>
    void set(void *instance, Arg &&arg) {
        return accept<Arg>() ? node->set(instance, MetaAny{std::forward<Arg>(arg)}) : void();
    }

    inline MetaAny get(const void *instance) const ENTT_NOEXCEPT {
        return node->get(instance);
    }

    template<typename Op>
    inline void properties(Op op) const ENTT_NOEXCEPT {
        internal::Utils::iterate<MetaProp>(std::move(op), node->prop);
    }

    template<typename Key>
    inline MetaProp * property(Key &&key) const ENTT_NOEXCEPT {
        return internal::Utils::property(std::forward<Key>(key), node->prop);
    }

private:
    internal::MetaDataNode *node;
};


class MetaFunc {
    friend class Meta;

    MetaFunc(internal::MetaFuncNode * node)
        : node{node}
    {}

public:
    using size_type = typename internal::MetaCtorNode::size_type;

    inline const char * name() const ENTT_NOEXCEPT {
        return node->name;
    }

    inline size_type size() const ENTT_NOEXCEPT {
        return node->size;
    }

    inline bool constant() const ENTT_NOEXCEPT {
        return node->constant;
    }

    inline bool shared() const ENTT_NOEXCEPT {
        return node->shared;
    }

    inline MetaType * ret() const ENTT_NOEXCEPT {
        return node->ret()->meta();
    }

    inline MetaType * arg(size_type index) const ENTT_NOEXCEPT {
        return index < size() ? node->arg(index)->meta() : nullptr;
    }

    template<typename... Args>
    bool accept() const ENTT_NOEXCEPT {
        std::array<const internal::MetaTypeNode *, sizeof...(Args)> args{{internal::MetaInfo::resolve<Args>()...}};
        return sizeof...(Args) == size() ? node->accept(args.data()) : false;
    }

    template<typename... Args>
    MetaAny invoke(const void *instance, Args &&... args) const {
        std::array<const MetaAny, sizeof...(Args)> any{{std::forward<Args>(args)...}};
        return accept<Args...>() ? node->cinvoke(instance, any.data()) : MetaAny{};
    }

    template<typename... Args>
    MetaAny invoke(void *instance, Args &&... args) {
        std::array<const MetaAny, sizeof...(Args)> any{{std::forward<Args>(args)...}};
        return accept<Args...>() ? node->invoke(instance, any.data()) : MetaAny{};
    }

    template<typename Op>
    inline void properties(Op op) const ENTT_NOEXCEPT {
        internal::Utils::iterate<MetaProp>(std::move(op), node->prop);
    }

    template<typename Key>
    inline MetaProp * property(Key &&key) const ENTT_NOEXCEPT {
        return internal::Utils::property(std::forward<Key>(key), node->prop);
    }

private:
    internal::MetaFuncNode *node;
};


class MetaType {
    friend class Meta;

    MetaType(internal::MetaTypeNode * node) ENTT_NOEXCEPT
        : node{node}
    {}

public:
    inline const char * name() const ENTT_NOEXCEPT {
        return node->name;
    }

    template<typename Op>
    inline void ctor(Op op) const ENTT_NOEXCEPT {
        internal::Utils::iterate<MetaCtor>(std::move(op), node->ctor);
    }

    template<typename... Args>
    inline MetaCtor * ctor() const ENTT_NOEXCEPT {
        MetaCtor *meta = nullptr;

        ctor([&meta](MetaCtor *curr) {
            meta = curr->accept<Args...>() ? curr : meta;
        });

        return meta;
    }

    template<typename Op>
    inline void dtor(Op op) const ENTT_NOEXCEPT {
        return node->dtor ? op(node->dtor->meta()) : void();
    }

    inline MetaDtor * dtor() const ENTT_NOEXCEPT {
        return node->dtor ? node->dtor->meta() : nullptr;
    }

    template<typename Op>
    inline void data(Op op) const ENTT_NOEXCEPT {
        internal::Utils::iterate<MetaData>(std::move(op), node->data);
    }

    inline MetaData * data(const char *str) const ENTT_NOEXCEPT {
        return internal::Utils::meta(HashedString{str}, node->data);
    }

    template<typename Op>
    inline void func(Op op) const ENTT_NOEXCEPT {
        internal::Utils::iterate<MetaFunc>(std::move(op), node->func);
    }

    inline MetaFunc * func(const char *str) const ENTT_NOEXCEPT {
        return internal::Utils::meta(HashedString{str}, node->func);
    }

    template<typename... Args>
    MetaAny construct(Args &&... args) const {
        auto *curr = node->ctor;

        while(curr && !curr->meta()->template accept<Args...>()) {
            curr = curr->next;
        }

        return curr ? curr->meta()->invoke(std::forward<Args>(args)...) : MetaAny{};
    }

    inline void destroy(void *instance) {
        return node->dtor ? node->dtor->invoke(instance) : node->destroy(instance);
    }

    template<typename Op>
    inline void properties(Op op) const ENTT_NOEXCEPT {
        internal::Utils::iterate<MetaProp>(std::move(op), node->prop);
    }

    template<typename Key>
    inline MetaProp * property(Key &&key) const ENTT_NOEXCEPT {
        return internal::Utils::property(std::forward<Key>(key), node->prop);
    }

private:
    internal::MetaTypeNode *node;
};


namespace internal {


template<typename Type>
struct TypeHelper {
    static void destroy(void *instance) {
        static_cast<Type *>(instance)->~Type();
    }
};


template<>
struct TypeHelper<void> {
     static void destroy(void *) {
        assert(false);
    }
};


template<typename>
struct FunctionHelper;


template<typename Ret, typename... Args>
struct FunctionHelper<Ret(Args...)> {
    using return_type = Ret;
    static constexpr auto size = sizeof...(Args);

    static auto arg(typename internal::MetaFuncNode::size_type index) {
        return std::array<internal::MetaTypeNode *, sizeof...(Args)>{{internal::MetaInfo::resolve<Args>()...}}[index];
    }

    static auto accept(const internal::MetaTypeNode ** const types) {
        std::array<internal::MetaTypeNode *, sizeof...(Args)> args{{internal::MetaInfo::resolve<Args>()...}};
        return std::equal(args.cbegin(), args.cend(), types);
    }
};


template<typename Type, typename... Args>
struct CtorHelper {
    template<std::size_t... Indexes>
    static MetaAny construct(const MetaAny * const any, std::index_sequence<Indexes...>) {
        return MetaAny{Type{(any+Indexes)->to<std::decay_t<Args>>()...}};
    }
};


template<typename, typename = void>
struct ReflectionHelper;


template<typename Class, typename Type, Type Class:: *Member>
struct ReflectionHelper<std::integral_constant<Type Class:: *, Member>, std::enable_if_t<std::is_member_object_pointer<Type Class:: *>::value>> {
    static constexpr auto readonly = false;
    static constexpr auto shared = false;

    static void setter(void *instance, const MetaAny &any) {
        static_cast<Class *>(instance)->*Member = any.to<std::decay_t<Type>>();
    }
};


template<typename Class, typename Type, const Type Class:: *Member>
struct ReflectionHelper<std::integral_constant<const Type Class:: *, Member>, std::enable_if_t<std::is_member_object_pointer<const Type Class:: *>::value>> {
    static constexpr auto readonly = true;
    static constexpr auto shared = false;

    static void setter(void *, const MetaAny &) {
        assert(false);
    }
};


template<typename Class, typename Ret, typename... Args, Ret(Class:: *Member)(Args...)>
struct ReflectionHelper<std::integral_constant<Ret(Class:: *)(Args...), Member>, std::enable_if_t<std::is_member_function_pointer<Ret(Class:: *)(Args...)>::value>>: FunctionHelper<Ret(Args...)> {
    static constexpr auto constant = false;
    static constexpr auto shared = false;

    template<std::size_t... Indexes>
    static auto invoke(int, void *instance, const MetaAny *any, std::index_sequence<Indexes...>)
    -> decltype(MetaAny{(static_cast<Class *>(instance)->*Member)((any+Indexes)->to<std::decay_t<Args>>()...)}, MetaAny{})
    {
        return MetaAny{(static_cast<Class *>(instance)->*Member)((any+Indexes)->to<std::decay_t<Args>>()...)};
    }

    template<std::size_t... Indexes>
    static auto invoke(char, void *instance, const MetaAny *any, std::index_sequence<Indexes...>) {
        (static_cast<Class *>(instance)->*Member)((any+Indexes)->to<std::decay_t<Args>>()...);
        return MetaAny{};
    }

    template<std::size_t... Indexes>
    static MetaAny invoke(char, const void *, const MetaAny *, std::index_sequence<Indexes...>) {
        assert(false);
        return MetaAny{};
    }
};


template<typename Class, typename Ret, typename... Args, Ret(Class:: *Member)(Args...) const>
struct ReflectionHelper<std::integral_constant<Ret(Class:: *)(Args...) const, Member>, std::enable_if_t<std::is_member_function_pointer<Ret(Class:: *)(Args...) const>::value>>: FunctionHelper<Ret(Args...)> {
    static constexpr auto constant = true;
    static constexpr auto shared = false;

    template<std::size_t... Indexes>
    static auto invoke(int, const void *instance, const MetaAny *any, std::index_sequence<Indexes...>)
    -> decltype(MetaAny{(static_cast<const Class *>(instance)->*Member)((any+Indexes)->to<std::decay_t<Args>>()...)}, MetaAny{})
    {
        return MetaAny{(static_cast<const Class *>(instance)->*Member)((any+Indexes)->to<std::decay_t<Args>>()...)};
    }

    template<std::size_t... Indexes>
    static auto invoke(char, const void *instance, const MetaAny *any, std::index_sequence<Indexes...>) {
        (static_cast<const Class *>(instance)->*Member)((any+Indexes)->to<std::decay_t<Args>>()...);
        return MetaAny{};
    }
};


template<typename Type, Type *Data>
struct ReflectionHelper<std::integral_constant<Type *, Data>, std::enable_if_t<!std::is_function<Type>::value>> {
    static constexpr auto readonly = false;
    static constexpr auto shared = true;

    static void setter(void *, const MetaAny &any) {
        *Data = any.to<std::decay_t<Type>>();
    }
};


template<typename Type, const Type *Data>
struct ReflectionHelper<std::integral_constant<const Type *, Data>, std::enable_if_t<!std::is_function<const Type>::value>> {
    static constexpr auto readonly = true;
    static constexpr auto shared = true;

    static void setter(void *, const MetaAny &) {
        assert(false);
    }
};


template<typename Ret, typename... Args, Ret(*Func)(Args...)>
struct ReflectionHelper<std::integral_constant<Ret(*)(Args...), Func>, std::enable_if_t<std::is_function<Ret(Args...)>::value>>: FunctionHelper<Ret(Args...)> {
    static constexpr auto constant = false;
    static constexpr auto shared = true;

    template<std::size_t... Indexes>
    static auto invoke(int, const void *, const MetaAny *any, std::index_sequence<Indexes...>)
    -> decltype(MetaAny{(*Func)((any+Indexes)->to<std::decay_t<Args>>()...)}, MetaAny{})
    {
        return MetaAny{(*Func)((any+Indexes)->to<std::decay_t<Args>>()...)};
    }

    template<std::size_t... Indexes>
    static auto invoke(char, const void *, const MetaAny *any, std::index_sequence<Indexes...>) {
        (*Func)((any+Indexes)->to<std::decay_t<Args>>()...);
        return MetaAny{};
    }
};


template<typename Type>
MetaTypeNode * MetaInfo::resolve() ENTT_NOEXCEPT {
    using actual_type = std::decay_t<Type>;

    if(!type<actual_type>) {
        static MetaTypeNode node{
            {},
            nullptr,
            nullptr,
            &TypeHelper<actual_type>::destroy,
            []() -> MetaType * {
                return nullptr;
            }
        };

        type<actual_type> = &node;
    }

    return type<actual_type>;
}


}


template<typename Key, typename Value>
inline auto property(Key &&key, Value &&value) {
    return std::make_pair(key, value);
}


class Meta final {
    template<typename Type, typename = void>
    struct MetaFactory {
        template<typename Func, Func *Ptr, typename... Property>
        static auto ctor(Property &&... property) ENTT_NOEXCEPT {
            using helper_type = internal::ReflectionHelper<std::integral_constant<Func *, Ptr>>;
            static_assert(std::is_same<typename helper_type::return_type, Type>::value, "!");
            auto * const type = internal::MetaInfo::type<Type>;

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

            assert((!internal::MetaInfo::ctor<Type, std::integral_constant<Func *, Ptr>>));
            internal::MetaInfo::ctor<Type, std::integral_constant<Func *, Ptr>> = &node;
            type->ctor = &node;
            return MetaFactory<Type>{};
        }

        template<typename... Args, typename... Property>
        static auto ctor(Property &&... property) ENTT_NOEXCEPT {
            using helper_type = internal::FunctionHelper<void(Args...)>;
            auto * const type = internal::MetaInfo::type<Type>;

            static internal::MetaCtorNode node{
                type->ctor,
                properties<Type, Args...>(std::forward<Property>(property)...),
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

            assert((!internal::MetaInfo::ctor<Type, Args...>));
            internal::MetaInfo::ctor<Type, Args...> = &node;
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

            assert(!internal::MetaInfo::type<Type>->dtor);
            assert((!internal::MetaInfo::dtor<Type, std::integral_constant<void(*)(Type &), Func>>));
            internal::MetaInfo::dtor<Type, std::integral_constant<void(*)(Type &), Func>> = &node;
            internal::MetaInfo::type<Type>->dtor = &node;
            return MetaFactory<Type>{};
        }

        template<typename Data, Data *Ptr, typename... Property>
        static auto data(const char *str, Property &&... property) ENTT_NOEXCEPT {
            using helper_type = internal::ReflectionHelper<std::integral_constant<Data *, Ptr>>;
            auto * const type = internal::MetaInfo::type<Type>;

            static internal::MetaDataNode node{
                HashedString{str},
                type->data,
                properties<Type, std::integral_constant<Data *, Ptr>>(std::forward<Property>(property)...),
                helper_type::readonly,
                helper_type::shared,
                &internal::MetaInfo::resolve<Data>,
                &helper_type::setter,
                [](const void *) {
                    return MetaAny{*Ptr};
                },
                [](const internal::MetaTypeNode * const other) {
                    return other == internal::MetaInfo::resolve<Data>();
                },
                []() {
                    static MetaData meta{&node};
                    return &meta;
                }
            };

            assert(!duplicate(HashedString{str}, node.next));
            assert((!internal::MetaInfo::data<Type, std::integral_constant<Data *, Ptr>>));
            internal::MetaInfo::data<Type, std::integral_constant<Data *, Ptr>> = &node;
            type->data = &node;
            return MetaFactory<Type>{};
        }

        template<typename Func, Func *Ptr, typename... Property>
        static auto func(const char *str, Property &&... property) ENTT_NOEXCEPT {
            using helper_type = internal::ReflectionHelper<std::integral_constant<Func *, Ptr>>;
            auto * const type = internal::MetaInfo::type<Type>;

            static internal::MetaFuncNode node{
                HashedString{str},
                type->func,
                properties<Type, std::integral_constant<Func *, Ptr>>(std::forward<Property>(property)...),
                helper_type::size,
                helper_type::constant,
                helper_type::shared,
                &internal::MetaInfo::resolve<typename helper_type::return_type>,
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
            assert((!internal::MetaInfo::func<Type, std::integral_constant<Func *, Ptr>>));
            internal::MetaInfo::func<Type, std::integral_constant<Func *, Ptr>> = &node;
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
            auto * const type = internal::MetaInfo::type<Class>;

            static internal::MetaDataNode node{
                HashedString{str},
                type->data,
                properties<Class, std::integral_constant<Type Class:: *, Member>>(std::forward<Property>(property)...),
                helper_type::readonly,
                helper_type::shared,
                &internal::MetaInfo::resolve<Type>,
                &helper_type::setter,
                [](const void *instance) {
                    return MetaAny{static_cast<const Class *>(instance)->*Member};
                },
                [](const internal::MetaTypeNode * const other) {
                    return other == internal::MetaInfo::resolve<Type>();
                },
                []() {
                    static MetaData meta{&node};
                    return &meta;
                }
            };

            assert(!duplicate(HashedString{str}, node.next));
            assert((!internal::MetaInfo::data<Class, std::integral_constant<Type Class:: *, Member>>));
            internal::MetaInfo::data<Class, std::integral_constant<Type Class:: *, Member>> = &node;
            type->data = &node;
            return MetaFactory<Class>{};
        }

        template<typename Type, Type Class:: *Member, typename... Property>
        static std::enable_if_t<std::is_member_function_pointer<Type Class:: *>::value, MetaFactory<Class>>
        member(const char *str, Property &&... property) ENTT_NOEXCEPT {
            using helper_type = internal::ReflectionHelper<std::integral_constant<Type Class:: *, Member>>;
            auto * const type = internal::MetaInfo::type<Class>;

            static internal::MetaFuncNode node{
                HashedString{str},
                type->func,
                properties<Class, std::integral_constant<Type Class:: *, Member>>(std::forward<Property>(property)...),
                helper_type::size,
                helper_type::constant,
                helper_type::shared,
                &internal::MetaInfo::resolve<typename helper_type::return_type>,
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
            assert((!internal::MetaInfo::func<Class, std::integral_constant<Type Class:: *, Member>>));
            internal::MetaInfo::func<Class, std::integral_constant<Type Class:: *, Member>> = &node;
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

    template<typename... Dispatch, typename Property, typename... Other>
    static internal::MetaPropNode * properties(const Property &property, const Other &... other) {
        static const MetaAny key{property.first};
        static const MetaAny value{property.second};

        static internal::MetaPropNode node{
            properties<Dispatch...>(other...),
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
        assert((!internal::MetaInfo::prop<Dispatch..., Property, Other...>));
        internal::MetaInfo::prop<Dispatch..., Property, Other...> = &node;
        return &node;
    }

    template<typename Type, typename... Property>
    static MetaFactory<Type> reflect(HashedString name, Property &&... property) ENTT_NOEXCEPT {
        static internal::MetaTypeNode node{
            name,
            internal::MetaInfo::type<>,
            properties<Type>(std::forward<Property>(property)...),
            internal::TypeHelper<Type>::destroy,
            []() {
                static MetaType meta{&node};
                return &meta;
            }
        };

        assert(!duplicate(name, node.next));
        assert(!internal::MetaInfo::type<Type>);
        internal::MetaInfo::type<Type> = &node;
        internal::MetaInfo::type<> = &node;
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
        return internal::MetaInfo::resolve<Type>()->meta();
    }

    inline static MetaType * resolve(const char *str) ENTT_NOEXCEPT {
        return internal::Utils::meta(HashedString{str}, internal::MetaInfo::type<>);
    }
};


}


#endif // ENTT_META_META_HPP
