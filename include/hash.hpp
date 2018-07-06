#pragma once

uint32 GetHash(const char* str, uint32 str_len)
{
    uint32 remain = str_len% sizeof(uint32);
    if (remain == 0)
        remain = sizeof(uint32);

    uint32 len = (str_len - remain)/ sizeof(uint32);
    uint32 index, i;
    uint32* tmp = (uint32*)str;
    index = 0xe6ee7103;
    index ^= *str << 7;
    for (i = 0; i < len; ++i)
    {
        index = (index* 0x3e11ef)^ tmp[i];
    }

    uint8* byte = (uint8*)&tmp[i];
    for (i = 0; i < remain; ++i)
    {
        index = (index* 0x3e11ef)^ *byte;
        byte++;
    }

    index ^= str_len;
    index ^= 0x1a8fed1f;

    return index;
}
