#include <cstddef>
#include <unordered_map>
#include <functional>
#include <string>
#include <typeindex>

using StringView = std::string;
struct TensorTuple{};

template <typename ...T>
using HashMap = std::unordered_map<T...>;

template<typename> struct Optional {};
struct OpConfProto{};

namespace oneflow{

namespace user_op {

struct OpConf;

using ParamMap = HashMap<StringView, std::function<TensorTuple&(OpConf*)>>;
using AttrMap = HashMap<StringView, std::pair<std::function<void*(OpConf*)>, std::type_index>>;

struct ParamIter {
        auto name() const { return it->first; }
        decltype(auto) operator*() const { 
            return it->second(op);
        }
        auto operator++() { return ++it; }

        bool operator==(const ParamIter& other) const { 
            return op == other.op && it == other.it; 
        };
        bool operator!=(const ParamIter& other) const { return !operator==(other); }
    private:
        ParamMap::const_iterator it;
        OpConf* op;
};

struct ConstParamIter {
        auto name() const { return it->first; }
        decltype(auto) operator*() const { 
            return static_cast<const TensorTuple&>(it->second(const_cast<OpConf*>(op))); 
        }
        auto operator++() { return ++it; }

        bool operator==(const ConstParamIter& other) const { 
            return op == other.op && it == other.it; 
        };
        bool operator!=(const ConstParamIter& other) const { return !operator==(other); }
    private:
        ParamMap::const_iterator it;
        const OpConf *op;
};

struct AttrIter {
        auto name() const { return it->first; }
        template <typename T>
        decltype(auto) operator*() const {
            assert(it->second.second == typeid(T));
            return *reinterpret_cast<T*>(it->second.first(op)); 
        }
        auto operator++() { return ++it; }

        bool operator==(const AttrIter& other) const { 
            return op == other.op && it == other.it; 
        };
        bool operator!=(const AttrIter& other) const { return !operator==(other); }
    private:
        AttrMap::const_iterator it;
        OpConf *op;
};

struct ConstAttrIter {
        auto name() const { return it->first; }
        template <typename T>
        decltype(auto) operator*() const {
            assert(it->second.second == typeid(T));
            return *reinterpret_cast<const T*>(it->second.first(const_cast<OpConf*>(op)));
        }
        auto operator++() { return ++it; }

        bool operator==(const ConstAttrIter& other) const { 
            return op == other.op && it == other.it; 
        };
        bool operator!=(const ConstAttrIter& other) const { return !operator==(other); }
    private:
        AttrMap::const_iterator it;
        const OpConf *op;
};

struct OpConf {
    virtual ParamIter InBegin() = 0;
    virtual ParamIter InEnd() = 0;
    virtual ConstParamIter InBegin() const = 0;
    virtual ConstParamIter InEnd() const = 0;
    virtual Optional<TensorTuple&> In(StringView) = 0;
    virtual Optional<const TensorTuple&> In(StringView) const = 0;
    virtual size_t InSize() const = 0;
    
    virtual ParamIter OutBegin() = 0;
    virtual ParamIter OutEnd() = 0;
    virtual ConstParamIter OutBegin() const = 0;
    virtual ConstParamIter OutEnd() const = 0;
    virtual Optional<TensorTuple&> Out(StringView) = 0;
    virtual Optional<const TensorTuple&> Out(StringView) const = 0;
    virtual size_t OutSize() const = 0;
    
    virtual AttrIter AttrBegin() = 0;
    virtual AttrIter AttrEnd() = 0;
    virtual ConstAttrIter AttrBegin() const = 0;
    virtual ConstAttrIter AttrEnd() const = 0;
    virtual size_t AttrSize() const = 0;
    template <typename T>
    Optional<T&> Attr(StringView name) {
        auto [v, t] = AttrImpl(name);
        assert(t == typeid(T));

        return *reinterpret<T*>(v);
    }
    template <typename T>
    Optional<const T&> Attr(StringView name) const {
        auto [v, t] = AttrImpl(name);
        assert(t == typeid(T));

        return *reinterpret<const T*>(v);
    }
    
    template <typename T>
    static T FromProto(OpConfProto& proto) {
        T op_conf;
        for(auto it = op_conf.InBegin(); it != op_conf.InEnd(); ++it) {
            // ...
        }
        
        // ...
    }
    
    void ToProto(OpConfProto& proto) const {
        for(auto it = InBegin(); it != InEnd(); ++it) {
            // ...
        }
        
        // ...
    }
    
    virtual StringView Name() const = 0;
    virtual StringView Description() const = 0;
    
    // virtual const DeviceInferFn& DeviceInfer() = 0;
    // virtual const DataTypeInferFn& DataTypeInfer() = 0;
    
    // ...
    
protected:
    virtual std::pair<void*, std::type_index> AttrImpl(StringView) = 0;
    virtual std::pair<void*, std::type_index> AttrImpl(StringView) const = 0;
};

} // namespace user_op
} // namespace oneflow
