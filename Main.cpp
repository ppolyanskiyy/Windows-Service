#include "WindowsService.h"

const std::wstring_view
INSTALL = L"install",
START = L"start",
STOP = L"stop",
RESTART = L"restart",
UNINSTALL = L"uninstall";

const short int TABLE_ENTRY_SIZE = 2;

int wmain(unsigned argc, wchar_t* argv[])
{
	Service::set_service_name(L"AAA_ServiceName");

	if (argc > 1)
	{
		if (!wcscmp(argv[1], INSTALL.data()))			Service::get_instance().Install();
		else if (!wcscmp(argv[1], START.data()))		Service::get_instance().Start();
		else if (!wcscmp(argv[1], STOP.data()))			Service::get_instance().Stop();
		else if (!wcscmp(argv[1], RESTART.data()))		Service::get_instance().Restart();
		else if (!wcscmp(argv[1], UNINSTALL.data()))	Service::get_instance().Uninstall();
	}
	else
	{
		SERVICE_TABLE_ENTRY service_table[TABLE_ENTRY_SIZE]; // Contain entry for each service
		service_table[0].lpServiceName = const_cast<wchar_t*>(Service::get_instance().get_service_name().c_str());
		service_table[0].lpServiceProc = reinterpret_cast<LPSERVICE_MAIN_FUNCTION>(Service::get_instance().Main);

		service_table[1].lpServiceName = nullptr;
		service_table[1].lpServiceProc = nullptr;

		if (!StartServiceCtrlDispatcher(service_table)) // Connect main thread of service procces with the SCM
			return GetLastError();
	}

	return 0;
}