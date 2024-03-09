#include <typeinfo>

// #TODO: Still missing some classes
// See: http://itanium-cxx-abi.github.io/cxx-abi/abi.html#rtti-layout

extern "C"
{
    /**
     * Invoked if a pure virtual function is ever called
     */
    void __cxa_pure_virtual()
    {
        // #TODO: Implement
        // infinite loop for now
        while (true) {}
    }
}

namespace __cxxabiv1
{
    class __fundamental_type_info: public std::type_info
    {
    public:
        /**
         * Destructor
         * 
         * NOTE: Intentionally NOT =default inside the definition because that
         * confuses the compiler since it will be looking for the first
         * virtual, non-inline function to designate as a "key function".
         */
        virtual ~__fundamental_type_info();
    };

    // Define OUTSIDE class so compiler can find the vtable
    __fundamental_type_info::~__fundamental_type_info() = default;

    /**
     * Base for both pointer types and pointer-to-member types
     */
    class __pbase_type_info: public std::type_info
    {
    public:
        unsigned int __flags;
        std::type_info const* __pointee;

        enum __masks
        {
            __const_mask = 0x1,
            __volatile_mask = 0x2,
            __restrict_mask = 0x4,
            __incomplete_mask = 0x8,
            __incomplete_class_mask = 0x10,
            __transaction_safe_mask = 0x20,
            __noexcept_mask = 0x40
        };

        /**
         * Destructor
         * 
         * NOTE: Intentionally NOT =default inside the definition because that
         * confuses the compiler since it will be looking for the first
         * virtual, non-inline function to designate as a "key function".
         */
        virtual ~__pbase_type_info();
    };

    // Define OUTSIDE class so compiler can find the vtable
    __pbase_type_info::~__pbase_type_info() = default;

    class __pointer_type_info: public __pbase_type_info
    {
    public:
        /**
         * Destructor
         * 
         * NOTE: Intentionally NOT =default inside the definition because that
         * confuses the compiler since it will be looking for the first
         * virtual, non-inline function to designate as a "key function".
         */
        virtual ~__pointer_type_info();
    };

    // Define OUTSIDE class so compiler can find the vtable
    __pointer_type_info::~__pointer_type_info() = default;

    /**
     * Classes having no bases, and as a base class for the other type_info types
     */
    class __class_type_info: public std::type_info
    {
    public:
        /**
         * Destructor
         * 
         * NOTE: Intentionally NOT =default inside the definition because that
         * confuses the compiler since it will be looking for the first
         * virtual, non-inline function to designate as a "key function".
         */
        virtual ~__class_type_info();
    };

    // Define OUTSIDE class so compiler can find the vtable
    __class_type_info::~__class_type_info() = default;

    /**
     * For classes with a single public non-virtual base at offset zero
     */
    class __si_class_type_info: public __class_type_info
    {
    public:
        __class_type_info const* __base_type;

        /**
         * Destructor
         * 
         * NOTE: Intentionally NOT =default inside the definition because that
         * confuses the compiler since it will be looking for the first
         * virtual, non-inline function to designate as a "key function".
         */
        virtual ~__si_class_type_info();
    };

    // Define OUTSIDE class so compiler can find the vtable
    __si_class_type_info::~__si_class_type_info() = default;
}

namespace abi = __cxxabiv1;