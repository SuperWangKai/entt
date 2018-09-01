#include <gtest/gtest.h>
#include <entt/core/hashed_string.hpp>
#include <entt/meta/factory.hpp>
#include <entt/meta/meta.hpp>

template<typename Type>
struct Helper {
    template<typename... Args>
    static Type ctor(Args... args) { return Type{args...}; }

    static void dtor(Type &v) {
        value = v;
    }

    static Type identity(Type v) {
        return v;
    }

    static Type value;
};

template<typename Type>
Type Helper<Type>::value{};

enum class Properties {
    boolProperty,
    intProperty
};

struct Meta: ::testing::Test {
    static void SetUpTestCase() {
        entt::Meta::reflect<char>("char", entt::property(Properties::boolProperty, false), entt::property(Properties::intProperty, 3))
                .ctor<>(entt::property(Properties::boolProperty, true))
                .ctor<char(char), &Helper<char>::ctor<char>>()
                .dtor<&Helper<char>::dtor>(entt::property(Properties::boolProperty, false))
                .data<char, &Helper<char>::value>("value")
                .func<char(char), &Helper<char>::identity>("identity");
    }
};

TEST_F(Meta, Fundamental) {
    ASSERT_EQ(entt::Meta::resolve<char>(), entt::Meta::resolve("char"));
    ASSERT_NE(entt::Meta::resolve<char>(), nullptr);
    ASSERT_NE(entt::Meta::resolve("char"), nullptr);

    auto *type = entt::Meta::resolve<char>();

    ASSERT_STREQ(type->name(), "char");

    ASSERT_NE(type->ctor<>(), nullptr);
    ASSERT_NE(type->ctor<char>(), nullptr);
    ASSERT_EQ(type->ctor<int>(), nullptr);

    type->ctor([](auto *meta) {
        ASSERT_TRUE(meta->template accept<>() || meta->template accept<char>());
    });

    ASSERT_NE(type->dtor(), nullptr);

    type->dtor([type](auto *meta) {
        ASSERT_EQ(type->dtor(), meta);
    });

    ASSERT_NE(type->data("value"), nullptr);
    ASSERT_EQ(type->data("eulav"), nullptr);

    type->data([](auto *meta) {
        ASSERT_STREQ(meta->name(), "value");
    });

    ASSERT_NE(type->func("identity"), nullptr);
    ASSERT_EQ(type->func("ytitnedi"), nullptr);

    type->func([](auto *meta) {
        ASSERT_STREQ(meta->name(), "identity");
    });

    auto any = type->construct();

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.convertible<char>());
    ASSERT_TRUE(any.convertible<const char>());
    ASSERT_TRUE(any.convertible<const char &>());
    ASSERT_EQ(any.type(), entt::Meta::resolve<char>());
    ASSERT_EQ(any.type(), entt::Meta::resolve<const char>());
    ASSERT_EQ(any.type(), entt::Meta::resolve<const char &>());
    ASSERT_NE(any.type(), nullptr);
    ASSERT_EQ(any.to<char>(), char{});

    any = type->construct('c');

    ASSERT_TRUE(any);
    ASSERT_TRUE(any.convertible<char>());
    ASSERT_TRUE(any.convertible<const char>());
    ASSERT_TRUE(any.convertible<const char &>());
    ASSERT_EQ(any.type(), entt::Meta::resolve<char>());
    ASSERT_EQ(any.type(), entt::Meta::resolve<const char>());
    ASSERT_EQ(any.type(), entt::Meta::resolve<const char &>());
    ASSERT_NE(any.type(), nullptr);
    ASSERT_EQ(any.to<char>(), 'c');

    ASSERT_EQ(Helper<char>::value, char{});

    type->destroy(any);

    ASSERT_EQ(Helper<char>::value, 'c');
}

TEST_F(Meta, Struct) {
    // TODO
}

TEST_F(Meta, MetaAny) {
    // TODO
}

