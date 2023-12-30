#ifndef KERNEL_POINTERTYPES_H
#define KERNEL_POINTERTYPES_H

#include <cstdint>
#include "Print.h"

/**
 * Wrapper class that represents a pointer to physical memory
 */
class PhysicalPtr
{
public:
    /**
     * Construct a physical pointer from a raw address
     * 
     * @param aAddress The physical address to wrap
     */
    explicit constexpr PhysicalPtr(uintptr_t const aAddress)
        : Address{ aAddress }
    {}

    /**
     * Obtains the address this pointer points at
     * 
     * @return The address pointed at
     */
    constexpr uintptr_t GetAddress() const 
    {
        return Address;
    }

private:
    uintptr_t Address = 0;
};

/**
 * Wrapper class that represents a pointer to virtual memory
 * (Normally just a regular pointer in code, but this is to help disambiguate in code that works with both)
 */
class VirtualPtr
{
public:
    /**
     * Construct a physical pointer from a raw address
     * 
     * @param aAddress The physical address to wrap
     */
    explicit constexpr VirtualPtr(uintptr_t const aAddress)
        : Address{ aAddress }
    {}

    /**
     * Obtains the address this pointer points at
     * 
     * @return The address pointed at
     */
    constexpr uintptr_t GetAddress() const 
    {
        return Address;
    }

private:
    uintptr_t Address = 0;
};

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