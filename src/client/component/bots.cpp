#include <std_include.hpp>
#include "loader/component_loader.hpp"

#include "command.hpp"
#include "scheduler.hpp"

#include <utils/hook.hpp>
#include <utils/io.hpp>
#include <utils/string.hpp>

#include <game/game.hpp>
#include "getinfo.hpp"

namespace bots
{
	namespace
	{
		constexpr auto* bot_format_string = "connect \"\\invited\\1\\cg_predictItems\\1\\cl_anonymous\\0\\color\\4\\head\\default\\model\\multi\\snaps\\20\\rate\\"
			"5000\\name\\%s\\clanAbbrev\\%s\\xuid\\%s\\xnaddr\\%s\\natType\\2\\protocol\\%d\\netfieldchk\\%d\\sessionmode\\%s\\qport\\%d\"";

		using bot_name = std::pair<std::string, std::string>;

		std::vector<bot_name> load_bots_names()
		{
			std::vector<bot_name> bot_names =
			{
				{"karma", "@vtolstall"},
				{"good lobster", "lobster"},
				{"evil lobster", "recets"},
				{"doms beach house", "faze_nsl"},
				{"dqminics", "dexy"},
				{"meta", "cruel"},
				{"sprays", "jank"},
				{"vainescape", "vis"},
			};

			std::string buffer;
			if (!utils::io::read_file("boiii/bots.txt", &buffer) || buffer.empty())
			{
				return bot_names;
			}

			auto data = utils::string::split(buffer, '\n');
			for (auto& entry : data)
			{
				utils::string::replace(entry, "\r", "");
				utils::string::trim(entry);

				if (entry.empty())
				{
					continue;
				}

				bot_names.emplace_back(entry, " ");
			}

			return bot_names;
		}

		const std::vector<bot_name>& get_bot_names()
		{
			static const auto bot_names = []
			{
				auto names = load_bots_names();

				std::random_device rd;
				std::mt19937 gen(rd());
				std::ranges::shuffle(names, gen);
				return names;
			}();

			return bot_names;
		}

		const char* get_bot_name()
		{
			static size_t current = 0;
			const auto& names = get_bot_names();

			current = (current + 1) % names.size();
			return names.at(current).first.data();
		}

		int format_bot_string(char* buffer, [[maybe_unused]] const char* format, const char* name, const char* xuid,
		                      const char* xnaddr, int protocol, int net_field_chk, const char* session_mode, int qport)
		{
			return sprintf_s(buffer, 1024, bot_format_string, name, " ",
			                 xuid, xnaddr, protocol, net_field_chk, session_mode, qport);
		}
	}

	struct component final : generic_component
	{
		static_assert(offsetof(game::client_s, bIsTestClient) == 0xBB360);

		void post_unpack() override
		{
			utils::hook::jump(game::select(0x141653B70, 0x1402732E0), get_bot_name);
			utils::hook::call(game::select(0x142249097, 0x14052E53A), format_bot_string);

			if (!game::is_server())
			{
				utils::hook::jump(0x141654280_g, get_bot_name); // SV_ZombieNameRandom
			}

			command::add("spawnBot", [](const command::params& params)
			{
				if (!getinfo::is_host())
				{
					return;
				}

				size_t count = 1;
				if (params.size() > 1)
				{
					if (params[1] == "all"s)
					{
						count = 18;
					}
					else
					{
						count = atoi(params[1]);
					}
				}

				scheduler::once([count]
				{
					for (size_t i = 0; i < count; ++i)
					{
						if (!game::SV_AddTestClient())
						{
							break;
						}
					}
				}, scheduler::server);
			});
		}
	};
}

REGISTER_COMPONENT(bots::component)
