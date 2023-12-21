#include <string.h>

extern "C"
{
    void* memcpy(void* apDest, const void* apSource, size_t aCount)
    {
        auto const pdestBytes = reinterpret_cast<char*>(apDest);
        auto const psourceBytes = reinterpret_cast<char const*>(apSource);
        for (auto curByte = 0u; curByte < aCount; ++curByte)
        {
            pdestBytes[curByte] = psourceBytes[curByte];
        }
        return apDest;
    }

    void* memset(void* const apDest, int const aChar, size_t const aCount)
    {
        auto const pdestBytes = reinterpret_cast<char*>(apDest);
        for (auto curItem = static_cast<size_t>(0u); curItem < aCount; ++curItem)
        {
            pdestBytes[curItem] = static_cast<char>(aChar);
        }
        return apDest;
    }

    int strcmp(char const* const apLHS, char const* const apRHS)
    {
        auto retVal = 0;
        auto pcurLHS = apLHS;
        auto pcurRHS = apRHS;

        auto endOfString = [](char const* const apStrPos) {return *apStrPos == '\0';};

        while (!endOfString(pcurLHS) && !endOfString(pcurRHS) && (retVal == 0))
        {
            retVal = (static_cast<int>(*pcurLHS) - static_cast<int>(*pcurRHS));
            ++pcurLHS;
            ++pcurRHS;
        }
        if ((retVal == 0) && (endOfString(pcurLHS) != endOfString(pcurRHS)))
        {
            retVal = endOfString(pcurLHS) ? -1 : 1;
        }
        return retVal;
    }

    size_t strlen(char const* const apStr)
    {
        auto pend = apStr;
        for (; *pend != 0; ++pend) {}
        return static_cast<size_t>(pend - apStr);
    }
}