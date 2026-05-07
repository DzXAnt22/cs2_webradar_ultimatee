#include "pch.hpp"
#include "win32_provider.hpp"

c_win32_provider::~c_win32_provider()
{
	if (this->m_handle != nullptr && this->m_handle != INVALID_HANDLE_VALUE) {
		CloseHandle(this->m_handle);
		m_handle = nullptr;
	}
}

bool c_win32_provider::initialize(uint32_t process_id)
{
	this->m_process_id = process_id;

	if (!attempt_handle_hijack()) {
		if (!attempt_standard_open()) {
			LOG_WARNING("Failed to open process handle for PID %u", process_id);
			return false;
		}
		LOG_WARNING("Failed to hijack handle, using standard OpenProcess method.");
	}
	else {
		LOG_INFO("Successfully hijacked handle for PID %u", process_id);
	}

	this->m_initialized = true;
	return true;
}

bool c_win32_provider::read_raw(uintptr_t address, void* buffer, size_t size)
{
	if (!m_initialized || m_handle == nullptr)
		return false;

	return ReadProcessMemory(
		reinterpret_cast<HANDLE>(m_handle),
		reinterpret_cast<LPCVOID>(address),
		buffer,
		size,
		nullptr) != 0;
}

std::pair<std::optional<uintptr_t>, std::optional<size_t>> c_win32_provider::get_module_info(
	const std::string_view& module_name)
{
	if (!m_initialized || m_process_id == 0)
		return {std::nullopt, std::nullopt};

	const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_process_id);
	if (snapshot == INVALID_HANDLE_VALUE)
		return {std::nullopt, std::nullopt};

	MODULEENTRY32 module_entry = {0};
	module_entry.dwSize = sizeof(module_entry);

	for (Module32First(snapshot, &module_entry); Module32Next(snapshot, &module_entry);) {
		auto equals_ignore_case = [](const std::string_view str_1, const std::string_view str_2) {
			return (str_1.size() == str_2.size()) &&
				   equal(str_1.begin(), str_1.end(), str_2.begin(), [](const char a, const char b) {
					   return tolower(a) == tolower(b);
				   });
		};

		if (equals_ignore_case(module_entry.szModule, module_name)) {
			CloseHandle(snapshot);
			return {reinterpret_cast<uintptr_t>(module_entry.modBaseAddr),
					static_cast<size_t>(module_entry.modBaseSize)};
		}
	}

	CloseHandle(snapshot);
	return {std::nullopt, std::nullopt};
}

void* c_win32_provider::get_handle() const
{
	return m_handle;
}

bool c_win32_provider::is_initialized() const
{
	return m_initialized;
}

std::optional<uint32_t> c_win32_provider::get_process_id(const std::string_view& process_name)
{
	const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE)
		return {};

	PROCESSENTRY32 process_entry = {0};
	process_entry.dwSize = sizeof(process_entry);

	for (Process32First(snapshot, &process_entry); Process32Next(snapshot, &process_entry);) {
		if (std::string_view(process_entry.szExeFile) == process_name) {
			CloseHandle(snapshot);
			return process_entry.th32ProcessID;
		}
	}

	CloseHandle(snapshot);
	return {};
}

bool c_win32_provider::attempt_handle_hijack()
{
	auto handle_opt = hijack_handle();
	if (handle_opt.has_value()) {
		m_handle = handle_opt.value();
		return true;
	}
	return false;
}

bool c_win32_provider::attempt_standard_open()
{
	m_handle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, false, m_process_id);
	return m_handle != nullptr;
}

std::optional<void*> c_win32_provider::hijack_handle()
{
	auto cleanup = [](std::vector<uint8_t>& handle_info, void*& process_handle) {
		handle_info.clear();

		if (process_handle != INVALID_HANDLE_VALUE && process_handle != nullptr) {
			CloseHandle(process_handle);
			process_handle = nullptr;
		}
	};

	const auto ntdll = GetModuleHandleA("ntdll.dll");
	if (!ntdll)
		return {};

	using fn_nt_query_system_information = long(__stdcall*)(unsigned long, void*, unsigned long,
															 unsigned long*);
	const auto nt_query_system_information = reinterpret_cast<fn_nt_query_system_information>(
		GetProcAddress(ntdll, "NtQuerySystemInformation"));
	if (!nt_query_system_information)
		return {};

	using fn_nt_duplicate_object = long(__stdcall*)(void*, void*, void*, void**, unsigned long,
												   unsigned long, unsigned long);
	const auto nt_duplicate_object = reinterpret_cast<fn_nt_duplicate_object>(
		GetProcAddress(ntdll, "NtDuplicateObject"));
	if (!nt_duplicate_object)
		return {};

	using fn_nt_open_process =
		long(__stdcall*)(void**, unsigned long, OBJECT_ATTRIBUTES*, CLIENT_ID*);
	const auto nt_open_process =
		reinterpret_cast<fn_nt_open_process>(GetProcAddress(ntdll, "NtOpenProcess"));
	if (!nt_open_process)
		return {};

	using fn_rtl_adjust_privilege = long(__stdcall*)(unsigned long, unsigned char, unsigned char,
													   unsigned char*);
	const auto rtl_adjust_privilege = reinterpret_cast<fn_rtl_adjust_privilege>(
		GetProcAddress(ntdll, "RtlAdjustPrivilege"));
	if (!rtl_adjust_privilege)
		return {};

	uint8_t old_privilege = 0;
	rtl_adjust_privilege(0x14, 1, 0, &old_privilege);

	OBJECT_ATTRIBUTES object_attributes{};
	InitializeObjectAttributes(&object_attributes, nullptr, 0, nullptr, nullptr);

	std::vector<uint8_t> handle_info(sizeof(system_handle_info_t));
	std::pair<void*, void*> handle{nullptr, nullptr};
	CLIENT_ID client_id{};

	unsigned long status = 0;
	do {
		handle_info.resize(handle_info.size() * 2);
		status = nt_query_system_information(0x10, handle_info.data(),
											  static_cast<unsigned long>(handle_info.size()), nullptr);
	} while (status == 0xc0000004);

	if (!NT_SUCCESS(status)) {
		cleanup(handle_info, handle.first);
		return {};
	}

	const auto system_handle_info = reinterpret_cast<system_handle_info_t*>(handle_info.data());
	for (uint32_t idx = 0; idx < system_handle_info->m_handle_count; ++idx) {
		const auto system_handle = system_handle_info->m_handles[idx];
		if (reinterpret_cast<void*>(system_handle.m_handle) == INVALID_HANDLE_VALUE ||
			system_handle.m_object_type_number != 0x07)
			continue;

		client_id.UniqueProcess = reinterpret_cast<void*>(system_handle.m_process_id);

		if (handle.first != nullptr && handle.first != INVALID_HANDLE_VALUE) {
			CloseHandle(handle.first);
			handle.first = nullptr;
		}

		const auto open_process =
			nt_open_process(&handle.first, PROCESS_DUP_HANDLE, &object_attributes, &client_id);
		if (!NT_SUCCESS(open_process))
			continue;

		const auto duplicate_object = nt_duplicate_object(handle.first,
														   reinterpret_cast<void*>(system_handle.m_handle),
														   GetCurrentProcess(), &handle.second,
														   PROCESS_ALL_ACCESS, 0, 0);
		if (!NT_SUCCESS(duplicate_object))
			continue;

		if (GetProcessId(handle.second) == this->m_process_id) {
			cleanup(handle_info, handle.first);
			return handle.second;
		}

		CloseHandle(handle.second);
	}

	cleanup(handle_info, handle.first);
	return {};
}