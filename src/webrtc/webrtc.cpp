#include "config/options.hpp"

#include <iostream>
#include <fstream>
#include <sstream>

#include <boost/tokenizer.hpp>
#include <boost/token_functions.hpp>

#define PO_DESC          "Allowed options"

#define PO_HELP_OP       "help,h"
#define PO_HELP_OP_SHORT "h"
#define PO_HELP_OP_LONG  "help"

#define PO_OP_FILE       "config,c"
#define PO_OP_FILE_LONG  "config"
#define PO_OP_FILE_DESC  "specify a config file"
#define PO_ERROR_OP_FILE "Could not open the given options file"

#define PO_HELP_DESC     "Displays this help message"

namespace utils { namespace program_options {

	/**
	 * constructors
	 */
	// default constructor
	options :: options()
	: _options(PO_DESC) {
		_init();
	}

	// constructor receiving a custom options description
	options :: options(const char* desc)
	: _options(desc) {
		_init();
	}

	/**
	 * add methods
	 */
	options& options :: flag(
			const char* name,
			bool& var,
			const char* desc) {
		this->_options.add_options() (name, bpo::bool_switch(&var), desc);
		return *this;
	}

	/**
	 *
	 */
	void options::parse(int argc, char **argv) {
		bpo::store(
				bpo::command_line_parser(argc, argv)
				   .options(this->_options)
				   .positional(this->_positional_options)
				   .run()
				, this->vm);

		bpo::notify(this->vm);

		// check for options-file
		if (vm.count(PO_OP_FILE_LONG)) {
			std::vector<std::string> files = vm[PO_OP_FILE_LONG].as<std::vector<std::string> >();
			for(std::vector<std::string>::reverse_iterator it = files.rbegin(); it != files.rend(); ++it) {
				this->load_options_file(*it);
			}
		}

		// deal with help options
		if (this->count("help")) {
			std::cout << this->help();
			exit(0);
		}
	}

	/**
	 * Getters
	 */
	bool options::get_flag(const char* name) const {
		return this->get<bool>(name);
	}

	int options::count(const char* name) const {
		return this->vm.count(name);
	}

	bool options::has(const char* name) const {
		return this->count(name) > 0;
	}

	bpo::options_description options::help() const {
		return this->_options;
	}

	void options::_init() {
		this->_options.add_options()
			(PO_HELP_OP, PO_HELP_DESC)
			(PO_OP_FILE, bpo::value<std::vector<std::string> >(), PO_OP_FILE_DESC);
		this->_positional_options.add(PO_OP_FILE_LONG, -1);
	}

//	pair<string, string> options::file_option_parser(const string& s) {
//		if (s[0] == '@')
//			return make_pair(string(PO_OP_FILE), s.substr(1));
//		else
//			return pair<string, string>();
//	}

	void options::load_options_file(const std::string& filename) {
		// Load the file and tokenize it
		std::ifstream ifs(filename.c_str());
		if (!ifs) {
			std::cerr << filename << ": " << PO_ERROR_OP_FILE << std::endl;
			//exit(1);
		}
		bpo::store(bpo::parse_config_file(ifs, this->_options), this->vm);
		ifs.close();
		bpo::notify(this->vm);
	}

} }