TEST_F(Meta, MetaProp) {
    auto *type = entt::Meta::resolve<char>();

    ASSERT_NE(type->property(Properties::boolProperty), nullptr);
    ASSERT_NE(type->property(Properties::intProperty), nullptr);
    ASSERT_TRUE(type->property(Properties::boolProperty)->key().convertible<Properties>());
    ASSERT_TRUE(type->property(Properties::intProperty)->key().convertible<Properties>());
    ASSERT_EQ(type->property(Properties::boolProperty)->key().to<Properties>(), Properties::boolProperty);
    ASSERT_EQ(type->property(Properties::intProperty)->key().to<Properties>(), Properties::intProperty);
    ASSERT_TRUE(type->property(Properties::boolProperty)->value().convertible<bool>());
    ASSERT_TRUE(type->property(Properties::intProperty)->value().convertible<int>());
    ASSERT_FALSE(type->property(Properties::boolProperty)->value().to<bool>());
    ASSERT_EQ(type->property(Properties::intProperty)->value().to<int>(), 3);

    type->properties([](auto *prop) {
        ASSERT_TRUE(prop->key().template convertible<Properties>());

        if(prop->value().template convertible<bool>()) {
            ASSERT_FALSE(prop->value().template to<bool>());
        } else if(prop->value().template convertible<int>()) {
            ASSERT_EQ(prop->value().template to<int>(), 3);
        } else {
            FAIL();
        }
    });
}

TEST_F(Meta, MetaCtor) {
    auto *ctor = entt::Meta::resolve<char>()->ctor<char>();

    ASSERT_NE(ctor, nullptr);
    ASSERT_EQ(ctor->size(), typename entt::MetaCtor::size_type{1});
    ASSERT_EQ(ctor->arg({}), entt::Meta::resolve<char>());
    ASSERT_NE(ctor->arg({}), entt::Meta::resolve<int>());
    ASSERT_EQ(ctor->arg(typename entt::MetaCtor::size_type{1}), nullptr);
    ASSERT_FALSE(ctor->accept<>());
    ASSERT_FALSE(ctor->accept<int>());
    ASSERT_TRUE(ctor->accept<char>());
    ASSERT_TRUE(ctor->accept<const char>());
    ASSERT_TRUE(ctor->accept<const char &>());

    auto ok = ctor->invoke('c');
    auto ko = ctor->invoke(42);

    ASSERT_FALSE(ko);
    ASSERT_TRUE(ok);
    ASSERT_NE(ok, ko);
    ASSERT_FALSE(ko.convertible<char>());
    ASSERT_FALSE(ko.convertible<int>());
    ASSERT_TRUE(ok.convertible<char>());
    ASSERT_FALSE(ok.convertible<int>());
    ASSERT_EQ(ok.to<char>(), 'c');

    ctor = entt::Meta::resolve<char>()->ctor<>();

    ASSERT_NE(ctor->property(Properties::boolProperty), nullptr);
    ASSERT_TRUE(ctor->property(Properties::boolProperty)->key().convertible<Properties>());
    ASSERT_EQ(ctor->property(Properties::boolProperty)->key().to<Properties>(), Properties::boolProperty);
    ASSERT_TRUE(ctor->property(Properties::boolProperty)->value().convertible<bool>());
    ASSERT_TRUE(ctor->property(Properties::boolProperty)->value().to<bool>());
    ASSERT_EQ(ctor->property(Properties::intProperty), nullptr);

    ctor->properties([](auto *prop) {
        ASSERT_TRUE(prop->key().template convertible<Properties>());
        ASSERT_EQ(prop->key().template to<Properties>(), Properties::boolProperty);
        ASSERT_TRUE(prop->value().template convertible<bool>());
        ASSERT_TRUE(prop->value().template to<bool>());
    });
}

TEST_F(Meta, MetaDtor) {
    auto *dtor = entt::Meta::resolve<char>()->dtor();
    char c = '*';

    ASSERT_NE(Helper<char>::value, '*');

    dtor->invoke(&c);

    ASSERT_EQ(Helper<char>::value, '*');

    ASSERT_NE(dtor->property(Properties::boolProperty), nullptr);
    ASSERT_TRUE(dtor->property(Properties::boolProperty)->key().convertible<Properties>());
    ASSERT_EQ(dtor->property(Properties::boolProperty)->key().to<Properties>(), Properties::boolProperty);
    ASSERT_TRUE(dtor->property(Properties::boolProperty)->value().convertible<bool>());
    ASSERT_FALSE(dtor->property(Properties::boolProperty)->value().to<bool>());
    ASSERT_EQ(dtor->property(Properties::intProperty), nullptr);

    dtor->properties([](auto *prop) {
        ASSERT_TRUE(prop->key().template convertible<Properties>());
        ASSERT_EQ(prop->key().template to<Properties>(), Properties::boolProperty);
        ASSERT_TRUE(prop->value().template convertible<bool>());
        ASSERT_FALSE(prop->value().template to<bool>());
    });
}

