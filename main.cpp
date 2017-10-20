#include <iostream>
#include <vector>
#include <sstream>

#include <boost/program_options.hpp>

namespace po = boost::program_options;

enum ECommand {
    OPT_USER_CREATE,
    OPT_USER_DELETE,
    OPT_USER_INFO
};

static std::string merge_strings(const std::vector<std::string>& v) {
    std::string ans;
    for (size_t i = 0; i < v.size() - 1; ++i)
        ans += v[i] + ' ';
    ans += v.back();
    return ans;
}

class Command {
public:
    using CallbackType = std::function<bool(const po::variables_map&)>;

    Command(ECommand id, const std::vector<std::string>& text,
            const std::string& help, const CallbackType& callback)
            : help_(help), text_(text), id_(id), callback_(callback) {
        if (text.empty())
            throw std::logic_error("command-text is empty");
    }

    std::string get_help() const {
        return help_;
    }

    std::vector<std::string> get_text() const {
        return text_;
    }

    std::string get_merged_text() const {
        return merge_strings(text_);
    }

    ECommand get_id() const {
        return id_;
    }

    bool process(const po::variables_map& vm) const {
        return callback_(vm);
    }

private:
    ECommand id_;
    std::vector<std::string> text_;
    const std::string help_;
    CallbackType callback_;
};

static std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> ans;
    std::istringstream s_stream(s);
    std::string tmp;

    while (s_stream >> tmp)
        ans.push_back(tmp);

    return ans;
}

static bool start_with(const char* str, const char* pref) {
    auto it_str = str;
    auto it_pref = pref;
    while (*it_str == *it_pref && *it_pref != 0) {
        ++it_str;
        ++it_pref;
    }
    return *it_pref == 0;
}

template <ECommand CmdId>
bool process_command(const po::variables_map &vm);

class CommandsParser {
public:
    CommandsParser() = default;

    CommandsParser(const CommandsParser&) = delete;
    CommandsParser& operator = (const CommandsParser&) = delete;

    CommandsParser(CommandsParser&&) = default;
    CommandsParser& operator = (CommandsParser&&) = default;

    void register_command(ECommand command_id, const std::string& text, const std::string& help, const Command::CallbackType& callback) {
        commands_.push_back(std::make_unique<Command>(command_id, split(text), help, callback));
        commands_is_sorted = false;
    }

    const Command *recognize_command(int argc, char **argv) {
        sort_();

        size_t token_num = 0;
        auto it_begin = commands_.cbegin();
        auto it_end = commands_.cend();

        for (auto it = argv + 1; it != argv + argc; ++it, ++token_num) {
            it_begin = std::lower_bound(it_begin, it_end, *it, [token_num] (const std::unique_ptr<Command>& cmd, const char* key) {
                return strcmp(cmd->get_text()[token_num].c_str(), key) < 0;
            });
            if (it_begin == it_end)
                return nullptr;
            it_end = std::upper_bound(it_begin, it_end, *it, [token_num] (const char* key, const std::unique_ptr<Command>& cmd) {
                return strcmp(key, cmd->get_text()[token_num].c_str()) < 0;
            });
            if (it_begin == it_end)
                return nullptr;
            if (std::distance(it_begin, it_end) == 1) {
                return it_begin->get();
            }
        }

        return nullptr;
    }

    void print_help() const {
        std::cout << "commands:" << std::endl;

        std::vector<std::string> cmd_names;
        std::vector<std::string> cmd_helps;

        size_t max_cmd_len = 0;

        for (const auto& cmd : commands_) {
            cmd_names.push_back(cmd->get_merged_text());
            cmd_helps.push_back(cmd->get_help());
            max_cmd_len = std::max(max_cmd_len, cmd_names.back().size());
        }

        for (size_t i = 0; i < cmd_names.size(); ++i) {
            std::cout << cmd_names[i];
            size_t rest = max_cmd_len - cmd_names[i].size();
            for (size_t j = 0; j < rest; ++j)
                std::cout << ' ';
            std::cout << "    " << cmd_helps[i] << std::endl;
        }
    }

private:
    std::vector<std::unique_ptr<Command>> commands_;
    bool commands_is_sorted = false;

private:
    void sort_() {
        if (!commands_is_sorted) {
            std::sort(commands_.begin(), commands_.end(),
                      [](const std::unique_ptr<Command> &c1, const std::unique_ptr<Command> &c2) {
                          return c1->get_text() < c2->get_text();
                      });
            commands_is_sorted = true;
        }
    }
};

template<>
bool process_command<ECommand::OPT_USER_CREATE>(const po::variables_map &vm) {
    if (vm.size() > 3)
        return false;
    auto uid = vm["uid"];
    if (uid.empty())
        return false;
    auto display_name = vm["display-name"];
    if (display_name.empty())
        return false;
    auto email = vm["email"];
    if (email.empty())
        return false;
    std::cout << "user created with uid " << boost::any_cast<int>(uid.value())
              << " display-name " << boost::any_cast<std::string>(display_name.value())
              << " and email " << boost::any_cast<std::string>(email.value()) << std::endl;
    return true;
}

template<>
bool process_command<ECommand::OPT_USER_DELETE>(const po::variables_map &vm) {
    if (vm.size() > 1)
        return false;
    auto uid = vm["uid"];
    if (uid.empty())
        return false;
    std::cout << "user with uid " << boost::any_cast<int>(uid.value()) << ' ' << " was deleted" << std::endl;
    return true;
}

template<>
bool process_command<ECommand::OPT_USER_INFO>(const po::variables_map &vm) {
    if (vm.size() > 1)
        return false;
    auto uid = vm["uid"];
    if (uid.empty())
        return false;
    std::cout << "info about user with uid " << boost::any_cast<int>(uid.value()) << std::endl;
    return true;
}

template <ECommand CommandId>
void register_command(CommandsParser& parser, const std::string& text, const std::string& help) {
    parser.register_command(CommandId, text, help, process_command<CommandId>);
}

CommandsParser register_commands() {
    CommandsParser parser;
    register_command<ECommand::OPT_USER_CREATE>(parser, "user create", "create a new user");
    register_command<ECommand::OPT_USER_DELETE>(parser, "user delete", "delete a user");
    register_command<ECommand::OPT_USER_INFO>(parser, "user info", "get user info");
    return parser;
}

po::options_description register_options() {
    po::options_description desk("options");
    desk.add_options()
        ("help", "produce help message")
        ("uid", po::value<int>(), "user id")
        ("display-name", po::value<std::string>(), "")
        ("email", po::value<std::string>(), "")
    ;
    return desk;
}

void print_help(const CommandsParser& parser, const po::options_description& desc) {
    std::cout << "usage: radosgw-admin <cmd> [options...]" << std::endl;
    parser.print_help();
    desc.print(std::cout);
}

void print_help_on_error(const CommandsParser& parser, po::options_description& option_desk) {
    std::cout << "invalid command" << std::endl;
    print_help(parser, option_desk);
}

int main(int argc, char** argv) {
    CommandsParser commands_parser = register_commands();
    po::options_description options_desc = register_options();

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv).options(options_desc).run(), vm);
    } catch (...) {
        print_help_on_error(commands_parser, options_desc);
        return 0;
    }
    po::notify(vm);

    if (vm.count("help")) {
        print_help(commands_parser, options_desc);
        return 0;
    }

    auto cmd = commands_parser.recognize_command(argc, argv);
    if (cmd == nullptr) {
        print_help_on_error(commands_parser, options_desc);
        return 0;
    }

    if (!cmd->process(vm)) {
        print_help_on_error(commands_parser, options_desc);
        return 0;
    }

    return 0;
}