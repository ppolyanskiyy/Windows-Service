#include "WindowsService.h"

SERVICE_STATUS Service::s_service_status{};
SERVICE_STATUS_HANDLE Service::s_service_status_handle{};
HANDLE Service::s_service_stop_event{};
std::shared_ptr<Service>Service::s_instance{};
std::wstring Service::s_service_name{};

const std::wstring& Service::get_service_name()
{
	return s_service_name;
}

Service& Service::get_instance()
{
	if (!s_instance.get())
	{
		s_instance = std::shared_ptr<Service>{ new Service };
	}
	return *s_instance;
}

void Service::set_service_name(const std::wstring& str)
{
	s_service_name = str;
}

// The entry point for a service
bool Service::Main(unsigned short argc, wchar_t* argv[])
{
	s_service_status_handle = RegisterServiceCtrlHandler( // Register our service control handler with the SCM
		Service::get_instance().s_service_name.c_str(),
		reinterpret_cast<LPHANDLER_FUNCTION>(CtrlHandler));

	if (!s_service_status_handle) return false;

	ZeroMemory(&s_service_status, sizeof(s_service_status)); // Tell the controller we are starting
	s_service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	s_service_status.dwServiceSpecificExitCode = 0;

	ReportStatus(SERVICE_START_PENDING, NO_ERROR, SERVICE_START_WAIT_HINT);
	return Initialize(argc, argv);
}

bool Service::ReportStatus(
	unsigned short current_state,
	unsigned short win32_exit_code,
	unsigned short wait_hint)
{
	static unsigned short check_point = 1;

	s_service_status.dwCurrentState = current_state;
	s_service_status.dwWin32ExitCode = win32_exit_code;
	s_service_status.dwWaitHint = wait_hint;

	if (current_state == SERVICE_START_PENDING) // Service is about to start
	{
		s_service_status.dwControlsAccepted = 0;
	}
	else
	{
		s_service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	}

	if (current_state == SERVICE_RUNNING || // Progress for Service operation
		current_state == SERVICE_STOPPED)
	{
		s_service_status.dwCheckPoint = 0;
	}
	else
	{
		s_service_status.dwCheckPoint = check_point++;
	}

	if (!SetServiceStatus(s_service_status_handle, &s_service_status))
	{
		std::cerr << "Report Status: ERROR " << GetLastError() << "\n";
		return false;
	}
	return true;
}

bool Service::Initialize(unsigned short argc, wchar_t* argv[])
{
	s_service_stop_event = CreateEvent( // Create a service stop event to wait on later
		nullptr,	// Security Attributes
		true,		// MANUAL Reset Event
		false,		// Non-Signaled
		nullptr);	// Name of Event

	if (!s_service_stop_event)
	{
		return ReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
	}
	else
	{
		ReportStatus(SERVICE_RUNNING, NO_ERROR, 0);
		WaitForSingleObject(s_service_stop_event, INFINITE); // Wait event to be Signaled
		return ReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
	}
}

bool Service::CtrlHandler(unsigned short request)
{
	switch (request)
	{
	case SERVICE_CONTROL_STOP:
		if (s_service_status.dwCurrentState != SERVICE_RUNNING) break;
		return ReportStatus(SERVICE_STOPPED, NO_ERROR, 0);

	default: return false;
	}
	return false;
}

bool Service::Install()
{
	SC_HANDLE handle_SCM = nullptr;
	SC_HANDLE handle_service = nullptr;
	wchar_t path[MAX_PATH];

	if (!GetModuleFileName(nullptr, path, MAX_PATH)) // Get the Executable file from SCM
	{
		std::cerr << "Install: GetModuleFileName: ERROR " << GetLastError() << "\n";
		return false;
	};

	handle_SCM = OpenSCManager(
		nullptr,					// Local Machine
		nullptr,					// By default Database 
		SC_MANAGER_ALL_ACCESS);		// Access Right

	if (!handle_SCM)
	{
		std::cerr << "Install: OpenSCManager: ERROR " << GetLastError() << "\n";
		return false;
	}
	else
	{
		handle_service = CreateService(
			handle_SCM,										// SCM Handle
			Service::get_instance().s_service_name.c_str(), // Service Name
			Service::get_instance().s_service_name.c_str(),	// Display Name			
			SERVICE_ALL_ACCESS,								// Access Right
			SERVICE_WIN32_OWN_PROCESS,						// Service Type
			SERVICE_DEMAND_START,							// Service Start Type
			SERVICE_ERROR_NORMAL,							// Service Error Code
			path,											// Path to .exe
			nullptr,									    // Load ordering group
			nullptr,                                        // Tag ID
			nullptr,	                                    // Dependencies
			nullptr,						                // Service start name (account)
			nullptr);                                       // Password

		bool is_created = true;

		if (!handle_service)
		{
			std::cerr << "Install: CreateService: ERROR " << GetLastError() << "\n";
			is_created = false;
		}
		else
		{
			std::cout << "Install: succeeded :)\n";
			CloseServiceHandle(handle_service);
		}
		CloseServiceHandle(handle_SCM);
		return is_created;
	}
}

