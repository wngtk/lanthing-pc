#pragma once
#include <cstdint>
#include <string>
#include <memory>
#include <Windows.h>

namespace ltlib
{

class WinApp
{
public:
    virtual ~WinApp() { }
    virtual void on_start() = 0;
    virtual void on_stop() = 0;
    virtual void run() = 0;
};

class ServiceApp
{
public:
    ServiceApp(WinApp* service);
    ~ServiceApp();
    void run();

private:
    static bool report_status(uint32_t current_state, uint32_t win32_exit_code, uint32_t wait_hint);
    static void __stdcall service_main();
    static void __stdcall service_control_handler(unsigned long ctrl_code);
};

bool is_service_running(const std::string& name, uint32_t& pid);
bool start_service(const std::string& name);

} // namespace ltlib
