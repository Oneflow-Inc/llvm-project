// RUN: %check_clang_tidy %s maybe-use-safe-methods %t

namespace std {
    using size_t = unsigned long;

    template <typename T>
    struct vector {
        size_t size() const;
        const T& at(size_t) const;
        T& at(size_t);
    };

    template <typename T>
    struct basic_string {
        const T& at(size_t) const;
    };

    using string = basic_string<char>;

    template <typename T>
    struct shared_ptr {
        T& operator*() const;
        T* operator->() const;
    };

    template <typename K, typename V>
    struct map {
        const V& at(const K&) const;
        V& at(const K&);
    };
}

namespace oneflow {

template <typename T>
struct Maybe {
    Maybe(T){}
};

template <>
struct Maybe<int> {
    Maybe(int){}
};

}

void f(int);
int rand();

using oneflow::Maybe;

Maybe<int> g(const std::vector<int>& v) {
    for(int i = 0; i < v.size(); ++i) f(v.at(i));
    // CHECK-MESSAGES: :[[@LINE-1]]:43: warning: unsafe method `std::vector<int>::at` {{.*}}
    // CHECK-FIXES: JUST(VectorAt(v, i))

    return Maybe<int>{1};
}


template <typename T>
Maybe<T> h(const std::vector<T>& v) {
    for(int i = 0; i < v.size(); ++i) f(v.at(i));
    // CHECK-MESSAGES: :[[@LINE-1]]:43: warning: unsafe method `std::vector<double>::at` {{.*}}
    // CHECK-MESSAGES: :[[@LINE-2]]:43: warning: unsafe method `std::vector<float>::at` {{.*}}

    return Maybe<T>{1};
}

Maybe<int> x1(const std::string& x) {
    return x.at(10);
    // CHECK-MESSAGES: :[[@LINE-1]]:14: warning: unsafe method `std::basic_string<char>::at` {{.*}}
    // CHECK-FIXES: JUST(VectorAt(x, 10))
}

std::string xx(int);

Maybe<int> x2() {
    return xx(2333).at(10);
    // CHECK-MESSAGES: :[[@LINE-1]]:21: warning: unsafe method `std::basic_string<char>::at` {{.*}}
    // CHECK-FIXES: JUST(VectorAt(xx(2333), 10))
}


void i() {
    h<float>({});
    h<double>({});
}

Maybe<char> yy(std::string* x) {
    return x->at(rand());
    // CHECK-MESSAGES: :[[@LINE-1]]:15: warning: unsafe method `std::basic_string<char>::at` {{.*}}
    // CHECK-FIXES: JUST(VectorAt(*x, rand()))
}

Maybe<char> zz(std::shared_ptr<std::string> x) {
    return x->at(rand());
    // CHECK-MESSAGES: :[[@LINE-1]]:15: warning: unsafe method `std::basic_string<char>::at` {{.*}}
    // CHECK-FIXES: JUST(VectorAt(*x, rand()))
}

template <typename T>
Maybe<T> t1(std::shared_ptr<std::vector<T>> x) {
    return x->at(rand());
    // CHECK-MESSAGES: :[[@LINE-1]]:15: warning: unsafe method `std::vector<{{.*}}>::at` {{.*}}
    // CHECK-FIXES: JUST(VectorAt(*x, rand()))
}

void i2() {
    t1<unsigned>({});
}

Maybe<int> z2(const std::map<int, int>& x) {
    return x.at(rand());
    // CHECK-MESSAGES: :[[@LINE-1]]:14: warning: unsafe method `std::map<int, int>::at` {{.*}}
    // CHECK-FIXES: JUST(MapAt(x, rand()))
}
