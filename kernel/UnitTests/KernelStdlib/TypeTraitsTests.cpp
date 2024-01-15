#include <cstdint>
#include <type_traits>

namespace UnitTests::KernelStdlib::TypeTraits
{
    namespace
    {
        ///////////////////////////////////////////////////////////////////////
        // std::integral_constant
        ///////////////////////////////////////////////////////////////////////

        static_assert(std::is_same_v<std::integral_constant<uint32_t, 10u>::value_type, uint32_t>, "Unexpected integral_constant::value_type");
        static_assert(std::is_same_v<std::integral_constant<uint32_t, 10u>::type, std::integral_constant<uint32_t, 10u>>, "Unexpected integral_constant::type");
        static_assert(std::integral_constant<uint32_t, 10u>::value == 10u, "Unexpected integral_constant::value");
        static_assert(static_cast<uint32_t>(std::integral_constant<uint32_t, 10u>{}) == 10u, "Unexpected integral_constant value via cast operation");
        static_assert(std::integral_constant<uint32_t, 10u>{}() == 10u, "Unexpected integral_constant value via invoke operation");

        ///////////////////////////////////////////////////////////////////////
        // std::bool_constant/true_type/false_type
        ///////////////////////////////////////////////////////////////////////

        static_assert(std::is_same_v<std::true_type::value_type, bool>, "Unexpected true_type::value_type");
        static_assert(std::true_type::value == true, "Unexpected true_type::value");
        static_assert(std::is_same_v<std::false_type::value_type, bool>, "Unexpected true_type::value_type");
        static_assert(std::false_type::value == false, "Unexpected false_type::value");

        ///////////////////////////////////////////////////////////////////////
        // std::is_same
        ///////////////////////////////////////////////////////////////////////

        static_assert(std::is_same_v<uint32_t, uint32_t>, "Unexpected result for is_same with same types");
        static_assert(!std::is_same_v<uint32_t, int32_t>, "Unexpected result for is_same with different types");

        ///////////////////////////////////////////////////////////////////////
        // std::is_const
        ///////////////////////////////////////////////////////////////////////

        static_assert(std::is_const_v<const int>, "Unexpected result for is_const with const type");
        static_assert(!std::is_const_v<int>, "Unexpected result for is_const with non-const type");
        static_assert(!std::is_const_v<const int*>, "Unexpected result for is_const with non-const type (with non-top-level const)");

        ///////////////////////////////////////////////////////////////////////
        // std::is_reference
        ///////////////////////////////////////////////////////////////////////

        static_assert(std::is_reference_v<int&>, "Unexpected result for is_reference with lvalue reference type");
        static_assert(std::is_reference_v<int&&>, "Unexpected result for is_reference with rvaule reference type");
        static_assert(!std::is_reference_v<int>, "Unexpected result for is_reference with non-reference type");

        ///////////////////////////////////////////////////////////////////////
        // std::is_lvalue_reference
        ///////////////////////////////////////////////////////////////////////

        static_assert(std::is_lvalue_reference_v<int&>, "Unexpected result for is_lvalue_reference with lvalue reference type");
        static_assert(!std::is_lvalue_reference_v<int&&>, "Unexpected result for is_lvalue_reference with rvaule reference type");
        static_assert(!std::is_lvalue_reference_v<int>, "Unexpected result for is_lvalue_reference with non-reference type");

        ///////////////////////////////////////////////////////////////////////
        // std::is_array
        ///////////////////////////////////////////////////////////////////////

        static_assert(std::is_array_v<int[]>, "Unexpected result for is_array with array type");
        static_assert(std::is_array_v<int[5]>, "Unexpected result for is_array with bounded array type");
        static_assert(!std::is_array_v<int*>, "Unexpected result for is_array with non-array type");

        ///////////////////////////////////////////////////////////////////////
        // std::is_function
        ///////////////////////////////////////////////////////////////////////

        struct DummyStruct
        {
            int MemberFunction() const;
        };

        // Function is only used for static_assert tests, so suppress the warning that it will not be emitted
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
        int DummyFunction();
#pragma clang diagnostic pop

        template<typename>
        struct PointerMemberTraits {};
        template<class T, class U>
        struct PointerMemberTraits<U T::*>
        {
            using member_type = U;
        };

        static_assert(std::is_function_v<decltype(DummyFunction)>, "Unexpected result for is_function with function");
        static_assert(std::is_function_v<int(int)>, "Unexpected result for is_function with function");
        static_assert(std::is_function_v<PointerMemberTraits<decltype(&DummyStruct::MemberFunction)>::member_type>, "Unexpected result for is_function with member function");
        static_assert(!std::is_function_v<DummyStruct>, "Unexpected result for is_function with struct");
        static_assert(!std::is_function_v<int>, "Unexpected result for is_function with base type");
        static_assert(!std::is_function_v<int&>, "Unexpected result for is_function with reference");
        static_assert(!std::is_function_v<int(*)(int)>, "Unexpected result for is_function with pointer to function");

        ///////////////////////////////////////////////////////////////////////
        // std::is_trivially_copyable
        ///////////////////////////////////////////////////////////////////////

