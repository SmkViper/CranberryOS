# Manually adding the headers here, instead of in a CMakeLists.txt file in the include folder to keep
# said folder "clean" and able to be copied around if needed.
set(HEADER_LIST
    "include/bitset"
    "include/climits"
    "include/cstddef"
    "include/cstdint"
    "include/cstring"
    "include/exception"
    "include/limits.h"
    "include/new"
    "include/stddef.h"
    "include/stdint.h"
    "include/string.h"
    "include/type_traits"
    "include/typeinfo"
    "include/utility"
)

add_library(kernel_stdlib
    ${HEADER_LIST}
)

target_link_libraries(kernel_stdlib PRIVATE CranberryOS_Options CranberryOS_Warnings)

# Flag each file in the header list as a header file (i.e. not compiled). This is because, to follow
# the standards, they don't have a ".h" extension
set_property(SOURCE ${HEADER_LIST}
    PROPERTY HEADER_FILE_ONLY TRUE
)

# Bring in all the library source files
add_subdirectory(src)

# Set up our include directory, and make sure it's visible to users of the library
target_include_directories(kernel_stdlib PUBLIC include)

# We use C++17, and our users need it too
target_compile_features(kernel_stdlib PUBLIC cxx_std_17)