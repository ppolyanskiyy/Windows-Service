#include <Windows.h>
#include <iostream>

constexpr unsigned SERVICE_START_WAIT_HINT = 3030;

class Service
{
public:
	Service(const Service&) = delete;
	Service(Service&&) = delete;
	Service& operator=(const Service&) = delete;
	Service& operator=(Service&&) = delete;

	static const std::wstring& get_service_name();
	static Service& get_instance();

	static void set_service_name(const std::wstring&);

	static bool Main(unsigned short argc, wchar_t* argv[]);
	static bool CtrlHandler(unsigned short control);
	static bool Initialize(unsigned short argc, wchar_t* argv[]);
	static bool ReportStatus(
		unsigned short current_state,
		unsigned short win32_exit_code,
		unsigned short wait_hint);
	static bool Install();
	static bool Start();
	static bool Stop();
	static bool Restart();
	static bool Uninstall();

private:
	Service() = default;

	static SERVICE_STATUS s_service_status;
	static SERVICE_STATUS_HANDLE s_service_status_handle;
	static HANDLE s_service_stop_event;
	static std::shared_ptr<Service> s_instance;
	static std::wstring s_service_name;
};