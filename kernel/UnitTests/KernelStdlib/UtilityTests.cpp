#include "UtilityTests.h"

#include <cstdint>
#include <utility>
#include "../Framework.h"

namespace UnitTests::KernelStdlib::Utility
{
    namespace
    {
        struct MoveCopyCountStruct
        {
            MoveCopyCountStruct() = default;
            MoveCopyCountStruct([[maybe_unused]] const MoveCopyCountStruct& aOther): CopyCount{1} {}
            MoveCopyCountStruct([[maybe_unused]] MoveCopyCountStruct&& amOther): MoveCount{1} {}
            MoveCopyCountStruct& operator=([[maybe_unused]] const MoveCopyCountStruct& aOther)
            {
                ++CopyCount;
                return *this;
            }
            MoveCopyCountStruct& operator=([[maybe_unused]] MoveCopyCountStruct&& amOther)
            {
                ++MoveCount;
                return *this;
            }

            uint32_t MoveCount = 0;
            uint32_t CopyCount = 0;
        };

        /**
         * Ensure std::move moves the type
         */
        void StdMoveTest()
        {
            MoveCopyCountStruct source;
            auto dest = MoveCopyCountStruct{std::move(source)};

            EmitTestResult(dest.MoveCount == 1, "std::move");
        }

        /**
         * Helper to get us a forwarding ref (requires templates - templated lamdas are in C++20)
         * 
         * @param aForwardingRef The forwarding ref to forward to the destination object
         * @param aResultFunctor The functor to call with the destination object
         */
        template<typename T, typename FunctorType>
        void StdForwardTestHelper(T&& aForwardingRef, const FunctorType& aResultFunctor)
        {
            auto dest = MoveCopyCountStruct{std::forward<T>(aForwardingRef)};
            aResultFunctor(dest);
        }

        /**
         * Ensure std::forward forwards the type
         */
        void StdForwardTest()
        {
            MoveCopyCountStruct lvalueSource;
            StdForwardTestHelper(lvalueSource, [](const MoveCopyCountStruct& aDest)
            {
                EmitTestResult(aDest.CopyCount == 1, "std::forward lvalue copy");
            });
            StdForwardTestHelper(std::move(lvalueSource), [](const MoveCopyCountStruct& aDest)
            {
                EmitTestResult(aDest.MoveCount == 1, "std::forward rvalue move");
            });
        }

        /**
         * Ensure that std::swap swaps values
         */
        void StdSwapTest()
        {
            int value1 = 1;
            int value2 = 2;
            std::swap(value1, value2);
            EmitTestResult((value1 == 2) && (value2 == 1), "std::swap basic types");

            MoveCopyCountStruct move1, move2;
            std::swap(move1, move2);
            EmitTestResult((move1.CopyCount == 0) && (move1.MoveCount != 0) && (move2.CopyCount == 0) && (move2.MoveCount != 0), "std::swap uses moves");
        }
    }

    void Run()
    {
        StdMoveTest();
        StdForwardTest();
        StdSwapTest();
    }
}