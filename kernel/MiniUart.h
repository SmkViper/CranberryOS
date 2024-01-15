#ifndef KERNEL_MINIUART_H
#define KERNEL_MINIUART_H

namespace MiniUART
{
    /**
     * Initialize MiniUART on the board - must be called once (and only once) before using
     * the other UART functions
     */
    void Init();

    /**
     * Receive a single byte of data over MiniUART
     * 
     * @return The data received
     */
    char Receive();

    /**
     * Send a single byte of data over MiniUART
     * 
     * @param aChar The data to send
     */
    void Send(char aChar);

    /**
     * Send a string over MiniUART
     * 
     * @param apString String to send - expected to be non-null and zero-terminated
     */
    void SendString(char const* apString);
}

#endif // KERNEL_MINIUART_H