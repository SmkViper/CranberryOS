#include <typeinfo>

// TODO
// Implementing what is needed for the linker to be happy with virtual functions
// See: http://itanium-cxx-abi.github.io/cxx-abi/abi.html#rtti-layout

extern "C"
{
    /**
     * Invoked if a pure virtual function is ever called
     */
    void __cxa_pure_virtual()
    {
        // TODO
        // infinite loop for now
        while (true) {}
    }
}

namespace __cxxabiv1
{
    class __class_type_info: public std::type_info
    {
    public:
        /**
         * Constructor
         * 
         * @param __apName Mangled class name
         */
        explicit __class_type_info(const char* __apName)
            : type_info{__apName}
        {}

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

    class __si_class_type_info: public __class_type_info
    {
    public:
        const __class_type_info* __base_type;

        /**
         * Constructor
         * 
         * @param __apName Mangled class name
         * @param __apBase The base class this one inherits from
         */
        explicit __si_class_type_info(const char* __apName, const __class_type_info* __apBase)
            : __class_type_info{__apName}
            , __base_type{__apBase}
        {}

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