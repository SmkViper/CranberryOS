#include <string.h>

int strcmp(const char* const apLHS, const char* const apRHS)
{
    auto retVal = 0;
    auto pcurLHS = apLHS;
    auto pcurRHS = apRHS;

    auto endOfString = [](const char* const apStrPos) {return *apStrPos == '\0';};

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