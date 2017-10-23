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
    using CallbackType = std::function<void(const po::variables_map&)>;

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

    void process(const po::variables_map& vm) const {
        callback_(vm);
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

class ProcessCmdError {
public:
    explicit ProcessCmdError(const std::string& message) : message_(message) {}

    explicit ProcessCmdError(std::string&& message) : message_(std::move(message)) {}

    explicit ProcessCmdError(const char* message) : message_(message) {}

    std::string getMessage() const {
        return message_;
    }

private:
    std::string message_;
};

template <ECommand CmdId>
void process_command(const po::variables_map &vm);

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

    const Command *recognize_command(const std::vector<std::string>& command_tokens) {
        sort_();

        size_t token_num = 0;
        auto it_begin = commands_.cbegin();
        auto it_end = commands_.cend();

        for (auto it = command_tokens.begin(); it != command_tokens.end(); ++it, ++token_num) {
            it_begin = std::lower_bound(it_begin, it_end, *it, [token_num] (const std::unique_ptr<Command>& cmd, const std::string& key) {
                return token_num < cmd->get_text().size() ? cmd->get_text()[token_num] < key : true;
            });
            if (it_begin == it_end)
                return nullptr;
            it_end = std::upper_bound(it_begin, it_end, *it, [token_num] (const std::string& key, const std::unique_ptr<Command>& cmd) {
                return token_num < cmd->get_text().size() ? key < cmd->get_text()[token_num]: false;
            });
            if (it_begin == it_end)
                return nullptr;

        }

        if (std::distance(it_begin, it_end) == 1) {
            return it_begin->get();
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
void process_command<ECommand::OPT_USER_CREATE>(const po::variables_map &vm) {
    if (vm.size() > 4)
        throw ProcessCmdError("too many options");

    auto uid = vm["uid"];
    if (uid.empty())
        throw ProcessCmdError("uid option is require for this command");

    auto display_name = vm["display-name"];
    if (display_name.empty())
        throw ProcessCmdError("display-name option is require for this command");

    auto email = vm["email"];
    if (email.empty())
        throw ProcessCmdError("email option is require for this command");

    std::cout << "user created with uid " << uid.as<int>()
              << " display-name " << (display_name.as<std::string>())
              << " and email " << email.as<std::string>() << std::endl;
}

template<>
void process_command<ECommand::OPT_USER_DELETE>(const po::variables_map &vm) {
    if (vm.size() > 2)
        throw ProcessCmdError("too many options");
    auto uid = vm["uid"];
    if (uid.empty())
        throw ProcessCmdError("uid option is require for this command");
    std::cout << "user with uid " << uid.as<int>() << ' ' << " was deleted" << std::endl;
}

template<>
void process_command<ECommand::OPT_USER_INFO>(const po::variables_map &vm) {
    if (vm.size() > 2)
        throw ProcessCmdError("too many options");
    auto uid = vm["uid"];
    if (uid.empty())
        throw ProcessCmdError("uid option is require for this command");
    std::cout << "info about user with uid " << uid.as<int>() << std::endl;
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

void print_help(const CommandsParser& parser, const po::options_description& desc, const char* error_message = nullptr) {
    if (error_message)
        std::cout << error_message << std::endl;
    std::cout << "usage: radosgw-admin <cmd> [options...]" << std::endl;
    parser.print_help();
    desc.print(std::cout);
}

int main(int argc, char** argv) {
    CommandsParser commands_parser = register_commands();
    po::options_description options_desc = register_options();

    const std::string command_tokens = "command_tokens";
    po::positional_options_description pos_options_desc;
    pos_options_desc.add(command_tokens.c_str(), -1);
    po::options_description options_desc_with_pos = options_desc;
    options_desc_with_pos.add_options()(command_tokens.c_str(), po::value<std::vector<std::string>>());

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv).options(options_desc_with_pos).positional(pos_options_desc).run(), vm);
    } catch (...) {
        print_help(commands_parser, options_desc, "invalid command, error when parse command line arguments");
        return 0;
    }
    po::notify(vm);

    if (vm.count("help")) {
        print_help(commands_parser, options_desc);
        return 0;
    }

    std::vector<std::string> tokens;

    try {
        tokens = vm[command_tokens].as<std::vector<std::string>>();
    } catch (const boost::bad_any_cast&) {
        print_help(commands_parser, options_desc, "invalid command, error when parse command line arguments");
        return 0;
    }

    auto cmd = commands_parser.recognize_command(tokens);
    if (cmd == nullptr) {
        print_help(commands_parser, options_desc, ("no such command " + merge_strings(tokens)).c_str());
        return 0;
    }

    try {
        cmd->process(vm);
    } catch (const ProcessCmdError& error) {
        print_help(commands_parser, options_desc, error.getMessage().c_str());
    }

    return 0;
}