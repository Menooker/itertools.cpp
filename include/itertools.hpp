#ifndef _ITERTOOLS_HPP_
#define _ITERTOOLS_HPP_
#include <vector>
#include <unordered_map>

namespace itertools {

template <typename T>
using decay_t = typename std::decay<T>::type;

template <typename T>
using select_iterator_for = typename std::conditional<
        std::is_const<typename std::remove_reference<T>::type>::value,
        typename decay_t<T>::const_iterator,
        typename decay_t<T>::iterator>::type;

template <typename Iter>
using select_access_type_for = typename std::conditional<
        std::is_same<Iter, typename std::vector<bool>::iterator>::value
                || std::is_same<Iter,
                        typename std::vector<bool>::const_iterator>::value,
        typename std::iterator_traits<Iter>::value_type,
        typename std::iterator_traits<Iter>::reference>::type;

template <typename Iter1, typename Iter2>
class zip_iterator {
public:
    using value_type = std::pair<select_access_type_for<Iter1>,
            select_access_type_for<Iter2>>;

    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using pointer = value_type *;
    using reference = value_type;

    zip_iterator() = delete;

    zip_iterator(Iter1 iter_1, Iter2 iter_2)
        : iter1_ {iter_1}, iter2_ {iter_2} {}

    zip_iterator &operator++() {
        ++iter1_;
        ++iter2_;
        return *this;
    }

    zip_iterator operator++(int) {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    bool operator!=(zip_iterator const &other) { return !(*this == other); }
    bool operator==(zip_iterator const &other) {
        return iter1_ == other.iter1_ || iter2_ == other.iter2_;
    }

    value_type operator*() { return value_type {*iter1_, *iter2_}; }

    Iter1 iter1_;
    Iter2 iter2_;
};

template <typename T, typename U>
class zipper {
public:
    using Iter1 = select_iterator_for<T>;
    using Iter2 = select_iterator_for<U>;

    using zip_type = zip_iterator<Iter1, Iter2>;

    template <typename V, typename W>
    explicit zipper(V &&a, W &&b)
        : lhs_(std::forward<V>(a)), rhs_(std::forward<W>(b)) {
        if (lhs_.size() != rhs_.size()) {
            throw std::runtime_error("zipper: size does not match");
        }
    }

    zip_type begin() { return zip_type {std::begin(lhs_), std::begin(rhs_)}; }
    zip_type end() { return zip_type {std::end(lhs_), std::end(rhs_)}; }

    template <typename TCopy = T, typename UCopy = U,
            typename enabled = typename std::enable_if<
                    !std::is_const<
                            typename std::remove_reference<TCopy>::type>::value
                    && !std::is_const<typename std::remove_reference<
                            UCopy>::type>::value>::type>
    zip_type erase(zip_type v) {
        return zip_type(lhs_.erase(v.iter1_), rhs_.erase(v.iter2_));
    }

    size_t size() const { return lhs_.size(); }
    T lhs_;
    U rhs_;
};

template <typename T, typename U>
zipper<T, U> zip(T &&t, U &&u) {
    return zipper<T, U> {std::forward<T>(t), std::forward<U>(u)};
}

template <typename T>
struct range_iterator {
    using value_type = T;
    using iterator_category = std::random_access_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using pointer = T *;
    using reference = T;
    T val_;
    range_iterator() = delete;

    range_iterator(T v) : val_(v) {}

    range_iterator &operator++() {
        ++val_;
        return *this;
    }

    range_iterator operator++(int) {
        auto tmp = *this;
        ++*this;
        return tmp;
    }

    bool operator!=(const range_iterator &other) { return !(*this == other); }
    bool operator==(const range_iterator &other) { return val_ == other.val_; }
    T operator*() { return val_; }
};

template <typename T>
class range_impl {
public:
    using const_iterator = range_iterator<T>;
    using iterator = range_iterator<T>;
    T begin_;
    T end_;

    range_impl(T beginv, T endv) : begin_(beginv), end_(endv) {}