bool Service::Start()
{
	//SERVICE_STATUS_PROCESS status_process ;
	SC_HANDLE open_SCM = nullptr;
	SC_HANDLE open_service = nullptr;

	open_SCM = OpenSCManager(
		nullptr,					// Local Machine
		nullptr,					// By default Database (SERVICES_ACTIVE_DATABASE)
		SC_MANAGER_ALL_ACCESS);		// Access Right

	if (!open_SCM)
	{
		std::cerr << "Start: OpenSCManager: ERROR " << GetLastError() << "\n";
		return false;
	}

	bool is_opened = true, is_started = true;

	open_service = OpenService(
		open_SCM,											// SCM Handle
		Service::get_instance().s_service_name.c_str(),		// Service Name
		SC_MANAGER_ALL_ACCESS);								// Access Right

	if (!open_service)
	{
		std::cerr << "Start: OpenService: ERROR " << GetLastError() << "\n";
		is_opened = false;
	}
	else if (is_opened)
	{
		if (!StartService(open_service, false, nullptr))
		{
			std::cerr << "Start: StartService: ERROR " << GetLastError() << "\n";
			is_started = false;
		}
		else
		{
			std::cout << "Start: succeeded :)\n";
		}

		CloseServiceHandle(open_service);
	}
	CloseServiceHandle(open_SCM);
	return is_opened && is_started;

}

bool Service::Stop()
{
	SERVICE_STATUS_PROCESS status_process;
	SC_HANDLE open_SCM = nullptr;
	SC_HANDLE open_service = nullptr;

	open_SCM = OpenSCManager(
		nullptr,					// Local Machine
		nullptr,					// By default Database (SERVICES_ACTIVE_DATABASE)
		SC_MANAGER_ALL_ACCESS);		// Access Right

	if (!open_SCM)
	{
		std::cerr << "Stop: OpenSCManager: ERROR " << GetLastError() << "\n";
		return false;
	}

	bool is_opened = true, is_stopped = true;

	open_service = OpenService(
		open_SCM,											// SCM Handle
		Service::get_instance().s_service_name.c_str(),		// Service Name
		SC_MANAGER_ALL_ACCESS);								// Access Right

	if (!open_service)
	{
		std::cerr << "Stop: OpenService: ERROR " << GetLastError() << "\n";
		is_opened = false;
	}
	else if (is_opened)
	{
		if (!ControlService(
			open_service,
			SERVICE_CONTROL_STOP,
			reinterpret_cast<LPSERVICE_STATUS>(&status_process)))
		{
			int error_code = GetLastError();
			if (!error_code)
			{
				std::cout << "Stop: succeeded :)\n";
			}
			else
			{
				std::cerr << "Stop: ControlService: ERROR " << GetLastError() << "\n";
				is_stopped = false;
			}
		}
		CloseServiceHandle(open_service);
	}
	CloseServiceHandle(open_SCM);
	return is_opened && is_stopped;
}

bool Service::Restart()
{
	bool is_stopped = Service::get_instance().Stop();
	bool is_started = Service::get_instance().Start();

	if (is_stopped && is_started)
	{
		std::cout << "Restart: succeeded :)\n";
		return true;
	}

	std::cerr << "Restart: ERROR " << GetLastError() << "\n";
	return false;
}

bool Service::Uninstall()
{
	SC_HANDLE handle_SCM = nullptr;
	SC_HANDLE handle_service = nullptr;

	handle_SCM = OpenSCManager(
		nullptr,					// Local Machine
		nullptr,					// By default Database (SERVICES_ACTIVE_DATABASE)
		SC_MANAGER_ALL_ACCESS);		// Access Right

	if (!handle_SCM)
	{
		std::cerr << "Uninstall: OpenSCManager: ERROR " << GetLastError() << "\n";
		return false;
	}

	bool is_opened = true, is_deleted = true;

	handle_service = OpenService(
		handle_SCM,										// SCM Handle
		Service::get_instance().s_service_name.c_str(), // Service Name
		SERVICE_ALL_ACCESS);							// Access Right

	if (!handle_service)
	{
		std::cerr << "Uninstall: OpenSCManager: ERROR " << GetLastError() << "\n";
		is_opened = false;
	}
	else if (is_opened)
	{
		if (!DeleteService(handle_service))
		{
			std::cerr << "Uninstall: DeleteService: ERROR " << GetLastError() << "\n";
			is_deleted = false;
		}
		else
		{
			std::cout << "Uninstall: succeeded :)\n";
		}
		CloseServiceHandle(handle_service);
	}
	CloseServiceHandle(handle_SCM);
	return is_opened && is_deleted;
}