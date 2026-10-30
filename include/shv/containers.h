#pragma once
#include "memory.h"
#include "crt.h"

namespace ScriptHook::Containers {
// Fallible containers for owner-thread, exception-free source ports. Mutations
// never yield; constructors/moves/destructors must not yield either.
template<class T>class Vector {
    static_assert(std::is_nothrow_move_constructible_v<T>,"Vector requires noexcept moves");
    static_assert(std::is_nothrow_destructible_v<T>,"Vector requires noexcept destruction");
    static_assert(alignof(T)<=4096,"Plugin heap alignment limit is 4096");
    T* data_{};size_t size_{},capacity_{};
    size_t Alias(const T* source)const noexcept{
        const auto first=reinterpret_cast<uintptr_t>(data_),at=reinterpret_cast<uintptr_t>(source);
        if(at>=first&&at-first<size_*sizeof(T)&&(at-first)%sizeof(T)==0)return (at-first)/sizeof(T);
        return SIZE_MAX;
    }
    bool Grow(size_t needed) noexcept{
        if(needed<=capacity_)return true;
        const size_t maximum=SIZE_MAX/sizeof(T);if(needed>maximum)return false;
        size_t next=capacity_>maximum/2?maximum:capacity_*2;
        if(next<8)next=maximum<8?maximum:8;
        if(next<needed)next=needed;
        return TryReserve(next);
    }
    template<class U>bool Push(U&& item) noexcept{
        if(size_==SIZE_MAX)return false;
        const size_t alias=Alias(std::addressof(item));if(!Grow(size_+1))return false;
        if(alias!=SIZE_MAX)::new(static_cast<void*>(data_+size_))T(std::forward<U>(data_[alias]));
        else ::new(static_cast<void*>(data_+size_))T(std::forward<U>(item));
        ++size_;return true;
    }
    template<class U>bool Insert(size_t index,U&& item) noexcept{
        if(index>size_||size_==SIZE_MAX)return false;
        const size_t alias=Alias(std::addressof(item));if(!Grow(size_+1))return false;
        T saved(alias!=SIZE_MAX?std::forward<U>(data_[alias]):std::forward<U>(item));
        for(size_t at=size_;at>index;--at){::new(static_cast<void*>(data_+at))T(std::move(data_[at-1]));data_[at-1].~T();}
        ::new(static_cast<void*>(data_+index))T(std::move(saved));++size_;return true;
    }
public:
    Vector() noexcept=default;
    Vector(const Vector&)=delete;Vector& operator=(const Vector&)=delete;
    Vector(Vector&& other) noexcept{Swap(other);}
    Vector& operator=(Vector&& other) noexcept{if(this!=&other){Reset();Swap(other);}return *this;}
    ~Vector() noexcept{Reset();}
    void Swap(Vector& other) noexcept{auto* data=data_;data_=other.data_;other.data_=data;auto n=size_;size_=other.size_;other.size_=n;n=capacity_;capacity_=other.capacity_;other.capacity_=n;}
    size_t Size()const noexcept{return size_;}
    size_t Capacity()const noexcept{return capacity_;}
    T* Data() noexcept{return data_;}const T* Data()const noexcept{return data_;}
    T* At(size_t i) noexcept{return i<size_?data_+i:nullptr;}const T* At(size_t i)const noexcept{return i<size_?data_+i:nullptr;}
    T* begin() noexcept{return data_;}T* end() noexcept{return data_?data_+size_:nullptr;}
    const T* begin()const noexcept{return data_;}const T* end()const noexcept{return data_?data_+size_:nullptr;}
    bool TryReserve(size_t n) noexcept{
        if(n<=capacity_)return true;
        if(n>SIZE_MAX/sizeof(T))return false;
        auto* next=static_cast<T*>(scriptAllocate(n*sizeof(T),alignof(T),0));if(!next)return false;
        for(size_t i=0;i<size_;++i){::new(static_cast<void*>(next+i))T(std::move(data_[i]));data_[i].~T();}
        if(data_)(void)scriptFree(data_);
        data_=next;capacity_=n;return true;
    }
    bool TryPushBack(const T& item) noexcept{static_assert(std::is_nothrow_copy_constructible_v<T>,"Copy requires noexcept construction");return Push<const T&>(item);}
    bool TryPushBack(T&& item) noexcept{return Push<T>(std::move(item));}
    bool TryInsert(size_t index,const T& item) noexcept{static_assert(std::is_nothrow_copy_constructible_v<T>,"Copy requires noexcept construction");return Insert<const T&>(index,item);}
    bool TryInsert(size_t index,T&& item) noexcept{return Insert<T>(index,std::move(item));}
    bool TryResize(size_t n) noexcept{
        static_assert(std::is_nothrow_default_constructible_v<T>,"Resize requires noexcept default construction");
        if(!Grow(n))return false;
        while(size_>n)data_[--size_].~T();
        while(size_<n){::new(static_cast<void*>(data_+size_))T();++size_;}
        return true;
    }
    bool Erase(size_t index) noexcept{
        if(index>=size_)return false;
        data_[index].~T();
        for(size_t i=index+1;i<size_;++i){::new(static_cast<void*>(data_+i-1))T(std::move(data_[i]));data_[i].~T();}
        --size_;return true;
    }
    bool TryCopyFrom(const Vector& other) noexcept{
        static_assert(std::is_nothrow_copy_constructible_v<T>,"Copy requires noexcept construction");
        if(this==&other)return true;
        Vector next;if(!next.TryReserve(other.size_))return false;
        for(const auto& item:other){::new(static_cast<void*>(next.data_+next.size_))T(item);++next.size_;}
        Swap(next);return true;
    }
    void Clear() noexcept{while(size_)data_[--size_].~T();}
    void Reset() noexcept{Clear();if(data_)(void)scriptFree(data_);data_=nullptr;capacity_=0;}
};

class String {
    Vector<char> value_;
    bool Source(const char* input,size_t bytes,size_t& offset)const noexcept{
        offset=SIZE_MAX;if(!input)return bytes==0;
        const auto first=reinterpret_cast<uintptr_t>(value_.Data()),at=reinterpret_cast<uintptr_t>(input);
        if(value_.Data()&&at>=first&&at-first<value_.Capacity()){
            offset=at-first;if(offset>value_.Size()||bytes>value_.Size()-offset)return false;
        }
        return true;
    }
public:
    String() noexcept=default;String(const String&)=delete;String& operator=(const String&)=delete;
    String(String&&) noexcept=default;String& operator=(String&&) noexcept=default;
    const char* CStr()const noexcept{return value_.Size()?value_.Data():"";}
    size_t Size()const noexcept{return value_.Size()?value_.Size()-1:0;}
    size_t Capacity()const noexcept{return value_.Capacity()?value_.Capacity()-1:0;}
    bool TryReserve(size_t n) noexcept{return n!=SIZE_MAX&&value_.TryReserve(n+1);}
    bool TryAssign(const char* input,size_t bytes) noexcept{
        size_t offset;if(!Source(input,bytes,offset)||bytes==SIZE_MAX)return false;
        if(!bytes){Clear();return true;}
        if(!TryReserve(bytes))return false;
        if(offset!=SIZE_MAX)input=value_.Data()+offset;
        const size_t count=bytes+1;
        if(count>value_.Size())(void)value_.TryResize(count);
        memmove(value_.Data(),input,bytes);
        if(count<value_.Size())(void)value_.TryResize(count);
        *value_.At(bytes)=0;return true;
    }
    bool TryAssign(const char* input) noexcept{return input&&TryAssign(input,strlen(input));}
    bool TryAppend(const char* input,size_t bytes) noexcept{
        size_t offset;if(!Source(input,bytes,offset))return false;
        if(!bytes)return true;
        const size_t previous=Size();if(bytes>SIZE_MAX-previous-1)return false;
        if(!TryReserve(previous+bytes))return false;
        if(offset!=SIZE_MAX)input=value_.Data()+offset;
        (void)value_.TryResize(previous+bytes+1);memmove(value_.Data()+previous,input,bytes);*value_.At(previous+bytes)=0;return true;
    }
    bool TryAppend(const char* input) noexcept{return input&&TryAppend(input,strlen(input));}
    bool TryCopyFrom(const String& other) noexcept{return TryAssign(other.CStr(),other.Size());}
    void Clear() noexcept{if(value_.Size()){(void)value_.TryResize(1);*value_.At(0)=0;}}
    void Reset() noexcept{value_.Reset();}
};

template<class K>struct Less {bool operator()(const K& a,const K& b)const noexcept(noexcept(a<b)){return a<b;}};
enum class InsertResult { Inserted, Exists, NoMemory };
// Sorted contiguous map: logarithmic lookup, linear insertion/erasure. Addresses
// can change on growth and shifts. Comparator must supply strict weak ordering.
template<class K,class V,class Compare=Less<K>>class FlatMap {
    struct Entry {K key;V value;};
    Vector<Entry> entries_;Compare less_{};
    static_assert(std::is_nothrow_default_constructible_v<Compare>,"Comparator initialization must be noexcept");
    static_assert(std::is_nothrow_invocable_r_v<bool,const Compare&,const K&,const K&>,"Comparator must be noexcept");
    size_t Lower(const K& key)const noexcept{
        size_t first=0,last=entries_.Size();while(first<last){const size_t mid=first+(last-first)/2;if(less_(entries_.At(mid)->key,key))first=mid+1;else last=mid;}return first;
    }
    bool Matches(size_t at,const K& key)const noexcept{return at<entries_.Size()&&!less_(key,entries_.At(at)->key);}
public:
    FlatMap() noexcept=default;
    size_t Size()const noexcept{return entries_.Size();}
    size_t Capacity()const noexcept{return entries_.Capacity();}
    bool TryReserve(size_t n) noexcept{return entries_.TryReserve(n);}
    const K* KeyAt(size_t i)const noexcept{auto* entry=entries_.At(i);return entry?std::addressof(entry->key):nullptr;}
    V* ValueAt(size_t i) noexcept{auto* entry=entries_.At(i);return entry?std::addressof(entry->value):nullptr;}
    const V* ValueAt(size_t i)const noexcept{auto* entry=entries_.At(i);return entry?std::addressof(entry->value):nullptr;}
    V* Find(const K& key) noexcept{const size_t at=Lower(key);return Matches(at,key)?std::addressof(entries_.At(at)->value):nullptr;}
    const V* Find(const K& key)const noexcept{const size_t at=Lower(key);return Matches(at,key)?std::addressof(entries_.At(at)->value):nullptr;}
    InsertResult TryInsert(const K& key,const V& value) noexcept{
        static_assert(std::is_nothrow_copy_constructible_v<K>&&std::is_nothrow_copy_constructible_v<V>,"Insertion requires noexcept copies");
        const size_t at=Lower(key);if(Matches(at,key))return InsertResult::Exists;
        Entry entry{key,value};return entries_.TryInsert(at,std::move(entry))?InsertResult::Inserted:InsertResult::NoMemory;
    }
    bool TrySet(const K& key,const V& value) noexcept{
        static_assert(std::is_nothrow_copy_constructible_v<V>,"Assignment requires noexcept copy construction");
        const size_t at=Lower(key);if(!Matches(at,key))return TryInsert(key,value)==InsertResult::Inserted;
        V saved(value);auto& target=entries_.At(at)->value;target.~V();::new(static_cast<void*>(std::addressof(target)))V(std::move(saved));return true;
    }
    bool Erase(const K& key) noexcept{const size_t at=Lower(key);return Matches(at,key)&&entries_.Erase(at);}
    bool TryCopyFrom(const FlatMap& other) noexcept{
        static_assert(std::is_empty_v<Compare>,"CopyFrom requires a stateless comparator");return entries_.TryCopyFrom(other.entries_);
    }
    void Clear() noexcept{entries_.Clear();}void Reset() noexcept{entries_.Reset();}
};
}
