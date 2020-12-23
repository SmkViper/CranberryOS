#ifndef KERNEL_UNIT_TESTS_H
#define KERNEL_UNIT_TESTS_H

namespace UnitTests
{
    /**
     * Runs kernel unit tests. Assumes MiniUART is set up for output and static
     * constructors have been run
     */
    void Run();

    /**
     * Runs kernel unit tests to be run after static destructors are run
     */
    void RunPostStaticDestructors();
}

#endif // KERNEL_UNIT_TESTS_H