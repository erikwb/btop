// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <charconv>
#include <istream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

namespace AsahiMemory {

struct Sample {
	long long allocated;
	long long limit;
};

struct Activity {
	bool qualified = false;
	std::optional<long long> percent;
};

// Only consume the qualified G17 ABI. Legacy raw util1..4 have no validated
// percentage semantics. Missing, stale, malformed and out-of-range data are
// unavailable, never an idle (0%) sample.
inline Activity parse_activity(std::istream& input) {
	std::string status, value, line;
	bool got_status = false, got_value = false, malformed = false;
	while (std::getline(input, line)) {
		std::istringstream fields(line);
		std::string key, text, extra;
		if (!(fields >> key)) continue;
		if (key != "utilisation_status" && key != "util_device_percent") continue;
		if (!(fields >> text) || fields >> extra) { malformed = true; continue; }
		if (key == "utilisation_status") {
			if (got_status) malformed = true;
			got_status = true;
			status = text;
		} else {
			if (got_value) malformed = true;
			got_value = true;
			value = text;
		}
	}
	Activity result{got_status, std::nullopt};
	if (input.bad() || malformed || status != "valid" || !got_value) return result;
	long long percent;
	const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), percent);
	if (error == std::errc{} && end == value.data() + value.size() && percent >= 0 && percent <= 100)
		result.percent = percent;
	return result;
}

// Aurora's gpu_memory ledger admits user_committed + kernel_resident bytes.
// These are shared RAM allocations, not dedicated VRAM or engine utilization.
inline std::optional<Sample> parse(std::istream& input) {
	std::optional<long long> user, kernel, limit;
	std::string line;
	while (std::getline(input, line)) {
		std::istringstream fields(line);
		std::string key, value, extra;
		if (!(fields >> key)) continue;
		auto* dest = key == "user_committed" ? &user
			: key == "kernel_resident" ? &kernel
			: key == "limit" ? &limit : nullptr;
		if (!dest) continue; // Allow additional fields in future kernels.
		if (*dest || !(fields >> value) || fields >> extra) return std::nullopt;
		long long bytes;
		const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), bytes);
		if (error != std::errc{} || end != value.data() + value.size() || bytes < 0)
			return std::nullopt;
		*dest = bytes;
	}
	if (input.bad() || !user || !kernel || !limit
		|| *user > std::numeric_limits<long long>::max() - *kernel)
		return std::nullopt;
	return Sample{*user + *kernel, *limit};
}

} // namespace AsahiMemory
