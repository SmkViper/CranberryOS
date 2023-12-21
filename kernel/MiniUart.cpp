#include "MiniUart.h"

#include "Peripherals/GPIO.h"
#include "Peripherals/MiniUart.h"
#include "Utils.h"

namespace MiniUART
{
    void Init()
    {
        // TODO: Manipulating the GPIO pins can likely be done if a more readable fashion with some functions to
        // perform each step, like setting up functionality for each pin, and setting pull up/down states.

        // GPFSEL1 register controls pins 10-19
        auto selector = MemoryMappedIO::Get32(MemoryMappedIO::GPIO::FunctionSelect1Register);
        selector &= ~(7u<<12);   // zero out the three bits for GPIO pin 14
        selector |= 2<<12;      // set alt5 (Mini UART Transmit Data Pin) for GPIO pin 14
        selector &= ~(7u<<15);   // zero out the three bits for GPIO pin 15
        selector |= 2<<15;      // set alt5 (Mini UART Receive Data Pin) for GPIO pin 15
        MemoryMappedIO::Put32(MemoryMappedIO::GPIO::FunctionSelect1Register, selector);

        // Initialize pins 14 and 15 to neither be pull up or pull down, since we assume they'll always be connected
        MemoryMappedIO::Put32(MemoryMappedIO::GPIO::PullUpDownRegister, 0); // Tell the circuit we want "Neither" instead of pull up or pull down
        Timing::Delay(150);
        MemoryMappedIO::Put32(MemoryMappedIO::GPIO::PullUpDownClock0Register, (1<<14)|(1<<15)); // Flag pins 14 and 15 as the ones to change
        Timing::Delay(150);
        MemoryMappedIO::Put32(MemoryMappedIO::GPIO::PullUpDownClock0Register, 0); // Clear the clock, indicating we are done

        MemoryMappedIO::Put32(MemoryMappedIO::Auxiliary::EnablesRegister, 1); // Enable mini uart (also enabling access to its registers)
        MemoryMappedIO::Put32(MemoryMappedIO::MiniUART::AdditionalControlRegister, 0); // Disable auto flow control (requires pins the cable doesn't use) and disable receiver and transmitter for now
        MemoryMappedIO::Put32(MemoryMappedIO::MiniUART::InterruptEnableRegister, 0); // Disable receive and transmit interrupts since we're just going to spin instead
        MemoryMappedIO::Put32(MemoryMappedIO::MiniUART::LineControlRegister, 3); // Enable 8 bit mode (as opposed to 7 bit mode)
        MemoryMappedIO::Put32(MemoryMappedIO::MiniUART::ModemControlRegister, 0); // Set RTS line to be always high (used in flow control and not needed)

        // Baud rate calculation = SystemClockFreqHz / (8 * (BaudrateRegister + 1)))
        // System clock frequency in this case is 250MHz
        MemoryMappedIO::Put32(MemoryMappedIO::MiniUART::BaudRateRegister, 270); // Set baud rate to 115200 (make sure this matches terminal emulator setting)

        MemoryMappedIO::Put32(MemoryMappedIO::MiniUART::AdditionalControlRegister, 3); // Enable transmitter and receiver now that everything is set up
    }

    char Receive()
    {
        // Wait until the device signals data is available (bit 0 is 1)
        while ((MemoryMappedIO::Get32(MemoryMappedIO::MiniUART::LineStatusRegister) & 0x01) == 0)
        {
            // Intentionally empty
        }
        return (MemoryMappedIO::Get32(MemoryMappedIO::MiniUART::IORegister) & 0xFF);
    }

    void Send(char aChar)
    {
        // Wait until the device signals that the transmitter is empty (bit 5 is 1)
        while ((MemoryMappedIO::Get32(MemoryMappedIO::MiniUART::LineStatusRegister) & 0x20) == 0)
        {
            // Intentionally empty
        }
        MemoryMappedIO::Put32(MemoryMappedIO::MiniUART::IORegister, aChar);
    }

    void SendString(const char* const apString)
    {
        for (auto i = 0; apString[i] != '\0'; ++i)
        {
            Send(apString[i]);
        }
    }
}