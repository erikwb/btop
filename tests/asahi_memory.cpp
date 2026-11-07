// SPDX-License-Identifier: Apache-2.0
#undef NDEBUG // Keep standalone checks enabled in Release builds.
#include "../src/linux/asahi_memory.hpp"
#include <cassert>
#include <sstream>

int main() {
    using namespace AsahiMemory;
    auto memory = [](const char* text) { std::istringstream in(text); return parse(in); };
    const auto live = memory("user_committed 315342848\nuser_resident 315342848\nkernel_resident 549617664\nlimit 90597566545\nclient_limit 67610124288\nrefused 0\n");
    assert(live && live->allocated == 864960512 && live->limit == 90597566545);
    assert(memory("user_committed 0\nkernel_resident 0\nlimit 0\n")->allocated == 0);
    assert(!memory("unsupported\n"));
    assert(!memory("user_committed -1\nkernel_resident 2\nlimit 3\n"));
    assert(!memory("user_committed 1x\nkernel_resident 2\nlimit 3\n"));
    assert(!memory("user_committed 9223372036854775807\nkernel_resident 1\nlimit 0\n"));
    assert(!memory("user_committed 1\nuser_committed 2\nkernel_resident 2\nlimit 3\n"));
    assert(!memory("user_committed 1\nkernel_resident 2\n"));
    auto activity = [](const char* text) { std::istringstream in(text); return parse_activity(in); };
    assert(activity("utilisation_status valid\nutil_device_percent 73\n").percent == 73);
    assert(activity("util_device_percent 0\nutilisation_status valid\n").percent == 0);
    assert(activity("utilisation_status stale\nutil_device_percent unsupported\n").qualified);
    assert(!activity("utilisation_status stale\nutil_device_percent 73\n").percent);
    assert(!activity("utilisation_status unavailable\nutil_device_percent 0\n").percent);
    assert(!activity("utilisation_status valid\nutil_device_percent 101\n").percent);
    assert(!activity("utilisation_status valid\nutil_device_percent -1\n").percent);
    assert(!activity("utilisation_status valid\nutil_device_percent 7x\n").percent);
    assert(!activity("utilisation_status valid\nutil_device_percent 12\nutil_device_percent 15\n").percent);
    assert(!activity("utilisation_status valid\n").percent);
    assert(!activity("util1 100\nutil2 100\n").qualified);
    assert(!activity("unsupported\n").qualified);
}