TEST_F(Meta, MetaData) {
    // TODO
}

TEST_F(Meta, MetaFunc) {
    // TODO
}

TEST_F(Meta, MetaType) {
    // TODO
}

TEST_F(Meta, Properties) {
    // TODO
}

TEST_F(Meta, Types) {
    // TODO conversions, decays are set correctly all around, etc.
}

TEST_F(Meta, DefDestructor) {
    // TODO
}

// >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>OLD

/*
#include <iostream>

void print(unsigned int n, entt::MetaType *meta) {
    std::cout << std::string(n, ' ') << "class: " << (meta->name() ? meta->name() : "") << std::endl;

    meta->properties([n=n+2](auto *prop) {
        std::cout << std::string(n, ' ') << "prop: " << static_cast<const char *>(prop->key().template to<entt::HashedString>()) << "/" << prop->value().template to<int>() << std::endl;
    });

    meta->data([n=n+2](auto *data) {
        std::cout << std::string(n, ' ') << "data: " << (data->name() ? data->name() : "") << std::endl;

        data->properties([n=n+2](auto *prop) {
            std::cout << std::string(n, ' ') << "prop: " << static_cast<const char *>(prop->key().template to<entt::HashedString>()) << "/" << prop->value().template to<int>() << std::endl;
        });

        if(data->type()) {
            print(n, data->type());
        }
    });

    meta->func([n=n+2](auto *func) {
        std::cout << std::string(n, ' ') << "func: " << (func->name() ? func->name() : "") << std::endl;

        func->properties([n=n+2](auto *prop) {
            std::cout << std::string(n, ' ') << "prop: " << static_cast<const char *>(prop->key().template to<entt::HashedString>()) << "/" << prop->value().template to<int>() << std::endl;
        });

        if(func->ret()) {
            print(n+2, func->ret());
        }

        for(unsigned int i = 0; i < func->size(); ++i) {
            if(func->arg(i)) {
                print(n+2, func->arg(i));
            }
        }
    });
}


struct S {
    S() = default;
    S(int i, int j): i{i}, j{j} {}
    S(const S &other): i{other.i}, j{other.j} {}

    S & operator=(const S &other) {
        i = other.i;
        j = other.j;
        return *this;
    }

    void f() { std::cout << "... ??? !!!" << std::endl; }
    const S * g(int) const { return this; }
    S * g(int) { return this; }
    int h(int v) const noexcept { return v+1; }

    int i;
    int j;
    const int * const k{nullptr};
    const int z{2};

    static int sd;
    static const int csd;
};


int S::sd = 1;
const int S::csd = 1;


struct T {
    S s1;
    const S s2;
    void f(const S &) {}
};


struct A {
    int i;
    char c;
};


A construct(int i, char c) {
    std::cout << "construct A: " << i << "/" << c << std::endl;
    return A{i, c};
}


void destroy(A &a) {
    std::cout << "destroy A: " << a.i << "/" << a.c << std::endl;
    a.~A();
}


void serialize(const S *) {
    std::cout << "serializing S" << std::endl;
}


void serialize(T *) {
    std::cout << "serializing T" << std::endl;
}


TEST(Meta, TODO) {
    entt::Meta::reflect<S>("foo", entt::property(entt::HashedString{"x"}, 42), entt::property(entt::HashedString{"y"}, 100))
            .ctor<>()
            .ctor<int, int>()
            .ctor<const S &>()
            .member<int, &S::i>("i")
            .member<int, &S::j>("j")
            .member<const int * const, &S::k>("k")
            .member<const int, &S::z>("z")
            .member<void(), &S::f>("f")
            .member<S *(int), &S::g>("g")
            .member<const S *(int) const, &S::g>("cg")
            .member<int(int) const, &S::h>("h")
            .func<void(const S *), &serialize>("serialize", entt::property(entt::HashedString{"3"}, 3))
            .data<int, &S::sd>("sd")
            .data<const int, &S::csd>("csd")
            ;

    entt::Meta::reflect<T>("bar")
            .member<S, &T::s1>("s1")
            .member<const S, &T::s2>("s2")
            .member<void(const S &), &T::f>("f")
            .func<void(T *), &serialize>("serialize")
            ;

    ASSERT_NE(entt::Meta::resolve<S>(), nullptr);
    ASSERT_NE(entt::Meta::resolve<T>(), nullptr);
    ASSERT_EQ(entt::Meta::resolve<int>(), nullptr);

    auto *sMeta = entt::Meta::resolve<S>();
    auto *tMeta = entt::Meta::resolve("bar");

    print(0u, sMeta);
    print(0u, tMeta);

    ASSERT_EQ(sMeta->property(entt::HashedString{"x"})->value().to<int>(), 42);
    ASSERT_EQ(sMeta->property(entt::HashedString{"y"})->value().to<int>(), 100);

    ASSERT_STREQ(sMeta->name(), "foo");
    ASSERT_NE(sMeta, tMeta);

    S s{0, 0};
    sMeta->data("i")->set(&s, 3);
    sMeta->data("j")->set(&s, 42);

    ASSERT_EQ(sMeta->data("z")->get(&s).to<int>(), 2);
    ASSERT_EQ(sMeta->data("sd")->get(nullptr).to<int>(), 1);
    ASSERT_EQ(sMeta->data("csd")->get(nullptr).to<int>(), 1);

    sMeta->data("sd")->set(nullptr, 3);

    ASSERT_EQ(sMeta->data("sd")->get(nullptr).to<int>(), 3);

    ASSERT_EQ(s.i, 3);
    ASSERT_EQ(s.j, 42);
    ASSERT_EQ(sMeta->data("i")->get(&s).to<int>(), 3);
    ASSERT_EQ(sMeta->data("j")->get(&s).to<int>(), 42);

    sMeta->data([&s](auto *data) {
        if(!data->readonly()) {
            data->set(&s, 0);
        }
    });

    ASSERT_EQ(s.i, 0);
    ASSERT_EQ(s.j, 0);
    ASSERT_EQ(sMeta->data("i")->get(&s).to<int>(), 0);
    ASSERT_EQ(sMeta->data("j")->get(&s).to<int>(), 0);

    T t{S{0, 0}, S{0, 0}};

    auto *s1Data = tMeta->data("s1");
    auto *s1DataMeta = s1Data->type();
    auto instance = s1DataMeta->ctor<int, int>()->invoke(99, 100);

    ASSERT_EQ(instance.type(), entt::Meta::resolve<S>());
    ASSERT_TRUE(instance);

    tMeta->data("s1")->set(&t, instance.to<S>());
    s1DataMeta->destroy(instance);

    ASSERT_EQ(t.s1.i, 99);
    ASSERT_EQ(t.s1.j, 100);

    auto res = sMeta->func("h")->invoke(&s, 41);

    ASSERT_EQ(res.type(), entt::Meta::resolve<int>());
    ASSERT_EQ(res.to<int>(), 42);

    sMeta->func("serialize")->invoke(static_cast<const void *>(&s));
    tMeta->func("serialize")->invoke(&t);

    ASSERT_EQ(sMeta->func("serialize")->size(), 1);
    ASSERT_EQ(tMeta->func("serialize")->size(), 1);

    entt::Meta::reflect<A>("A")
            .ctor<A(int, char), &construct>()
            .dtor<&destroy>();

    auto any = entt::Meta::resolve<A>()->construct(42, 'c');

    ASSERT_EQ(any.to<A>().i, 42);
    ASSERT_EQ(any.to<A>().c, 'c');

    entt::Meta::resolve<A>()->dtor()->invoke(any);
    entt::Meta::resolve<A>()->destroy(entt::Meta::resolve<A>()->construct(42, 'c'));

    print(0, entt::Meta::resolve("A"));
}
*/