    range_iterator<T> begin() { return range_iterator<T> {begin_}; }
    range_iterator<T> end() { return range_iterator<T> {end_}; }
    range_iterator<T> erase(range_iterator<T> v) { return ++v; }
    size_t size() const { return end_ - begin_; }
};

template <typename T>
range_impl<T> range(T begin, T end) {
    return range_impl<T> {begin, end};
}

template <typename T>
range_impl<T> range(T end) {
    return range_impl<T> {T {0}, end};
}

template <typename T>
struct container_traits {};

template <typename T>
struct container_traits<std::vector<T>> {
    template <typename T2>
    using mapped_t = std::vector<T2>;
    static void reserve(std::vector<T> &v, size_t sz) { v.reserve(sz); }
    static void insert_back(std::vector<T> &v, T &&val) {
        v.emplace_back(std::move(val));
    }
};

template <typename T1, typename T2>
struct container_traits<zipper<T1, T2>> {
    template <typename T3>
    using mapped_t = std::vector<T3>;
    static void reserve(zipper<T1, T2> &v, size_t sz) {
        container_traits<decay_t<T1>>::reserve(v.lhs_, sz);
        container_traits<decay_t<T2>>::reserve(v.rhs_, sz);
    }
    // static void insert_back(zipper<T1, T2> &v, T &&val) {

    //     container_traits<decay_t<T1>>::reserve(v.lhs_, sz);
    //     container_traits<decay_t<T2>>::reserve(v.rhs_, sz);
    // }
};

template <typename T>
struct container_traits<range_impl<T>> {
    template <typename T2>
    using mapped_t = range_impl<T2>;
    static void reserve(range_impl<T> &v, size_t sz) {}
    static void insert_back(range_impl<T> &v, T &&val) {}
};

template <typename R, typename... A>
R ret_type_helper(R (*)(A...));

template <typename C, typename R, typename... A>
R ret_type_helper(R (C::*)(A...));

template <typename T>
struct itertool_impl {
    using decayed = decay_t<T>;
    using the_traits = container_traits<decayed>;
    static constexpr bool is_const
            = std::is_const<typename std::remove_reference<T>::type>::value;

    T val_;
    template <typename TIn>
    itertool_impl(TIn &&v) : val_(std::forward<TIn>(v)) {}

    template <typename TF,
            typename OutContainer = typename the_traits::template mapped_t<
                    decay_t<decltype(std::declval<TF>()(*val_.begin()))>>>
    auto map(TF &&f) -> itertool_impl<OutContainer> {
        using ret_t = itertool_impl<OutContainer>;
        ret_t ret {OutContainer {}};
        container_traits<OutContainer>::reserve(ret.val_, val_.size());
        for (auto &&v : val_) {
            container_traits<OutContainer>::insert_back(ret.val_, f(v));
        }
        return ret;
    }

    template <typename TF>
    itertool_impl<decayed> filter(TF &&f) {
        itertool_impl<decayed> ret {decayed {}};
        for (auto &&v : val_) {
            if (f(v)) {
                the_traits::insert_back(ret.val_, decay_t<decltype(v)>(v));
            }
        }
        return ret;
    }

    template <typename TF, typename TCopy = T,
            typename enabled = typename std::enable_if<
                    !std::is_const<
                            typename std::remove_reference<TCopy>::type>::value,
                    void>::type>
    itertool_impl &map_inplace(TF &&f) {
        for (auto &&v : val_) {
            v = f(v);
        }
        return *this;
    }

    template <typename TF, typename TCopy = T,
            typename enabled = typename std::enable_if<
                    !std::is_const<
                            typename std::remove_reference<TCopy>::type>::value,
                    void>::type>
    itertool_impl &filter_inplace(TF &&f) {
        for (auto itr = val_.begin(); itr != val_.end();) {
            if (!f(*itr)) {
                itr = val_.erase(itr);
            } else {
                ++itr;
            }
        }
        return *this;
    }

    template <typename TF, typename TVal>
    TVal reduce(const TVal &initv, TF &&f) {
        TVal reducev = initv;
        for (auto &&v : val_) {
            reducev = f(reducev, v);
        }
        return reducev;
    }

    template <typename TF>
    auto find_if(TF &&f) -> decay_t<decltype(*val_.begin())> {
        for (const auto &v : val_) {
            if (f(v)) { return v; }
        }
        throw std::runtime_error("find_if: the element not found");
    }

    template <typename TCopy = T,
            typename enabled = typename std::enable_if<!std::is_const<
                    typename std::remove_reference<TCopy>::type>::value>::type>
    T &&get() {
        return std::move(val_);
    }

    template <typename OutT>
    OutT as() {
        return OutT {val_.begin(), val_.end()};
    }
};

template <typename T>
auto iter_on(T &&v) -> itertools::itertool_impl<decltype(std::forward<T>(v))> {
    return itertools::itertool_impl<decltype(std::forward<T>(v))>(
            std::forward<T>(v));
}

} // namespace itertool


#endif