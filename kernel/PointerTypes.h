#ifndef KERNEL_POINTERTYPES_H
#define KERNEL_POINTERTYPES_H

#include <cstdint>
#include "Print.h"

namespace PointerWrapper
{
    namespace Detail
    {
        struct PhysicalTag {};
        struct VirtualTag {};

        template<class TagT>
        class WrapperT
        {
        public:
            /**
             * Constructs a null pointer
             */
            constexpr WrapperT() = default;

            /**
             * Construct a pointer from a raw address
             * 
             * @param aAddress The address to wrap
             */
            explicit constexpr WrapperT(uintptr_t const aAddress)
                : Address{ aAddress }
            {}

            /**
             * Compare this pointer with another
             * 
             * @param aOther Other pointer to compare with
             * @return True if equal
             */
            constexpr bool operator==(WrapperT const& aOther) const
            {
                return Address == aOther.Address;
            }

            /**
             * Compare this pointer with another
             * 
             * @param aOther Other pointer to compare with
             * @return True if not equal
             */
            constexpr bool operator!=(WrapperT const& aOther) const
            {
                return !(*this == aOther);
            }

            /**
             * Compare this pointer with another
             * 
             * @param aOther Other pointer to compare with
             * @return True if less than
             */
            constexpr bool operator<(WrapperT const& aOther) const
            {
                return Address < aOther.Address;
            }

            /**
             * Compare this pointer with another
             * 
             * @param aOther Other pointer to compare with
             * @return True if greater than
             */
            constexpr bool operator>(WrapperT const& aOther) const
            {
                return !(*this <= aOther);
            }

            /**
             * Compare this pointer with another
             * 
             * @param aOther Other pointer to compare with
             * @return True if less than or equal
             */
            constexpr bool operator<=(WrapperT const& aOther) const
            {
                return (*this < aOther) || (*this == aOther);
            }

            /**
             * Compare this pointer with another
             * 
             * @param aOther Other pointer to compare with
             * @return True if greater than or equal
             */
            constexpr bool operator>=(WrapperT const& aOther) const
            {
                return !(*this < aOther);
            }

            /**
             * Obtains the address this pointer points at
             * 
             * @return The address pointed at
             */
            constexpr uintptr_t GetAddress() const 
            {
                return Address;
            }

            /**
             * Obtains a pointer that is offset from this pointer
             * 
             * @param aOffset How much to offset this pointer by
             * @return The pointer at the given offset
             */
            constexpr WrapperT Offset(uintptr_t aOffset) const
            {
                // #TODO: Should probably panic if it would wrap the address
                // (or provide a version of offset that will)
                return WrapperT{ Address + aOffset };
            }

        private:
            uintptr_t Address = 0;
        };
    }
}

/**
 * Wrapper class that represents a pointer to physical memory
 */
using PhysicalPtr = PointerWrapper::Detail::WrapperT<PointerWrapper::Detail::PhysicalTag>;

/**
 * Wrapper class that represents a pointer to virtual memory
 * (Normally just a regular pointer in code, but this is to help disambiguate in code that works with both)
 */
using VirtualPtr = PointerWrapper::Detail::WrapperT<PointerWrapper::Detail::VirtualTag>;

namespace Print
{
    // #TODO: We're probably going to want to come up with a way to customize formatting output better, at least
    // without including the full Print header
    namespace Detail
    {
        template<>
        class DataWrapper<PhysicalPtr> final: public DataWrapperBase
        {
        public:
            /**
             * Wraps the specified data
             * 
             * @param aData Data to wrap
             */
            explicit DataWrapper(PhysicalPtr const aData): WrappedData{ aData } {}

        private:
            bool OutputDataImpl(char aFormat, OutputFunctorBase& arOutput) const override;

            PhysicalPtr WrappedData;
        };

        template<>
        class DataWrapper<VirtualPtr> final: public DataWrapperBase
        {
        public:
            /**
             * Wraps the specified data
             * 
             * @param aData Data to wrap
             */
            explicit DataWrapper(VirtualPtr const aData): WrappedData{ aData } {}

        private:
            bool OutputDataImpl(char aFormat, OutputFunctorBase& arOutput) const override;

            VirtualPtr WrappedData;
        };
    }
}

#endif // KERNEL_POINTERTYPES_H