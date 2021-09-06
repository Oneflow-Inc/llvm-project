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

using oneflow::Maybe;

Maybe<int> g(const std::vector<int>& v) {
    for(int i = 0; i < v.size(); ++i) f(v.at(i));
    // CHECK-MESSAGES: :[[@LINE-1]]:43: warning: unsafe method `std::vector<int>::at` {{.*}}
    // CHECK-FIXES: JUST(oneflow::VectorAt(v, i))

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
    // CHECK-FIXES: JUST(oneflow::VectorAt(x, 10))
}

std::string xx(int);

Maybe<int> x2() {
    return xx(2333).at(10);
    // CHECK-MESSAGES: :[[@LINE-1]]:21: warning: unsafe method `std::basic_string<char>::at` {{.*}}
    // CHECK-FIXES: JUST(oneflow::VectorAt(xx(2333), 10))
}


void i() {
    h<float>({});
    h<double>({});
}
