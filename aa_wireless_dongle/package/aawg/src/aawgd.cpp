#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <atomic>

#include "common.h"
#include "bluetoothHandler.h"
#include "proxyHandler.h"
#include "uevent.h"
#include "usb.h"

// Constants
constexpr int CONNECTION_RETRY_DELAY_SEC = 2;  // Delay between connection retry attempts

static std::atomic<bool> g_should_exit{false};

void signal_handler(int signum) {
    Logger::instance()->info("Received signal %d, initiating graceful shutdown\n", signum);
    g_should_exit = true;
}

int main(void) {
    Logger::instance()->info("AA Wireless Dongle\n");

    // Setup signal handlers for graceful shutdown
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    // Global init
    std::optional<std::thread> ueventThread =  UeventMonitor::instance().start();
    UsbManager::instance().init();
    BluetoothHandler::instance().init();

    ConnectionStrategy connectionStrategy = Config::instance()->getConnectionStrategy();
    if (connectionStrategy == ConnectionStrategy::DONGLE_MODE) {
        BluetoothHandler::instance().powerOn();
    }

    while (!g_should_exit) {
        Logger::instance()->info("Connection Strategy: %d\n", connectionStrategy);

        // Per connection setup and processing
        if (connectionStrategy == ConnectionStrategy::USB_FIRST) {
            Logger::instance()->info("Waiting for the accessory to connect first\n");
            UsbManager::instance().enableDefaultAndWaitForAccessory();
        }

        AAWProxy proxy;
        std::optional<std::thread> proxyThread = proxy.startServer(Config::instance()->getWifiInfo().port);

        if (!proxyThread) {
            break;
        }

        if (connectionStrategy != ConnectionStrategy::DONGLE_MODE) {
            BluetoothHandler::instance().powerOn();
        }

        std::optional<std::thread> btConnectionThread = BluetoothHandler::instance().connectWithRetry();

        proxyThread->join();

        if (btConnectionThread) {
            BluetoothHandler::instance().stopConnectWithRetry();
            btConnectionThread->join();
        }

        UsbManager::instance().disableGadget();

        if (connectionStrategy != ConnectionStrategy::DONGLE_MODE && !g_should_exit) {
            sleep(CONNECTION_RETRY_DELAY_SEC);
        }
    }

    Logger::instance()->info("Cleaning up and shutting down\n");

    // Cleanup
    BluetoothHandler::instance().powerOff();
    UsbManager::instance().disableGadget();

    if (ueventThread && ueventThread->joinable()) {
        // Note: ueventThread will never exit on its own, we would need to implement
        // a mechanism to signal it to stop. For now, we detach it.
        ueventThread->detach();
    }

    Logger::instance()->info("Shutdown complete\n");
    return 0;
}
