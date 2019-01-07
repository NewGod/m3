#ifndef __HASHOBJECT_HH__
#define __HASHOBJECT_HH__

namespace m3
{

class HashObject
{
protected:
    unsigned long hashValue;
public:
    virtual ~HashObject() {};
    virtual void hash() = 0;
    inline unsigned long getHashValue()
    {
        return hashValue;
    }
    
    template <typename T>
    class Hash
    {
    public:
        unsigned long operator()(const T& x) const
        {
            return x.hashValue;
        }
    };

    template <typename T>
    class PointerHash
    {
    public:
        unsigned long operator()(T* const& px) const
        {
            return px->hashValue;
        }
    };
};

}

#endif