        struct TrivialStruct
        {
            int x = 0;
            float y = 0.0f;
        };
        struct TrivialChild: public TrivialStruct
        {
            char z = 'a';
        };
        struct NonTrivialStruct
        {
            ~NonTrivialStruct() {}
            int x = 0;
            float y = 0.0f;
        };
        struct VirtualStruct
        {
            virtual ~VirtualStruct() = default;
            virtual int f() { return x; }
            int x = 0;
        };

        static_assert(std::is_trivially_copyable_v<int>, "Unexpected result from is_trivially_copyable with scalar type");
        static_assert(std::is_trivially_copyable_v<int*>, "Unexpected result from is_trivially_copyable with pointer type");
        static_assert(std::is_trivially_copyable_v<TrivialChild>, "Unexpected result from is_trivially_copyable with trivial struct");
        static_assert(!std::is_trivially_copyable_v<NonTrivialStruct>, "Unexpected result from is_trivially_copyable with non-trivial struct");
        static_assert(!std::is_trivially_copyable_v<VirtualStruct>, "Unexpected result from is_trivially_copyable with struct with virtual");

        ///////////////////////////////////////////////////////////////////////
        // std::remove_reference
        ///////////////////////////////////////////////////////////////////////

        static_assert(std::is_same_v<std::remove_reference_t<int*>, int*>, "Unexpected result from remove_reference with non-reference type");
        static_assert(std::is_same_v<std::remove_reference_t<int&>, int>, "Unexpected result from remove_reference with lvalue reference type");
        static_assert(std::is_same_v<std::remove_reference_t<int&&>, int>, "Unexpected result from remove_reference with rvalue reference type");

        ///////////////////////////////////////////////////////////////////////
        // std::remove_extent
        ///////////////////////////////////////////////////////////////////////

        static_assert(std::is_same_v<std::remove_extent_t<int*>, int*>, "Unexpected result from remove_extent with non-array type");
        static_assert(std::is_same_v<std::remove_extent_t<int[]>, int>, "Unexpected result from remove_extent with array type");
        static_assert(std::is_same_v<std::remove_extent_t<int[10]>, int>, "Unexpected result from remove_extent with sized array type");

        ///////////////////////////////////////////////////////////////////////
        // std::remove_cv
        ///////////////////////////////////////////////////////////////////////

        static_assert(std::is_same_v<std::remove_cv_t<int>, int>, "Unexpected result from remove_cv with non-const, non-volatile type");
        static_assert(std::is_same_v<std::remove_cv_t<const int>, int>, "Unexpected result from remove_cv with const, non-volatile type");
        static_assert(std::is_same_v<std::remove_cv_t<volatile int>, int>, "Unexpected result from remove_cv with non-const, volatile type");
        static_assert(std::is_same_v<std::remove_cv_t<const volatile int>, int>, "Unexpected result from remove_cv with const, volatile type");

        ///////////////////////////////////////////////////////////////////////
        // std::add_pointer
        ///////////////////////////////////////////////////////////////////////

        static_assert(std::is_same_v<std::add_pointer_t<int>, int*>, "Unexpected result from add_pointer with base type");
        static_assert(std::is_same_v<std::add_pointer_t<int*>, int**>, "Unexpected result from add_pointer with pointer type");
        static_assert(std::is_same_v<std::add_pointer_t<int&>, int*>, "Unexpected result from add_pointer with reference type");
        static_assert(std::is_same_v<std::add_pointer_t<int(int)>, int(*)(int)>, "Unexpected result from add_pointer with function type");
        static_assert(std::is_same_v<std::add_pointer_t<int(int) const>, int(int) const>, "Unexpected result from add_pointer with cv-qualified function type");

        ///////////////////////////////////////////////////////////////////////
        // std::conditional
        ///////////////////////////////////////////////////////////////////////

        static_assert(std::is_same_v<std::conditional_t<true, int32_t, int8_t>, int32_t>, "Unexpected result from true conditional");
        static_assert(std::is_same_v<std::conditional_t<false, int32_t, int8_t>, int8_t>, "Unexpected result from false conditional");

        ///////////////////////////////////////////////////////////////////////
        // std::decay
        ///////////////////////////////////////////////////////////////////////

        static_assert(std::is_same_v<std::decay_t<int>, int>, "Unexpected result from std::decay for base type");
        static_assert(std::is_same_v<std::decay_t<int&>, int>, "Unexpected result from std::decay for lvalue ref type");
        static_assert(std::is_same_v<std::decay_t<int&&>, int>, "Unexpected result from std::decay for rvalue ref type");
        static_assert(std::is_same_v<std::decay_t<const int&>, int>, "Unexpected result from std::decay for const lvalue ref type");
        static_assert(std::is_same_v<std::decay_t<int[2]>, int*>, "Unexpected result from std::decay for array type");
        static_assert(std::is_same_v<std::decay_t<int(int)>, int(*)(int)>, "Unexpected result from std::decay for function type");

        ///////////////////////////////////////////////////////////////////////
        // std::enable_if
        ///////////////////////////////////////////////////////////////////////

        template<class T>
        constexpr typename std::enable_if_t<std::is_reference_v<T>, bool> TestEnableIf() { return true; }
        template<class T>
        constexpr typename std::enable_if_t<!std::is_reference_v<T>, bool> TestEnableIf() { return false; }

        static_assert(!TestEnableIf<int>(), "Unexpected result from enable_if");
        static_assert(TestEnableIf<int&>(), "Unexpected result from enable_if");
    }
}