#pragma once

#include <cstdint>
#include <string_view>
#include <utility>
#include <optional>

struct module_info_t {
	uintptr_t base_address;
	size_t size;
};

class i_memory_provider {
public:
	virtual ~i_memory_provider() = default;

	virtual bool initialize(uint32_t process_id) = 0;
	virtual bool read_raw(uintptr_t address, void* buffer, size_t size) = 0;
	virtual std::pair<std::optional<uintptr_t>, std::optional<size_t>> get_module_info(
		const std::string_view& module_name) = 0;
	virtual void* get_handle() const = 0;
	virtual bool is_initialized() const = 0;
};