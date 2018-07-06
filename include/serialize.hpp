#pragma once


template<class ElemType>
inline static bool SerializeBase(uint8* buf, size_t size, size_t& offset, const ElemType& elem)
{
    size_t elem_size = sizeof(elem);
    if (elem_size + offset > size)
        return false;

    ElemType* pelem = (ElemType*)(buf + offset);
    *pelem = elem;
    offset += elem_size;
    return true;
}

template<class ElemType>
inline static bool SerializePB(uint8* buf, size_t size, size_t& offset, const ElemType& elem)
{
    size_t elem_size = elem.ByteSize();
    if (!SerializeBase(buf, size, offset, elem_size))
        return false;

    if (elem_size + offset > size)
        return false;

    if (!elem.SerializeWithCachedSizesToArray(buf + offset))
        return false;

    offset += elem_size;
    return true;
}

template<class ElemType>
inline static bool UnSerializeBase(const uint8* buf, size_t size, size_t& offset, ElemType& elem)
{
    size_t elem_size = sizeof(elem);
    if (elem_size + offset > size)
        return false;

    ElemType* pelem = (ElemType*)(buf + offset);
    elem = *pelem;
    offset += elem_size;
    return true;
}

template<class ElemType>
inline static bool UnSerializePB(const uint8 * buf, size_t size, size_t & offset, ElemType & elem)
{
    size_t elem_size = 0;
    if (!UnSerializeBase(buf, size, offset, elem_size))
        return false;

    if (elem_size + offset > size)
        return false;

    if (!elem.ParseFromArray(buf + offset, elem_size))
        return false;

    offset += elem_size;
    return true;
}

