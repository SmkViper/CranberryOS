#include "MiniUart.h"

#include <cstdint>
#include "Peripherals/GPIO.h"
#include "Peripherals/MiniUart.h"
#include "Utils.h"

namespace MiniUART
{
    void Init()
    {
        // #TODO: Manipulating the GPIO pins can likely be done if a more readable fashion with some functions to
        // perform each step, like setting up functionality for each pin, and setting pull up/down states.

        constexpr uint32_t gpioPin14Mask = 7U << 12U;
        constexpr uint32_t gpioPin14Alt5Mode = 2U << 12U;
        constexpr uint32_t gpioPin15Mask = 7U << 15U;
        constexpr uint32_t gpioPin15Alt5Mode = 2U << 15U;

        constexpr uint32_t gpioPin14Change = 1U << 14U;
        constexpr uint32_t gpioPin15Change = 1U << 15U;

        constexpr uint64_t registerCycleDelay = 150U;

        // Baud rate calculation = SystemClockFreqHz / (8 * (BaudrateRegister + 1)))
        // System clock frequency in this case is 250MHz
        constexpr uint32_t baudRate = 270U; // Set baud rate to 115200 (make sure this matches terminal emulator setting)

        // GPFSEL1 register controls pins 10-19
        auto selector = MemoryMappedIO::Get32(MemoryMappedIO::GPIO::FunctionSelect1Register);
        selector &= ~(gpioPin14Mask);   // zero out the three bits for GPIO pin 14
        selector |= gpioPin14Alt5Mode;  // set alt5 (Mini UART Transmit Data Pin) for GPIO pin 14
        selector &= ~(gpioPin15Mask);   // zero out the three bits for GPIO pin 15
        selector |= gpioPin15Alt5Mode;  // set alt5 (Mini UART Receive Data Pin) for GPIO pin 15
        MemoryMappedIO::Put32(MemoryMappedIO::GPIO::FunctionSelect1Register, selector);

        // Initialize pins 14 and 15 to neither be pull up or pull down, since we assume they'll always be connected
        MemoryMappedIO::Put32(MemoryMappedIO::GPIO::PullUpDownRegister, 0); // Tell the circuit we want "Neither" instead of pull up or pull down
        Timing::Delay(registerCycleDelay);
        MemoryMappedIO::Put32(MemoryMappedIO::GPIO::PullUpDownClock0Register, gpioPin14Change | gpioPin15Change); // Flag pins 14 and 15 as the ones to change
        Timing::Delay(registerCycleDelay);
        MemoryMappedIO::Put32(MemoryMappedIO::GPIO::PullUpDownClock0Register, 0); // Clear the clock, indicating we are done

        MemoryMappedIO::Put32(MemoryMappedIO::Auxiliary::EnablesRegister, 1); // Enable mini uart (also enabling access to its registers)
        MemoryMappedIO::Put32(MemoryMappedIO::MiniUART::AdditionalControlRegister, 0); // Disable auto flow control (requires pins the cable doesn't use) and disable receiver and transmitter for now
        MemoryMappedIO::Put32(MemoryMappedIO::MiniUART::InterruptEnableRegister, 0); // Disable receive and transmit interrupts since we're just going to spin instead
        MemoryMappedIO::Put32(MemoryMappedIO::MiniUART::LineControlRegister, 3); // Enable 8 bit mode (as opposed to 7 bit mode)
        MemoryMappedIO::Put32(MemoryMappedIO::MiniUART::ModemControlRegister, 0); // Set RTS line to be always high (used in flow control and not needed)

        MemoryMappedIO::Put32(MemoryMappedIO::MiniUART::BaudRateRegister, baudRate);

        MemoryMappedIO::Put32(MemoryMappedIO::MiniUART::AdditionalControlRegister, 3); // Enable transmitter and receiver now that everything is set up
    }

    char Receive()
    {
        // Wait until the device signals data is available (bit 0 is 1)
        constexpr uint32_t dataReady = 0x01U;
        while ((MemoryMappedIO::Get32(MemoryMappedIO::MiniUART::LineStatusRegister) & dataReady) == 0)
        {
            // Intentionally empty
        }
        constexpr uint32_t charMask = 0xFFU;
        return (MemoryMappedIO::Get32(MemoryMappedIO::MiniUART::IORegister) & charMask);
    }

    void Send(char const aChar)
    {
        // Wait until the device signals that the transmitter is empty (bit 5 is 1)
        constexpr uint32_t transmitterEmpty = 0x20U;
        while ((MemoryMappedIO::Get32(MemoryMappedIO::MiniUART::LineStatusRegister) & transmitterEmpty) == 0)
        {
            // Intentionally empty
        }
        MemoryMappedIO::Put32(MemoryMappedIO::MiniUART::IORegister, aChar);
    }

    void SendString(char const* const apString)
    {
        // #TODO: Can likely remove lint flag when we switch to something like string_view
        // NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        for (auto i = 0U; apString[i] != '\0'; ++i)
        {
            Send(apString[i]);
        }
        // NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
}