#ifndef KERNEL_UNITTESTS_FRAMEWORK_H
#define KERNEL_UNITTESTS_FRAMEWORK_H

namespace UnitTests
{
    /**
     * Emits pass or failure based on a result bool
     * 
     * @param aResult The result of the test
     * @param apMessage The message to emit
     */
    void EmitTestResult(bool aResult, char const* apMessage);

    /**
     * Emits a skip message
     * 
     * @param apMessage The message to emit
     */
    void EmitTestSkipResult(char const* apMessage);
}

#endif // KERNEL_UNITTESTS_FRAMEWORK_H